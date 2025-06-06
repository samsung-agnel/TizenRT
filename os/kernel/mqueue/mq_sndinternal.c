/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/
/****************************************************************************
 *  kernel/mqueue/mq_send.c
 *
 *   Copyright (C) 2007, 2009, 2013-2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <fcntl.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <sched.h>
#include <debug.h>

#include <tinyara/kmalloc.h>
#include <tinyara/arch.h>
#include <tinyara/sched.h>
#include <tinyara/cancelpt.h>
#include "sched/sched.h"
#ifndef CONFIG_DISABLE_SIGNALS
#include "signal/signal.h"
#endif
#include "mqueue/mqueue.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: mq_verifysend
 *
 * Description:
 *   This is internal, common logic shared by both mq_send and mq_timesend.
 *   This function verifies the input parameters that are common to both
 *   functions.
 *
 * Parameters:
 *   mqdes - Message queue descriptor
 *   msg - Message to send
 *   msglen - The length of the message in bytes
 *   prio - The priority of the message
 *
 * Return Value:
 *   One success, 0 (OK) is returned. On failure, -1 (ERROR) is returned and
 *   the errno is set appropriately:
 *
 *   EINVAL   Either msg or mqdes is NULL or the value of prio is invalid.
 *   EPERM    Message queue opened not opened for writing.
 *   EMSGSIZE 'msglen' was greater than the maxmsgsize attribute of the
 *             message queue.
 *
 * Assumptions:
 *
 ****************************************************************************/

int mq_verifysend(mqd_t mqdes, FAR const char *msg, size_t msglen, int prio)
{
	/* Verify the input parameters */

	if (!msg || !mqdes || prio < 0 || prio > MQ_PRIO_MAX) {
		set_errno(EINVAL);
		return ERROR;
	}

	if ((mqdes->oflags & O_WROK) == 0) {
		set_errno(EPERM);
		return ERROR;
	}

	if (msglen > mqdes->msgq->maxmsgsize) {
		set_errno(EMSGSIZE);
		return ERROR;
	}

	return OK;
}

/****************************************************************************
 * Name: mq_msgalloc
 *
 * Description:
 *   The mq_msgalloc function will get a free message for use by the
 *   operating system.  The message will be allocated from the g_msgfree
 *   list.
 *
 *   If the list is empty AND the message is NOT being allocated from the
 *   interrupt level, then the message will be allocated.  If a message
 *   cannot be obtained, the operating system is dead and therefore cannot
 *   continue.
 *
 *   If the list is empty AND the message IS being allocated from the
 *   interrupt level.  This function will attempt to get a message from
 *   the g_msgfreeirq list.  If this is unsuccessful, the calling interrupt
 *   handler will be notified.
 *
 * Inputs:
 *   None
 *
 * Return Value:
 *   A reference to the allocated msg structure.
 *   NULL on a failure to allocate,
 *    if from interrupt handler, set the errno as EBUSY,
 *    if from normal thread, set the errno as ENOMEM.
 *
 ****************************************************************************/

FAR struct mqueue_msg_s *mq_msgalloc(void)
{
	FAR struct mqueue_msg_s *mqmsg;
	irqstate_t saved_state;

	/* If we were called from an interrupt handler, then try to get the message
	 * from generally available list of messages. If this fails, then try the
	 * list of messages reserved for interrupt handlers
	 */

	if (up_interrupt_context()) {
		/* Try the general free list */

		mqmsg = (FAR struct mqueue_msg_s *)sq_remfirst(&g_msgfree);
		if (!mqmsg) {
			/* Try the free list reserved for interrupt handlers */

			mqmsg = (FAR struct mqueue_msg_s *)sq_remfirst(&g_msgfreeirq);
		}
		if (!mqmsg) {
			set_errno(EBUSY);
		}
	}

	/* We were not called from an interrupt handler. */

	else {
		/* Try to get the message from the generally available free list.
		 * Disable interrupts -- we might be called from an interrupt handler.
		 */

		saved_state = enter_critical_section();
		mqmsg = (FAR struct mqueue_msg_s *)sq_remfirst(&g_msgfree);
		leave_critical_section(saved_state);

		/* If we cannot a message from the free list, then we will have to allocate one. */

		if (!mqmsg) {
			mqmsg = (FAR struct mqueue_msg_s *)kmm_malloc((sizeof(struct mqueue_msg_s)));

			/* Check if we got an allocated message */
			if (mqmsg) {
				mqmsg->type = MQ_ALLOC_DYN;
			} else {
				set_errno(ENOMEM);
			}
		}
	}

	return mqmsg;
}

/****************************************************************************
 * Name: mq_waitsend
 *
 * Description:
 *   This is internal, common logic shared by both mq_send and mq_timesend.
 *   This function waits until the message queue is not full.
 *
 * Parameters:
 *   mqdes - Message queue descriptor
 *
 * Return Value:
 *   On success, mq_send() returns 0 (OK); on error, -1 (ERROR) is
 *   returned, with errno set to indicate the error:
 *
 *   EAGAIN   The queue was empty, and the O_NONBLOCK flag was set for the
 *            message queue description referred to by mqdes.
 *   EINTR    The call was interrupted by a signal handler.
 *   ETIMEOUT A timeout expired before the message queue became non-full
 *            (mq_timedsend only).
 *
 * Assumptions/restrictions:
 * - The caller has verified the input parameters using mq_verifysend().
 * - Interrupts are disabled.
 *
 ****************************************************************************/

int mq_waitsend(mqd_t mqdes)
{
	FAR struct tcb_s *rtcb;
	FAR struct mqueue_inode_s *msgq;

	/* mq_waitsend() is not a cancellation point, but it is always called from
	 * a cancellation point.
	 */
	if (enter_cancellation_point()) {
		/* If there is a pending cancellation, then do not perform
		 * the wait. Exit now with ECANCELED.
		 */
		set_errno(ECANCELED);
		leave_cancellation_point();
		return ERROR;
	}

	/* Get a pointer to the message queue */

	msgq = mqdes->msgq;

	/* Verify that the queue is indeed full as the caller thinks */

	if (msgq->nmsgs >= msgq->maxmsgs) {
		/* Should we block until there is sufficient space in the
		 * message queue?
		 */

		if ((mqdes->oflags & O_NONBLOCK) != 0) {
			/* No... We will return an error to the caller. */

			set_errno(EAGAIN);
			leave_cancellation_point();
			return ERROR;
		}

		/* Yes... We will not return control until the message queue is
		 * available or we receive a signal or at timout occurs.
		 */

		else {
			/* Loop until there are fewer than max allowable messages in the
			 * receiving message queue
			 */

			while (msgq->nmsgs >= msgq->maxmsgs) {
				/* Block until the message queue is no longer full.
				 * When we are unblocked, we will try again
				 */

				rtcb = this_task();
				rtcb->msgwaitq = msgq;
				msgq->nwaitnotfull++;

				set_errno(OK);
				up_block_task(rtcb, TSTATE_WAIT_MQNOTFULL);

				/* When we resume at this point, either (1) the message queue
				 * is no longer empty, or (2) the wait has been interrupted by
				 * a signal.  We can detect the latter case be examining the
				 * errno value (should be EINTR or ETIMEOUT).
				 */

				if (get_errno() != OK) {
					leave_cancellation_point();
					return ERROR;
				}
			}
		}
	}
	leave_cancellation_point();
	return OK;
}

/****************************************************************************
 * Name: mq_dosend
 *
 * Description:
 *   This is internal, common logic shared by both mq_send and mq_timesend.
 *   This function adds the specificied message (msg) to the message queue
 *   (mqdes).  Then it notifies any tasks that were waiting for message
 *   queue notifications setup by mq_notify.  And, finally, it awakens any
 *   tasks that were waiting for the message not empty event.
 *
 * Parameters:
 *   mqdes - Message queue descriptor
 *   msg - Message to send
 *   msglen - The length of the message in bytes
 *   prio - The priority of the message
 *
 * Return Value:
 *   This function always returns OK.
 *
 * Assumptions/restrictions:
 *
 ****************************************************************************/

int mq_dosend(mqd_t mqdes, FAR struct mqueue_msg_s *mqmsg, FAR const char *msg, size_t msglen, int prio)
{
	FAR struct tcb_s *btcb;
	FAR struct mqueue_inode_s *msgq;
	FAR struct mqueue_msg_s *next;
	FAR struct mqueue_msg_s *prev;
	irqstate_t saved_state;

	/* Get a pointer to the message queue */

	sched_lock();
	msgq = mqdes->msgq;

	/* Construct the message header info */

	mqmsg->priority = prio;
	mqmsg->msglen = msglen;

	/* Copy the message data into the message */

	memcpy((void *)mqmsg->mail, (FAR const void *)msg, msglen);

	/* Insert the new message in the message queue */

	saved_state = enter_critical_section();

	/* Search the message list to find the location to insert the new
	 * message. Each is list is maintained in ascending priority order.
	 */

	for (prev = NULL, next = (FAR struct mqueue_msg_s *)msgq->msglist.head; next && prio <= next->priority; prev = next, next = next->next) ;

	/* Add the message at the right place */

	if (prev) {
		sq_addafter((FAR sq_entry_t *)prev, (FAR sq_entry_t *)mqmsg, &msgq->msglist);
	} else {
		sq_addfirst((FAR sq_entry_t *)mqmsg, &msgq->msglist);
	}

	/* Increment the count of messages in the queue */

	msgq->nmsgs++;
	leave_critical_section(saved_state);

	/* Check if we need to notify any tasks that are attached to the
	 * message queue
	 */

#ifndef CONFIG_DISABLE_SIGNALS
	if (msgq->ntmqdes) {
		/* Remove the message notification data from the message queue. */

#ifdef CONFIG_CAN_PASS_STRUCTS
		union sigval value = msgq->ntvalue;
#else
		void *sival_ptr = msgq->ntvalue.sival_ptr;
#endif
		int signo = msgq->ntsigno;
		int pid = msgq->ntpid;

		/* Detach the notification */

		msgq->ntpid = INVALID_PROCESS_ID;
		msgq->ntsigno = 0;
		msgq->ntvalue.sival_int = 0;
		msgq->ntmqdes = NULL;

		/* Queue the signal -- What if this returns an error? */

#ifdef CONFIG_CAN_PASS_STRUCTS
		sig_mqnotempty(pid, signo, value);
#else
		sig_mqnotempty(pid, signo, sival_ptr);
#endif
	}
#endif

	/* Check if any tasks are waiting for the MQ not empty event. */

	saved_state = enter_critical_section();
	if (msgq->nwaitnotempty > 0) {
		/* Find the highest priority task that is waiting for
		 * this queue to be non-empty in g_waitingformqnotempty
		 * list. sched_lock() should give us sufficent protection since
		 * interrupts should never cause a change in this list
		 */

		for (btcb = (FAR struct tcb_s *)g_waitingformqnotempty.head; btcb && btcb->msgwaitq != msgq; btcb = btcb->flink) ;

		/* If one was found, unblock it */

		ASSERT(btcb);

		btcb->msgwaitq = NULL;
		msgq->nwaitnotempty--;
		up_unblock_task(btcb);
	}

	leave_critical_section(saved_state);
	sched_unlock();
	return OK;
}
