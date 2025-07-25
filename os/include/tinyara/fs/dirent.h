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
 *
 *   Copyright (C) 2007, 2009, 2011-2013 Gregory Nutt. All rights reserved.
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

#ifndef __INCLUDE_FS_DIRENT_H
#define __INCLUDE_FS_DIRENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <dirent.h>

#include <tinyara/fs/fs.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* The internal representation of type DIR is just a container for an inode
 * reference, a position, a dirent structure, and file-system-specific
 * information.
 *
 * For the root pseudo-file system, we need retain only the 'next' inode
 * need for the next readdir() operation.  We hold a reference on this
 * inode so we know that it will persist until closedir is called.
 */

struct fs_pseudodir_s {
	struct inode *fd_next;		/* The inode for the next call to readdir() */
};

#ifndef CONFIG_DISABLE_MOUNTPOINT

#ifdef CONFIG_FS_ROMFS
/* For ROMFS, we need to return the offset to the current and start positions
 * of the directory entry being read
 */

struct fs_romfsdir_s {
	off_t fr_firstoffset;		/* Offset to the first entry in the directory */
	off_t fr_curroffset;		/* Current offset into the directory contents */
};
#endif							/* CONFIG_FS_ROMFS */

#ifdef CONFIG_FS_TMPFS
/* For TMPFS, we need the directory object and an index into the directory
 * entries.
 */
struct tmpfs_directory_s;             /* Forward reference */
struct fs_tmpfsdir_s {
	FAR struct tmpfs_directory_s *tf_tdo; /* Directory being enumerated */
	unsigned int tf_index;               /* Directory index */
};
#endif /* CONFIG_FS_TMPFS */

#ifdef CONFIG_FS_SMARTFS
/* SMARTFS is the Sector Mapped Allocation for Really Tiny FLASH filesystem.
 * it is designed to use small sectors on small serial FLASH devices, using
 * minimal RAM footprint.
 */

struct fs_smartfsdir_s {
	uint16_t fs_firstsector;	/* First sector of directory list */
	uint16_t fs_currsector;		/* Current sector of directory list */
	uint16_t fs_curroffset;		/* Current offset withing current sector */
};
#endif

#endif							/* CONFIG_DISABLE_MOUNTPOINT */

struct fs_dirent_s {
	/* This is the node that was opened by opendir.  The type of the inode
	 * determines the way that the readdir() operations are performed. For the
	 * pseudo root pseudo-file system, it is also used to support rewind.
	 *
	 * We hold a reference on this inode so we know that it will persist until
	 * closedir() is called (although inodes linked to this inode may change).
	 */

	struct inode *fd_root;

	/* At present, only mountpoints require special handling flags */

#ifndef CONFIG_DISABLE_MOUNTPOINT
	unsigned int fd_flags;
#endif

	/* This keeps track of the current directory position for telldir */

	off_t fd_position;

	/* Retained control information depends on the type of file system that
	 * provides is provides the mountpoint.  Ideally this information should
	 * be hidden behind an opaque, file-system-dependent void *, but we put
	 * the private definitions in line here for now to reduce allocations.
	 */

	union {
		/* Private data used by the built-in pseudo-file system */

		struct fs_pseudodir_s pseudo;

		/* Private data used by other file systems */

#ifndef CONFIG_DISABLE_MOUNTPOINT
#ifdef CONFIG_FS_ROMFS
		struct fs_romfsdir_s romfs;
#endif
#ifdef CONFIG_FS_TMPFS
		struct fs_tmpfsdir_s tmpfs;
#endif
#ifdef CONFIG_FS_PROCFS
		FAR void *procfs;
#endif
#ifdef CONFIG_FS_SMARTFS
		struct fs_smartfsdir_s smartfs;
#endif
#ifdef CONFIG_FS_LITTLEFS
		FAR void *littlefs;
#endif
#endif							/* !CONFIG_DISABLE_MOUNTPOINT */
	} u;

	/* In any event, this the actual struct dirent that is returned by readdir */

	struct dirent fd_dir;		/* Populated when readdir is called */
};

/****************************************************************************
 * Global Variables
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif							/* __INCLUDE_FS_DIRENT_H */
