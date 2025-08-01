/****************************************************************************
 *
 * Copyright 2017 Samsung Electronics All Rights Reserved.
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

/**
 * @defgroup Wi-Fi_Manager Wi-Fi_Manager
 * @ingroup Wi-Fi_Manager
 * @brief Provides APIs for Wi-Fi Manager
 * @{
 */

/**
 * @file wifi_manager/wifi_manager.h
 * @brief Provides APIs for Wi-Fi Manager
 */

#pragma once

#include <sys/time.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Length defines */
#define WIFIMGR_MACADDR_LEN 6
#define WIFIMGR_MACADDR_STR_LEN 17
#define WIFIMGR_SSID_LEN 32
#define WIFIMGR_2G_CHANNEL_MAX 14
#define WIFIMGR_PASSPHRASE_LEN 64
#define WIFIMGR_PASSPHRASE_LEN_MIN 8
#define WIFIMGR_SPECIFIC_SCAN_AP_CNT 6
/**
 * @brief <b> wifi MAC/PHY standard types
 */
typedef enum {
	WIFI_MANAGER_IEEE_80211_LEGACY, /**<  IEEE 802.11a/g/b           */
	WIFI_MANAGER_IEEE_80211_A,		/**<  IEEE 802.11a               */
	WIFI_MANAGER_IEEE_80211_B,		/**<  IEEE 802.11b               */
	WIFI_MANAGER_IEEE_80211_G,		/**<  IEEE 802.11g               */
	WIFI_MANAGER_IEEE_80211_N,		/**<  IEEE 802.11n               */
	WIFI_MANAGER_IEEE_80211_AC,		/**<  IEEE 802.11ac              */
	WIFI_MANAGER_NOT_SUPPORTED,		/**<  Driver does not report     */
} wifi_manager_standard_type_e;

/**
 * @brief Status of Wi-Fi interface such as connected or disconnected
 */
typedef enum {
	// STA mode status
	AP_DISCONNECTED,
	AP_CONNECTED,
	AP_RECONNECTING,

	// SOFT AP mode status
	CLIENT_CONNECTED,
	CLIENT_DISCONNECTED,

	// Unkown
	STATUS_UNKNOWN
} connect_status_e;

/**
 * @brief Result types of Wi-Fi Manager APIs such as FAIL, SUCCESS, or INVALID ARGS
 */
typedef enum {
	WIFI_MANAGER_FAIL = -1,
	WIFI_MANAGER_SUCCESS,
	WIFI_MANAGER_INVALID_ARGS,
	WIFI_MANAGER_INITIALIZED,
	WIFI_MANAGER_DEINITIALIZED,
	WIFI_MANAGER_TIMEOUT,
	WIFI_MANAGER_BUSY,
	WIFI_MANAGER_ALREADY_CONNECTED,
	WIFI_MANAGER_CALLBACK_NOT_REGISTERED,
	WIFI_MANAGER_NOT_AVAILABLE,
	WIFI_MANAGER_DISCONNECT,
	WIFI_MANAGER_RECONNECT, // AUTOCONNECT
	WIFI_MANAGER_NO_API,
	WIFI_MANAGER_POST_FAIL,
} wifi_manager_result_e;

#undef STA_MODE

/**
 * @brief Mode of Wi-Fi interface such as station mode or ap mode
 */
typedef enum {
	WIFI_NONE = -1,
	STA_MODE,
	SOFTAP_MODE,
	BRIDGE_MODE,
	WIFI_MODE_CHANGING,
	WIFI_INITIALIZING,
	WIFI_DEINITIALIZING,
	WIFI_FAILURE
} wifi_manager_mode_e;

/**
 * @brief Reconnection mode of Wi-Fi interface
 */
typedef enum {
	WIFI_RECONN_INTERVAL,
	WIFI_RECONN_EXPONENT,
	WIFI_RECONN_NONE,
} wifi_manager_reconn_type_e;

/**
 * @brief Wi-Fi authentication type such as WPA, WPA2, or WPS
 */
typedef enum {
	WIFI_MANAGER_AUTH_OPEN,					/**<  open mode                                 */
	WIFI_MANAGER_AUTH_WEP_SHARED,			/**<  use shared key (wep key)                  */
	WIFI_MANAGER_AUTH_WPA_PSK,				/**<  WPA_PSK mode                              */
	WIFI_MANAGER_AUTH_WPA2_PSK,				/**<  WPA2_PSK mode                             */
	WIFI_MANAGER_AUTH_WPA3_PSK,				/**<  WPA3_PSK mode                             */
	WIFI_MANAGER_AUTH_WPA_AND_WPA2_PSK,		/**<  WPA_PSK and WPA_PSK mixed mode            */
	WIFI_MANAGER_AUTH_WPA2_AND_WPA3_PSK,	/**<  WPA_PSK and WPA_PSK mixed mode            */
	WIFI_MANAGER_AUTH_WPA_PSK_ENT,			/**<  Enterprise WPA_PSK mode                   */
	WIFI_MANAGER_AUTH_WPA2_PSK_ENT,			/**<  Enterprise WPA2_PSK mode                  */
	WIFI_MANAGER_AUTH_WPA_AND_WPA2_PSK_ENT, /**<  Enterprise WPA_PSK and WPA_PSK mixed mode */
	WIFI_MANAGER_AUTH_IBSS_OPEN,			/**<  IBSS ad-hoc mode                          */
	WIFI_MANAGER_AUTH_WPS,					/**<  WPS mode                                  */
	WIFI_MANAGER_AUTH_UNKNOWN,				/**<  unknown type                              */
} wifi_manager_ap_auth_type_e;

/**
 * @brief Wi-Fi encryption type such as WEP, AES, or TKIP
 */
typedef enum {
	WIFI_MANAGER_CRYPTO_NONE,			  /**<  none encryption                           */
	WIFI_MANAGER_CRYPTO_WEP_64,			  /**<  WEP encryption wep-40                     */
	WIFI_MANAGER_CRYPTO_WEP_128,		  /**<  WEP encryption wep-104                    */
	WIFI_MANAGER_CRYPTO_AES,			  /**<  AES encryption                            */
	WIFI_MANAGER_CRYPTO_TKIP,			  /**<  TKIP encryption                           */
	WIFI_MANAGER_CRYPTO_TKIP_AND_AES,	  /**<  TKIP and AES mixed encryption             */
	WIFI_MANAGER_CRYPTO_AES_ENT,		  /**<  Enterprise AES encryption                 */
	WIFI_MANAGER_CRYPTO_TKIP_ENT,		  /**<  Enterprise TKIP encryption                */
	WIFI_MANAGER_CRYPTO_TKIP_AND_AES_ENT, /**<  Enterprise TKIP and AES mixed encryption  */
	WIFI_MANAGER_CRYPTO_UNKNOWN,		  /**<  unknown encryption                        */
} wifi_manager_ap_crypto_type_e;

/**
 * @brief Keep information of nearby access points as scan results
 */
struct wifi_manager_scan_info_s {
	char ssid[WIFIMGR_SSID_LEN + 1];			  // 802.11 spec defined unspecified or uint8
	char bssid[WIFIMGR_MACADDR_STR_LEN + 1];	  // char string e.g. xx:xx:xx:xx:xx:xx
	int8_t rssi;								  // received signal strength indication
	uint8_t channel;							  // channel/frequency
	wifi_manager_standard_type_e phy_mode;		  /**< Supported MAC/PHY standard                              */
	wifi_manager_ap_auth_type_e ap_auth_type;	  /**<  @ref wifi_utils_ap_auth_type   */
	wifi_manager_ap_crypto_type_e ap_crypto_type; /**<  @ref wifi_utils_ap_crypto_type */
	struct wifi_manager_scan_info_s *next;
};

typedef struct wifi_manager_scan_info_s wifi_manager_scan_info_s;

struct wifi_manager_cb_msg {
	wifi_manager_result_e res;
	uint32_t reason;
	uint8_t bssid[6];
	wifi_manager_scan_info_s *scanlist;
};
typedef struct wifi_manager_cb_msg wifi_manager_cb_msg_s;

/**
 * @brief Include callback functions which are asynchronously called after Wi-Fi Manager APIs are called
 */
typedef struct {
	// in station mode, connected to ap
	void (*sta_connected)(wifi_manager_cb_msg_s msg, void *arg);
	// in station mode, disconnected from ap
	void (*sta_disconnected)(wifi_manager_cb_msg_s msg, void *arg);
	// in softap mode, a station joined
	void (*softap_sta_joined)(wifi_manager_cb_msg_s msg, void *arg);
	// in softap mode, a station left
	void (*softap_sta_left)(wifi_manager_cb_msg_s msg, void *arg);
	// scanning ap is done
	void (*scan_ap_done)(wifi_manager_cb_msg_s msg, void *arg);
} wifi_manager_cb_s;

typedef enum {
	WIFI_NORMAL = 0,
	WIFI_ANDROID_MOBILE_HOTSPOT = 3,
	WIFI_HOMELYNK = 9,
} wifi_manager_ap_type_e;

/**
 * @brief Keep Wi-Fi Manager information including ip/mac address, ssid, rssi, etc.
 */
typedef struct {
	char ssid[WIFIMGR_SSID_LEN + 1];
	int rssi;
	connect_status_e status;
	wifi_manager_mode_e mode;
	char bssid[WIFIMGR_MACADDR_LEN]; /**< bssid is a mac address of AP */
} wifi_manager_info_s;

typedef struct {
	unsigned int channel;
	signed char snr;
	int network_bw;
	unsigned int max_rate;
} wifi_manager_signal_quality_s;

typedef struct {
	int wpa_supplicant_state;
	int wpa_supplicant_key_mgmt;
} wifi_manager_wpa_states_s;

typedef struct {
	char lib_version[64];
} wifi_manager_driver_info_s;

/**
 * @brief Specify information of soft access point (softAP) such as ssid and channel number
 */
typedef struct {
	char ssid[WIFIMGR_SSID_LEN + 1];
	char passphrase[WIFIMGR_PASSPHRASE_LEN + 1];
	uint16_t channel;
} wifi_manager_softap_config_s;

/**
 * @brief Specify the policy of reconnect when the device is disconnected
 */
typedef struct {
	wifi_manager_reconn_type_e type;
	// interval: if type is INTERVAL, it will try to connect AP every interval second
	//           if type is EXPONENTIAL, it is initial wait time.
	int interval;
	// max_interval: it is the maximum wait time if type is EXPONENTIAL
	//             : it is not used if type is INTERVAL
	int max_interval;
} wifi_manager_reconnect_config_s;

/**
 * @brief Specify which access point (AP) a client connects to
 */
typedef struct {
	char ssid[WIFIMGR_SSID_LEN + 1];			  /**<  Service Set Identification         */
	unsigned int ssid_length;					  /**<  Service Set Identification Length  */
	char passphrase[WIFIMGR_PASSPHRASE_LEN + 1];  /**<  ap passphrase(password)            */
	unsigned int passphrase_length;				  /**<  ap passphrase length               */
	wifi_manager_ap_auth_type_e ap_auth_type;	  /**<  @ref wifi_utils_ap_auth_type   */
	wifi_manager_ap_crypto_type_e ap_crypto_type; /**<  @ref wifi_utils_ap_crypto_type */
} wifi_manager_ap_config_s;

typedef struct {
	unsigned int channel;			 /**<  wifi channel, =0 is full channel scan */
	char ssid[WIFIMGR_SSID_LEN + 1]; /**<  Service Set Identification            */
	unsigned int ssid_length;		 /**<  Service Set Identification Length     */
} wifi_manager_scan_config_s;

typedef struct {
	wifi_manager_scan_config_s ap_configs[WIFIMGR_SPECIFIC_SCAN_AP_CNT]; /**< The AP configurations to scan */
	unsigned int scan_ap_config_count;									 /**< The count of AP to scan       */
	bool scan_all;														 /**<  Flag to enable scanning specific AP + other APs responding to NULL probe req  */
} wifi_manager_scan_multi_configs_s;

/**
 * @brief Specify Wi-Fi Manager internal stats information
 */
typedef struct {
	uint16_t connect;
	uint16_t connectfail;
	uint16_t disconnect;
	uint16_t reconnect;
	uint16_t joined;
	uint16_t left;
	uint16_t scan;
	uint16_t softap;
	/*
	 * driver stats
	 * a driver collects data periodically
	 */
	struct timeval start;
	struct timeval end;
	uint32_t tx_retransmit;
	uint32_t tx_drop;
	uint32_t rx_drop;
	uint32_t tx_success_cnt;
	uint32_t tx_success_bytes;
	uint32_t rx_cnt;
	uint32_t rx_bytes;
	uint32_t tx_try;
	uint32_t rssi_avg;
	uint32_t rssi_min;
	uint32_t rssi_max;
	uint32_t beacon_miss_cnt;
} wifi_manager_stats_s;

typedef enum {
	WIFI_MANAGER_POWERMODE_DISABLE,
	WIFI_MANAGER_POWERMODE_ENABLE,
} wifi_manager_powermode_e;

#if defined(CONFIG_ENABLE_HOMELYNK) && (CONFIG_ENABLE_HOMELYNK == 1)
/**
 * @brief Specify information of bridge mode on/off and soft access point (softAP)
 */

typedef struct {
	bool enable;
	wifi_manager_softap_config_s softap_config;
} wifi_manager_bridge_config_s;
#endif

/**
 * @brief Initialize Wi-Fi Manager including starting Wi-Fi interface.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] wmcb callback functions called when wi-fi events happen
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API_type: synchronous
 * @callback: none
 * @since TizenRT v1.1
 */
wifi_manager_result_e wifi_manager_init(wifi_manager_cb_s *wmcb);

/**
 * @brief Deinitialize Wi-Fi Manager including stoping Wi-Fi interface.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] none
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API_type: synchronous
 * @callback: none
 * @since TizenRT v1.1
 */
wifi_manager_result_e wifi_manager_deinit(void);

/**
 * @brief Register the callback to receive wi-fi events
 * @details @b #include <wifi_manager/wifi_manager.h>
 *          if it fails to register the callbacks with result WIFI_MANAGER_DEINITIALIZED
 *          then it have to call wifi_manager_init to run Wi-Fi
 * @param[in] wmcb callback functions called when wi-fi events happen
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v2.0
 */

wifi_manager_result_e wifi_manager_register_cb(wifi_manager_cb_s *wmcb);

/**
 * @brief Unregister the callback not to receive wi-fi events
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] wmcb callback functions are used to find registered callback
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v2.0
 */
wifi_manager_result_e wifi_manager_unregister_cb(wifi_manager_cb_s *wmcb);

/**
 * @brief Change the Wi-Fi mode to station or AP.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] mode Wi-Fi mode (station or AP)
 * @param[in] config In case of AP mode, AP configuration information should be given including ssid, channel, and passphrase.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v1.1
 */
wifi_manager_result_e wifi_manager_set_mode(wifi_manager_mode_e mode, wifi_manager_softap_config_s *config);

/**
 * @brief Retrieve current status of Wi-Fi interface including mode, connection status, ssid, received signal strengh indication, and ip address.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[out] retrieved information including  mode, connection status, ssid, received signal strengh indication, and ip address.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v1.1
 */
wifi_manager_result_e wifi_manager_get_info(wifi_manager_info_s *info);

/**
 * @brief Retrieve currently connected ap type.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[out] retrieved information of currently connected ap type. Normal type : 0, android mobile hotspot : 3, homelynk : 9.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v3.1
 */
wifi_manager_result_e wifi_manager_get_ap_type(wifi_manager_ap_type_e *aptype);

/**
 * @brief Change the Wi-Fi channel plan
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] channel_plan channel plan for 5G
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchoronous
 * @callback: none
 * @since Tizen Lite 5.0
 */
wifi_manager_result_e wifi_manager_set_channel_plan(uint8_t channel_plan);

/**
 * @brief Retrieve the signal quality.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[out] retrieved information including snr, network bandwidth, and max rate.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT 3.1
 *
 */
wifi_manager_result_e wifi_manager_get_signal_quality(wifi_manager_signal_quality_s *wifi_manager_signal_quality);

/**
 * @brief Retrieve the disconnection_reason code from the last disconnection event.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[out] retrieved information of the disconnection_reason code defined in IEEE std 802.11-2016 Table 8-71.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v3.1
 */
wifi_manager_result_e wifi_manager_get_disconnect_reason(int *disconnect_reason);

/**
 * @brief Retrieve the Wi-Fi driver information such as driver version.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[out] retrieved information of driver version.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v3.1
 */
wifi_manager_result_e wifi_manager_get_driver_info(wifi_manager_driver_info_s *wifi_manager_driver_info);

/**
 * @brief Retrieve the current WPA supplicant state of Wi-Fi interface.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[out] retrieved The WPA supplicant state and the key mgmt value just before the disconnection of Wi-Fi interface.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v3.1
 */
wifi_manager_result_e wifi_manager_get_wpa_supplicant_state(wifi_manager_wpa_states_s *wifi_manager_wpa_state);

/**
 * @brief set host name using lwip api
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] hostname hostname string (const)
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v3.1
 */
wifi_manager_result_e wifi_manager_set_hostname_sta(const char *hostname);

/**
 * @brief Connect to an access point.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] config ssid, passphrase, authentication type, and crypto type of the access point which the wi-fi interface connect to.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: asynchronous
 * @callback: sta_connected
 * @since TizenRT v1.1
 */
wifi_manager_result_e wifi_manager_connect_ap(wifi_manager_ap_config_s *config);

/**
 * @brief Disconnect from the connected access point
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] none
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: asynchronous
 * @callback: sta_disconnected
 * @since TizenRT v1.1
 */
wifi_manager_result_e wifi_manager_disconnect_ap(void);

/**
 * @brief Scan nearby access points
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] none
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API_type: asynchronous
 * @callback: scan_ap_done
 * @since TizenRT v1.1
 */
/* SCAN type
 *     1) full scan: config is null
 *     2) scan with specific SSID: ssid is set in config
 *     3) scan with specific channel: channel is set in config
 *     4) scan with specific SSID and channel: Both ssid and channel are set in config
 *
 * Notes
 *     if both SSID and channel are not set in config then it'll return error.
 *     wifi_manager check whether SSID is set by ssid_length in config. In the same way
 *     not checking channel is 0, but checking channel is greater than 0 and less than 15 in 2.5GHz.
 *     So if both channel and ssid_length are 0 then wifi_manager returns WIFI_MANAGER_INVALID_ARGS.
 *     To request full scan to wifi_manager, config which is assigned null be pass to wifi_manager.
 */
wifi_manager_result_e wifi_manager_scan_ap(wifi_manager_scan_config_s *config);

/**
 * @brief Scan nearby access points
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] config Required AP configuration, which SSID should be given.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: asynchronous
 * @callback: scan_ap_done
 * @since TizenRT v3.0
 * @deprecated TizenRT v3.1
 */
wifi_manager_result_e wifi_manager_scan_specific_ap(wifi_manager_ap_config_s *config);

/**
 * @brief Scan nearby access points
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] configs Required a number of scan configurations with its count, See wifi_manager_scan_ap for more details.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: asynchronous
 * @callback: scan_ap_done
 * @since TizenRT v4.1
 */
wifi_manager_result_e wifi_manager_scan_multi_aps(wifi_manager_scan_multi_configs_s *configs);

/**
 * @brief Save the AP configuration at persistent storage
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] config AP configuration information should be given including ssid, channel, and passphrase.
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v2.0
 */
wifi_manager_result_e wifi_manager_save_config(wifi_manager_ap_config_s *config);

/**
 * @brief Get the AP configuration which was saved
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] config The pointer of AP configuration information which will be filled
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v2.0
 */
wifi_manager_result_e wifi_manager_get_config(wifi_manager_ap_config_s *config);

/**
 * @brief Remove the AP configuration which was saved
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v2.0
 */
wifi_manager_result_e wifi_manager_remove_config(void);

/**
 * @brief Get the most recently connected AP configuration which was saved by Wi-Fi Manager
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] config The pointer of AP configuration infomation which will be filled
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v2.0
 */
wifi_manager_result_e wifi_manager_get_connected_config(wifi_manager_ap_config_s *config);

/**
 * @brief Obtain WiFi Manager state stats
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] The pointer of WiFi Manager stats information which will be filled
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v2.0
 */
wifi_manager_result_e wifi_manager_get_stats(wifi_manager_stats_s *stats);

/**
 * @brief Obtain WiFi Manager state stats
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in] power save mode
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API type: synchronous
 * @callback: none
 * @since TizenRT v3.1
 */
wifi_manager_result_e wifi_manager_set_powermode(wifi_manager_powermode_e mode);

#if defined(CONFIG_ENABLE_HOMELYNK) && (CONFIG_ENABLE_HOMELYNK == 1)
/**
 * @brief Start Wi-Fi Manager bridge mode.
 * @details @b #include <wifi_manager/wifi_manager.h>
 * @param[in]  enable : true, disable : false
 * @return On success, WIFI_MANAGER_SUCCESS (i.e., 0) is returned. On failure, non-zero value is returned.
 * @API_type: synchronous
 * @callback: none
 * @since TizenRT v1.1
 */
wifi_manager_result_e wifi_manager_control_bridge(bool enable, wifi_manager_softap_config_s *softap_config);
#endif

#ifdef __cplusplus
}
#endif
/**
 *@}
 */
