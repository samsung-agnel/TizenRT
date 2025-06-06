/*
 *******************************************************************************
 * Copyright(c) 2022, Realtek Semiconductor Corporation. All rights reserved.
 *******************************************************************************
 */

#include <string.h>
#include <stdio.h>
#include <osif.h>

#include <bt_api_config.h>
#include <rtk_bt_def.h>
#include <rtk_bt_common.h>
#include <rtk_bt_device.h>
#include <rtk_bt_le_gap.h>
#include <rtk_bt_gatts.h>
#include <rtk_bt_gattc.h>

#include <bt_utils.h>
#include <rtk_service_config.h>
#include <rtk_client_config.h>
#include <rtk_gcs_client.h>
#include <rtk_stack_config.h>
#include <tinyara/net/if/ble.h>
#include <ble_tizenrt_service.h>
#include <debug.h>

#define RTK_BT_DEV_NAME "BLE_TIZENRT"
#define BT_APP_PROCESS(func)                                \
	do {                                                    \
		uint16_t __func_ret = func;                         \
		if (RTK_BT_OK != __func_ret) {                      \
			dbg("[APP] %s failed! line: %d, err: 0x%x\r\n", __func__, __LINE__, __func_ret);   \
			return -1;                                      \
		}                                                   \
	} while (0)
//#define CONFIG_DEBUG_SCAN_INFO

static uint8_t adv_data[] = {
    0x02, //AD len
    RTK_BT_LE_GAP_ADTYPE_FLAGS, //AD types
    RTK_BT_LE_GAP_ADTYPE_FLAGS_GENERAL | RTK_BT_LE_GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED, //AD data
    0xc,
    RTK_BT_LE_GAP_ADTYPE_LOCAL_NAME_COMPLETE,
    'B', 'L', 'E', '_', 'T', 'I', 'Z', 'E', 'N', 'R', 'T', 
};

static uint8_t scan_rsp_data[] = {
    0x3,
    RTK_BT_LE_GAP_ADTYPE_APPEARANCE, //GAP_ADTYPE_APPEARANCE
    0x00,
    0x00,
};

// static rtk_bt_le_adv_param_t adv_param = {
//     .interval_min = 0x30,
//     .interval_max = 0x60,
//     .type = RTK_BT_LE_ADV_TYPE_IND,
//     .own_addr_type = RTK_BT_LE_ADDR_TYPE_PUBLIC,
//     .peer_addr = {
//         .type = (rtk_bt_le_addr_type_t)0,
//         .addr_val = {0},
//     },
//     .channel_map = RTK_BT_LE_ADV_CHNL_ALL,
//     .filter_policy = RTK_BT_LE_ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
// };

// static rtk_bt_le_scan_param_t scan_param = {
//     .type          = RTK_BT_LE_SCAN_TYPE_PASSIVE,
//     .interval      = 0x60,
//     .window        = 0x30,
//     .own_addr_type = RTK_BT_LE_ADDR_TYPE_PUBLIC,
//     .filter_policy = RTK_BT_LE_SCAN_FILTER_ALLOW_ALL,
// };

typedef struct {
    bool is_active;
    rtk_bt_le_link_role_t role;
} app_conn_table_t;

extern bool is_secured;
extern trble_client_init_config *client_init_parm;
extern trble_server_init_config server_init_parm;
extern TIZENERT_SRV_DATABASE tizenrt_ble_srv_database[7];
extern rtk_bt_le_conn_ind_t *ble_tizenrt_scatternet_conn_ind;
extern uint8_t *del_bond_addr;
extern uint8_t ble_client_connect_is_running;
extern void *ble_tizenrt_read_sem;
extern void *ble_tizenrt_write_sem;
extern void *ble_tizenrt_write_no_rsp_sem;
extern uint16_t scan_timeout;

static app_conn_table_t conn_link[RTK_BLE_GAP_MAX_LINKS] = {0};
rtk_bt_gattc_read_ind_t ble_tizenrt_scatternet_read_results[RTK_BLE_GAP_MAX_LINKS] = {0};
rtk_bt_gattc_write_ind_t g_scatternet_write_result = {0};
rtk_bt_gattc_write_ind_t g_scatternet_write_no_rsp_result = {0};
trble_device_connected ble_tizenrt_scatternet_bond_list[RTK_BLE_GAP_MAX_LINKS] = {0};

static rtk_bt_evt_cb_ret_t ble_tizenrt_scatternet_gap_app_callback(uint8_t evt_code, void* param) 
{
    char le_addr[30] = {0};
    char *role;

    switch (evt_code) {
    case RTK_BT_LE_GAP_EVT_ADV_START_IND: {
        rtk_bt_le_adv_start_ind_t *adv_start_ind = (rtk_bt_le_adv_start_ind_t *)param;
        if(!adv_start_ind->err)
            dbg("[APP] ADV started: adv_type %d  \r\n", adv_start_ind->adv_type);
        else
            dbg("[APP] ADV start failed, err 0x%x \r\n", adv_start_ind->err);
        break;
    }

    case RTK_BT_LE_GAP_EVT_ADV_STOP_IND: {
        rtk_bt_le_adv_stop_ind_t *adv_stop_ind = (rtk_bt_le_adv_stop_ind_t *)param;
        if(!adv_stop_ind->err)
            dbg("[APP] ADV stopped: reason 0x%x \r\n",adv_stop_ind->stop_reason);
        else
            dbg("[APP] ADV stop failed, err 0x%x \r\n", adv_stop_ind->err);
        break;
    }

    case RTK_BT_LE_GAP_EVT_SCAN_START_IND: {
        rtk_bt_le_scan_start_ind_t *scan_start_ind = (rtk_bt_le_scan_start_ind_t *)param;
        if (!scan_start_ind->err) {
			client_init_parm->trble_scan_state_changed_cb(TRBLE_SCAN_STARTED);
            dbg("[APP] Scan started, scan_type: %d\r\n", scan_start_ind->scan_type);
        } else {
            dbg("[APP] Scan start failed(err: 0x%x)\r\n", scan_start_ind->err);
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_SCAN_RES_IND: {
        rtk_bt_le_scan_res_ind_t *scan_res_ind = (rtk_bt_le_scan_res_ind_t *)param;
        rtk_bt_le_addr_to_str(&(scan_res_ind->adv_report.addr), le_addr, sizeof(le_addr));
#if defined(CONFIG_DEBUG_SCAN_INFO)
        debug_print("[APP] Scan info, [Device]: %s, AD evt type: %d, RSSI: %i\r\n", 
                le_addr, scan_res_ind->adv_report.evt_type, scan_res_ind->adv_report.rssi);
#endif
		trble_scanned_device scanned_device;
	    scanned_device.adv_type = scan_res_ind->adv_report.evt_type;
	    if(TRBLE_ADV_TYPE_SCAN_RSP == scanned_device.adv_type){
		    memcpy(scanned_device.resp_data, scan_res_ind->adv_report.data, scan_res_ind->adv_report.len);
		    scanned_device.raw_data_length = scan_res_ind->adv_report.len;
	    }else{
		    memcpy(scanned_device.raw_data, scan_res_ind->adv_report.data, scan_res_ind->adv_report.len);
		    scanned_device.raw_data_length = scan_res_ind->adv_report.len;
	    }
	    memcpy(scanned_device.addr.mac, scan_res_ind->adv_report.addr.addr_val, RTK_BD_ADDR_LEN);
		scanned_device.addr.type = scan_res_ind->adv_report.addr.type;
	    scanned_device.rssi = scan_res_ind->adv_report.rssi;
	    client_init_parm->trble_device_scanned_cb(&scanned_device);
        break;
    }
    
    case RTK_BT_LE_GAP_EVT_SCAN_STOP_IND: {
        rtk_bt_le_scan_stop_ind_t *scan_stop_ind = (rtk_bt_le_scan_stop_ind_t *)param;
        if (!scan_stop_ind->err) {
			client_init_parm->trble_scan_state_changed_cb(TRBLE_SCAN_STOPPED);
            dbg("[APP] Scan stopped\r\n");
        } else {
            dbg("[APP] Scan stop failed(err: 0x%x)\r\n", scan_stop_ind->err);
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_CONNECT_IND: {
        rtk_bt_le_conn_ind_t *conn_ind = (rtk_bt_le_conn_ind_t *)param;
		memcpy(ble_tizenrt_scatternet_conn_ind, conn_ind, sizeof(rtk_bt_le_conn_ind_t));
        rtk_bt_le_addr_to_str(&(conn_ind->peer_addr), le_addr, sizeof(le_addr));
        if (!conn_ind->err) {
            role = conn_ind->role ? "slave" : "master";
            dbg("[APP] Connected, handle: %d, role: %s, remote device: %s\r\n", 
                    conn_ind->conn_handle, role, le_addr);
            uint8_t conn_id;
            rtk_bt_le_gap_get_conn_id(conn_ind->conn_handle, &conn_id);
            conn_link[conn_id].is_active = true;
            conn_link[conn_id].role = conn_ind->role;
            /* gattc action */
            if (RTK_BT_LE_ROLE_MASTER == conn_link[conn_id].role &&
                RTK_BT_OK == general_client_attach_conn(conn_ind->conn_handle)) {
                dbg("[APP] GATTC Profiles attach connection success, conn_handle: %d\r\n",
																	conn_ind->conn_handle);
            }
			if(RTK_BT_LE_ROLE_MASTER == conn_ind->role)
			{
				if(is_secured && !ble_tizenrt_scatternet_bond_list[conn_id].is_bonded)
				{
					rtk_bt_le_get_active_conn_t active_conn;
					if (RTK_BT_OK != rtk_bt_le_gap_get_active_conn(&active_conn))
					{
						dbg("[APP] Start security flow failed!");
					}
					if (active_conn.conn_num > GAP_MAX_LINKS)
					{
						dbg("[APP] Start security flow failed!");
					}
					if (RTK_BT_OK != rtk_bt_le_sm_start_security(conn_ind->conn_handle))
					{
						dbg("[APP] Start security flow failed!");
					}
				} else {
					debug_print("LL connected %d, do not need pairing \n", conn_ind->conn_handle);
					trble_device_connected connected_dev;
					uint16_t mtu_size = 0;
					if(RTK_BT_OK != rtk_bt_le_gap_get_mtu_size(conn_ind->conn_handle, &mtu_size)){
						dbg("[APP] Get mtu size failed \r\n");
					}
					connected_dev.conn_handle = conn_ind->conn_handle;
					connected_dev.is_bonded = ble_tizenrt_scatternet_bond_list[conn_id].is_bonded;
					memcpy(connected_dev.conn_info.addr.mac, conn_ind->peer_addr.addr_val, RTK_BD_ADDR_LEN);
					connected_dev.conn_info.conn_interval = conn_ind->conn_interval;
					connected_dev.conn_info.slave_latency = conn_ind->conn_latency;
					connected_dev.conn_info.scan_timeout = scan_timeout;
					connected_dev.conn_info.is_secured_connect = is_secured;
					connected_dev.conn_info.mtu = mtu_size;
					client_init_parm->trble_device_connected_cb(&connected_dev);
				}

				if(ble_client_connect_is_running)
					ble_client_connect_is_running = 0;

			}else if (RTK_BT_LE_ROLE_SLAVE == conn_ind->role) {
				server_init_parm.connected_cb(conn_ind->conn_handle, TRBLE_SERVER_LL_CONNECTED, conn_ind->peer_addr.addr_val, 0xff);// the last field 0xff is a dummy value for adv handle which is for AI-Lite only
			}

        } else {
            dbg("[APP] Connection establish failed(err: 0x%x), remote device: %s\r\n",
                    conn_ind->err, le_addr);
			if(ble_client_connect_is_running)
				ble_client_connect_is_running = 0;
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_DISCONN_IND: {
        rtk_bt_le_disconn_ind_t *disconn_ind = (rtk_bt_le_disconn_ind_t *)param;
        rtk_bt_le_addr_to_str(&(disconn_ind->peer_addr), le_addr, sizeof(le_addr));
        role = disconn_ind->role ? "slave" : "master";
        dbg("[APP] Disconnected, reason: 0x%x, handle: %d, role: %s, remote device: %s\r\n", 
                disconn_ind->reason, disconn_ind->conn_handle, role, le_addr);
        client_init_parm->trble_device_disconnected_cb(disconn_ind->conn_handle);

        if(ble_client_connect_is_running)
            ble_client_connect_is_running = 0;

        if(ble_tizenrt_read_sem != NULL) {
            osif_sem_give(ble_tizenrt_read_sem);
            ble_tizenrt_read_sem = NULL;
        }
        if(ble_tizenrt_write_sem != NULL) {
            osif_sem_give(ble_tizenrt_write_sem);
            ble_tizenrt_write_sem = NULL;
        }
        if(ble_tizenrt_write_no_rsp_sem != NULL) {
            osif_sem_give(ble_tizenrt_write_no_rsp_sem);
            ble_tizenrt_write_no_rsp_sem = NULL;
        }

        uint8_t conn_id;
        rtk_bt_le_gap_get_conn_id(disconn_ind->conn_handle, &conn_id);
        memset(&conn_link[conn_id], 0, sizeof(app_conn_table_t));
        /* gattc action */
        general_client_detach_conn(disconn_ind->conn_handle);
        /* gap action */
        if (RTK_BT_LE_ROLE_SLAVE == disconn_ind->role) {
            //rtk_bt_le_gap_start_adv(&adv_param);
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_CONN_UPDATE_IND: {
        rtk_bt_le_conn_update_ind_t *conn_update_ind = 
                                        (rtk_bt_le_conn_update_ind_t *)param;
        if (conn_update_ind->err) {
            dbg("[APP] Update conn param failed, conn_handle: %d, err: 0x%x\r\n", 
                    conn_update_ind->conn_handle, conn_update_ind->err);
        } else {
            dbg("[APP] Conn param is updated, conn_handle: %d, conn_interval: 0x%x, "       \
                    "conn_latency: 0x%x, supervision_Timeout: 0x%x\r\n",
                        conn_update_ind->conn_handle, 
                        conn_update_ind->conn_interval,
                        conn_update_ind->conn_latency, 
                        conn_update_ind->supv_timeout);
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_REMOTE_CONN_UPDATE_REQ_IND: { //BT sync api shall not be called here
        rtk_bt_le_remote_conn_update_req_ind_t *rmt_update_req = 
                                (rtk_bt_le_remote_conn_update_req_ind_t *)param;
        dbg("[APP] Remote device request a change in conn param, conn_handle: %d, "      \
                "conn_interval_max: 0x%x, conn_interval_min: 0x%x, conn_latency: 0x%x, "      \
                "timeout: 0x%x. The host stack accept it.\r\n",
                rmt_update_req->conn_handle, 
                rmt_update_req->conn_interval_max,
                rmt_update_req->conn_interval_min,
                rmt_update_req->conn_latency,
                rmt_update_req->supv_timeout);
        return RTK_BT_EVT_CB_ACCEPT;
        break;
    }

    case RTK_BT_LE_GAP_EVT_DATA_LEN_CHANGE_IND: {
        rtk_bt_le_data_len_change_ind_t *data_len_change = 
                                        (rtk_bt_le_data_len_change_ind_t *)param;
        dbg("[APP] Data len is updated, conn_handle: %d, "       \
                "max_tx_octets: 0x%x, max_tx_time: 0x%x, "        \
                "max_rx_octets: 0x%x, max_rx_time: 0x%x\r\n", 
                data_len_change->conn_handle,
                data_len_change->max_tx_octets, 
                data_len_change->max_tx_time,
                data_len_change->max_rx_octets, 
                data_len_change->max_rx_time);
        break;
    }

    case RTK_BT_LE_GAP_EVT_PHY_UPDATE_IND: {
        rtk_bt_le_phy_update_ind_t *phy_update_ind = 
                                    (rtk_bt_le_phy_update_ind_t *)param;
        if (phy_update_ind->err) {
            dbg("[APP] Update PHY failed, conn_handle: %d, err: 0x%x\r\n", 
                    phy_update_ind->conn_handle,
                    phy_update_ind->err);
        } else {
            dbg("[APP] PHY is updated, conn_handle: %d, tx_phy: %d, rx_phy: %d\r\n",
                    phy_update_ind->conn_handle, 
                    phy_update_ind->tx_phy, 
                    phy_update_ind->rx_phy);
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_AUTH_PAIRING_CONFIRM_IND:{
        rtk_bt_le_auth_pair_cfm_ind_t *pair_cfm_ind = 
                                        (rtk_bt_le_auth_pair_cfm_ind_t *)param;
        APP_PROMOTE("[APP] Just work pairing need user to confirm, conn_handle: %d!\r\n", 
                                                                pair_cfm_ind->conn_handle);
        rtk_bt_le_pair_cfm_t pair_cfm_param = {0};
        uint16_t ret = 0;
        pair_cfm_param.conn_handle = pair_cfm_ind->conn_handle;
        pair_cfm_param.confirm = 1;
        ret = rtk_bt_le_sm_pairing_confirm(&pair_cfm_param);
        if (RTK_BT_OK == ret) {
            dbg("[APP] Just work pairing auto confirm succcess\r\n");
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_AUTH_PASSKEY_DISPLAY_IND:{
        rtk_bt_le_auth_key_display_ind_t *key_dis_ind =
                                        (rtk_bt_le_auth_key_display_ind_t *)param;
        APP_PROMOTE("[APP] Auth passkey display: %d, conn_handle:%d\r\n", 
                                                    key_dis_ind->passkey,
                                                    key_dis_ind->conn_handle);
        break;
    }

    case RTK_BT_LE_GAP_EVT_AUTH_PASSKEY_INPUT_IND: {
        rtk_bt_le_auth_key_input_ind_t *key_input_ind = 
                                        (rtk_bt_le_auth_key_input_ind_t *)param;
        APP_PROMOTE("[APP] Please input the auth passkey get from remote, conn_handle: %d\r\n",
                                                                key_input_ind->conn_handle);
        break;
    }

    case RTK_BT_LE_GAP_EVT_AUTH_PASSKEY_CONFIRM_IND: {
        rtk_bt_le_auth_key_cfm_ind_t *key_cfm_ind = 
                                        (rtk_bt_le_auth_key_cfm_ind_t *)param;
        APP_PROMOTE("[APP] Auth passkey confirm: %d, conn_handle: %d."  \
                    "Please comfirm if the passkeys are equal!\r\n",
                                                key_cfm_ind->passkey,
                                                key_cfm_ind->conn_handle);
        break;
    }

    case RTK_BT_LE_GAP_EVT_AUTH_OOB_KEY_INPUT_IND:{
        rtk_bt_le_auth_oob_input_ind_t *oob_input_ind = 
                                        (rtk_bt_le_auth_oob_input_ind_t *)param;
        APP_PROMOTE("[APP] Bond use oob key, conn_handle: %d. Please input the oob tk \r\n",
                                                                oob_input_ind->conn_handle);
        break;
    }

    case RTK_BT_LE_GAP_EVT_AUTH_COMPLETE_IND: {
        rtk_bt_le_auth_complete_ind_t *auth_cplt_ind = 
                                        (rtk_bt_le_auth_complete_ind_t *)param;
        if (auth_cplt_ind->err) {
            dbg("[APP] Pairing failed(err: 0x%x), conn_handle: %d\r\n", 
                    auth_cplt_ind->err, auth_cplt_ind->conn_handle);
        } else {
            dbg("[APP] Pairing success, conn_handle: %d\r\n", auth_cplt_ind->conn_handle);
			if(RTK_BT_LE_ROLE_MASTER == ble_tizenrt_scatternet_conn_ind->role)
			{
				uint8_t conn_id;
				rtk_bt_le_gap_get_conn_id(auth_cplt_ind->conn_handle, &conn_id);
				ble_tizenrt_scatternet_bond_list[conn_id].is_bonded = true;
				memcpy(ble_tizenrt_scatternet_bond_list[conn_id].conn_info.addr.mac, ble_tizenrt_scatternet_conn_ind->peer_addr.addr_val, RTK_BD_ADDR_LEN);
				trble_device_connected connected_dev;
				uint16_t mtu_size = 0;
				if(RTK_BT_OK != rtk_bt_le_gap_get_mtu_size(auth_cplt_ind->conn_handle, &mtu_size)){
					dbg("[APP] Get mtu size failed \r\n");
				}
				connected_dev.conn_handle = ble_tizenrt_scatternet_conn_ind->conn_handle;
				connected_dev.is_bonded = ble_tizenrt_scatternet_bond_list[conn_id].is_bonded;
				memcpy(connected_dev.conn_info.addr.mac, ble_tizenrt_scatternet_conn_ind->peer_addr.addr_val, RTK_BD_ADDR_LEN);
				connected_dev.conn_info.conn_interval = ble_tizenrt_scatternet_conn_ind->conn_interval;
				connected_dev.conn_info.slave_latency = ble_tizenrt_scatternet_conn_ind->conn_latency;
				connected_dev.conn_info.scan_timeout = scan_timeout;
				connected_dev.conn_info.is_secured_connect = is_secured;
				connected_dev.conn_info.mtu = mtu_size;
				client_init_parm->trble_device_connected_cb(&connected_dev);
			}else if(RTK_BT_LE_ROLE_SLAVE == ble_tizenrt_scatternet_conn_ind->role)
			{
				server_init_parm.connected_cb(auth_cplt_ind->conn_handle, TRBLE_SERVER_SM_CONNECTED, ble_tizenrt_scatternet_conn_ind->peer_addr.addr_val, 0xff);// the last field 0xff is a dummy value for adv handle which is for AI-Lite only			
			}
        }
        break;
    }

    case RTK_BT_LE_GAP_EVT_BOND_MODIFY_IND: {
        rtk_bt_le_bond_modify_ind_t *bond_mdf_ind = 
                                        (rtk_bt_le_bond_modify_ind_t *)param;
        dbg("[APP] Bond info modified, op: %d", bond_mdf_ind->op);
		if(RTK_BT_LE_BOND_DELETE == bond_mdf_ind->op && del_bond_addr){
			for(int i = 0; i < GAP_MAX_LINKS; i++)
			{
				if(!memcmp(ble_tizenrt_scatternet_bond_list[i].conn_info.addr.mac, del_bond_addr, RTK_BD_ADDR_LEN))
				{
					ble_tizenrt_scatternet_bond_list[i].is_bonded = false;
					memset(ble_tizenrt_scatternet_bond_list[i].conn_info.addr.mac, 0, RTK_BD_ADDR_LEN);
					break;
				}
			}			
		}else if(RTK_BT_LE_BOND_CLEAR == bond_mdf_ind->op){
			for(int i = 0; i < GAP_MAX_LINKS; i++)
			{
				ble_tizenrt_scatternet_bond_list[i].is_bonded = false;
				memset(ble_tizenrt_scatternet_bond_list[i].conn_info.addr.mac, 0, RTK_BD_ADDR_LEN);
			}				
		}
        break;
    }

    default:
        debug_print("[APP] Unkown gap cb evt type: %d", evt_code);
        break;
	}

    return RTK_BT_EVT_CB_OK;
}

static uint16_t app_get_gatts_app_id(uint8_t event, void *data)
{
    uint16_t app_id = 0xFFFF;

    switch(event) {
    case RTK_BT_GATTS_EVT_REGISTER_SERVICE: {
        rtk_bt_gatts_reg_ind_t *p_reg_ind = (rtk_bt_gatts_reg_ind_t *)data;
        app_id = p_reg_ind->app_id;
        break;
    }
    case RTK_BT_GATTS_EVT_READ_IND: {
        rtk_bt_gatts_read_ind_t *p_read_ind = (rtk_bt_gatts_read_ind_t *)data;
        app_id = p_read_ind->app_id;
        break;
    }
    case RTK_BT_GATTS_EVT_WRITE_IND: {
        rtk_bt_gatts_write_ind_t *p_write_ind = (rtk_bt_gatts_write_ind_t *)data;
        app_id = p_write_ind->app_id;
        break;
    }
    case RTK_BT_GATTS_EVT_CCCD_IND: {
        rtk_bt_gatts_cccd_ind_t *p_cccd_ind = (rtk_bt_gatts_cccd_ind_t *)data;
        app_id = p_cccd_ind->app_id;
        break;
    }
    case RTK_BT_GATTS_EVT_NOTIFY_COMPLETE_IND:
    case RTK_BT_GATTS_EVT_INDICATE_COMPLETE_IND: {
        rtk_bt_gatts_ntf_and_ind_ind_t *p_ind_ntf = (rtk_bt_gatts_ntf_and_ind_ind_t *)data;
        app_id = p_ind_ntf->app_id;
        break;
    }
    default:
        break; 
    }

    return app_id;
}

static rtk_bt_evt_cb_ret_t ble_tizenrt_scatternet_gatts_app_callback(uint8_t event, void *data)
{
	/* printf("ble_peripheral_gatts_service_callback: app_id = %d, HID_SRV_ID = %d \r\n",app_id,HID_SRV_ID); */

    if (RTK_BT_GATTS_EVT_MTU_EXCHANGE == event) {
        rtk_bt_gatt_mtu_exchange_ind_t *p_gatt_mtu_ind = (rtk_bt_gatt_mtu_exchange_ind_t *)data;
        if(p_gatt_mtu_ind->result == RTK_BT_OK){
            dbg("[APP] GATTS mtu exchange successfully, mtu_size: %d, conn_handle: %d \r\n",
                        p_gatt_mtu_ind->mtu_size, p_gatt_mtu_ind->conn_handle);
        }else{
            dbg("[APP] GATTS mtu exchange fail \r\n");
        }
        return RTK_BT_EVT_CB_OK;
    }else{
		ble_tizenrt_srv_callback(event,data);
    }

    return RTK_BT_EVT_CB_OK;
}

static uint16_t app_get_gattc_profile_id(uint8_t event, void *data)
{
    uint16_t profile_id = 0xFFFF;

    switch(event) {
    case RTK_BT_GATTC_EVT_DISCOVER_RESULT_IND: {
        rtk_bt_gattc_discover_ind_t *disc_res = (rtk_bt_gattc_discover_ind_t *)data;
        profile_id = disc_res->profile_id;
        break;
    }
    case RTK_BT_GATTC_EVT_READ_RESULT_IND: {
        rtk_bt_gattc_read_ind_t *read_res = (rtk_bt_gattc_read_ind_t *)data;
        profile_id = read_res->profile_id;
        break;
    }
    case RTK_BT_GATTC_EVT_WRITE_RESULT_IND: {
        rtk_bt_gattc_write_ind_t *write_res = (rtk_bt_gattc_write_ind_t *)data;
        profile_id = write_res->profile_id;
        break;
    }
    case RTK_BT_GATTC_EVT_NOTIFY_IND: 
    case RTK_BT_GATTC_EVT_INDICATE_IND: {
        rtk_bt_gattc_cccd_value_ind_t *cccd_ind = (rtk_bt_gattc_cccd_value_ind_t *)data;
        profile_id = cccd_ind->profile_id;
        break;
    }
    case RTK_BT_GATTC_EVT_CCCD_ENABLE_IND:
    case RTK_BT_GATTC_EVT_CCCD_DISABLE_IND: {
        rtk_bt_gattc_cccd_update_ind_t *cccd_update = (rtk_bt_gattc_cccd_update_ind_t *)data;
        profile_id = cccd_update->profile_id;
        break;
    }
    default:
        break;
    }

    return profile_id;
}

static rtk_bt_evt_cb_ret_t ble_tizenrt_scatternet_gattc_app_callback(uint8_t event, void *data)
{
	uint16_t profile_id = 0xFFFF;

	if (RTK_BT_GATTC_EVT_MTU_EXCHANGE == event) {
		rtk_bt_gatt_mtu_exchange_ind_t *p_gatt_mtu_ind = (rtk_bt_gatt_mtu_exchange_ind_t *)data;
		if(p_gatt_mtu_ind->result == RTK_BT_OK){
			dbg("[APP] GATTC mtu exchange success, mtu_size: %d, conn_handle: %d \r\n",
						p_gatt_mtu_ind->mtu_size, p_gatt_mtu_ind->conn_handle);
		}else{
			dbg("[APP] GATTC mtu exchange fail \r\n");
		}
		return RTK_BT_EVT_CB_OK;
	}

	profile_id = app_get_gattc_profile_id(event, data);
	switch(profile_id) {
	case GCS_CLIENT_PROFILE_ID:
		general_client_app_callback(event, data);
		break;
	default:
		break;
	}

	return RTK_BT_EVT_CB_OK;
}

extern bool rtk_bt_pre_enable(void);
int ble_tizenrt_scatternet_main(uint8_t enable)
{
    rtk_bt_app_conf_t bt_app_conf = {0};
    rtk_bt_le_addr_t bd_addr = {(rtk_bt_le_addr_type_t)0, {0}};
    char addr_str[30] = {0};
    char name[30] = {0};

    if (1 == enable || 2 == enable)
    {
        if (rtk_bt_pre_enable() == false) {
            dbg("%s fail!\r\n", __func__);
            return -1;
        }

        //set GAP configuration
		bt_app_conf.app_profile_support = RTK_BT_PROFILE_GATTS | RTK_BT_PROFILE_GATTC;
		bt_app_conf.mtu_size = 180;
		bt_app_conf.master_init_mtu_req = true;
		bt_app_conf.prefer_all_phy = 0;
		bt_app_conf.prefer_tx_phy = 1 | 1<<1 | 1<<2;
		bt_app_conf.prefer_rx_phy = 1 | 1<<1 | 1<<2;
		bt_app_conf.max_tx_octets = 0x40;
		bt_app_conf.max_tx_time = 0x200;
		bt_app_conf.user_def_service = false;
		bt_app_conf.cccd_not_check = false;
		
        /* Enable BT */
		BT_APP_PROCESS(rtk_bt_enable(&bt_app_conf)); 
        BT_APP_PROCESS(rtk_bt_le_gap_get_address(&bd_addr));
        rtk_bt_le_addr_to_str(&bd_addr, addr_str, sizeof(addr_str));
        dbg("[APP] BD_ADDR: %s\r\n", addr_str);

        BT_APP_PROCESS(rtk_bt_evt_register_callback(RTK_BT_LE_GP_GAP, 
                                                    ble_tizenrt_scatternet_gap_app_callback));
        memcpy(name,(const uint8_t*)RTK_BT_DEV_NAME,strlen((const char *)RTK_BT_DEV_NAME));
		BT_APP_PROCESS(rtk_bt_le_gap_set_device_name((uint8_t *)name)); 
        BT_APP_PROCESS(rtk_bt_le_gap_set_appearance(RTK_BT_LE_GAP_APPEARANCE_HEART_RATE_BELT));
		
        BT_APP_PROCESS(rtk_bt_le_gap_set_adv_data(adv_data,sizeof(adv_data))); 
        BT_APP_PROCESS(rtk_bt_le_gap_set_scan_rsp_data(scan_rsp_data,sizeof(scan_rsp_data)));
//        BT_APP_PROCESS(rtk_bt_le_gap_start_adv(&adv_param)); 
        /* gatts related */
        BT_APP_PROCESS(rtk_bt_evt_register_callback(RTK_BT_LE_GP_GATTS, 
                                                    ble_tizenrt_scatternet_gatts_app_callback));
		BT_APP_PROCESS(ble_tizenrt_srv_add());
        /* gattc related */
        BT_APP_PROCESS(rtk_bt_evt_register_callback(RTK_BT_LE_GP_GATTC, 
                                                    ble_tizenrt_scatternet_gattc_app_callback));
        BT_APP_PROCESS(general_client_add());
    }
    else if (0 == enable)
    {
		BT_APP_PROCESS(general_client_delete());

        /* no need to unreg callback here, it is done in rtk_bt_disable */
        // BT_APP_PROCESS(rtk_bt_evt_unregister_callback(RTK_BT_LE_GP_GAP));
        // BT_APP_PROCESS(rtk_bt_evt_unregister_callback(RTK_BT_LE_GP_GATTS));
        // BT_APP_PROCESS(rtk_bt_evt_unregister_callback(RTK_BT_LE_GP_GATTC));

        /* Disable BT */
        BT_APP_PROCESS(rtk_bt_disable());

    }

    return 0;
}
