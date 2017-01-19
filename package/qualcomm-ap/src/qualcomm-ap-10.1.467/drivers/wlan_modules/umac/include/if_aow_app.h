/*
 * Copyright (c) 2013 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */

/*
 * =====================================================================================
 *
 *       Filename:  if_aow_app.h
 *
 *    Description:  AoW Header File, for data shared between driver, app and host app
 *
 *        Version:  1.0
 *        Created:  03/03/2011 02:04:38 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  S.Karthikeyan (), 
 *        Company:  Atheros Communication
 *
 * =====================================================================================
 */

#ifndef IF_AOW_APP_H
#define IF_AOW_APP_H

#ifndef __packed
#define __packed    __attribute__((__packed__))
#endif

#define AOW_VERSION_MAJOR_NUM   0
#define AOW_VERSION_MINOR_NUM   4
#define AOW_VERSION_MAJOR_NUM_OFFSET    8
#define AOW_RELEASE_VERSION_OFFSET 16
#define AOW_VOLUME_DATA_LENGTH 8
#define AOW_MAX_HOST_PKT_SIZE   512
#define AOW_MAX_AUDIO_CHANNELS      (4)

typedef struct volume_data {
    u_int8_t info[8];
}volume_data_t;    

typedef struct ch_volume_data {
    volume_data_t ch[AOW_MAX_AUDIO_CHANNELS];
    u_int32_t     volume_flag;
}ch_volume_data_t;    

typedef enum aow_verion{
    AOW_DEBUG_VERSION,
    AOW_RELEASE_VERSION,
}aow_verion_t;    

typedef enum STA_JOIN_STATE {
    AOW_STA_CONNECTED,
    AOW_STA_DISCONNECTED,
}STA_JOIN_STATE_T;    

typedef struct sta_join_info {
    STA_JOIN_STATE_T state;
    u_int8_t addr[6];
}sta_join_info_t;

typedef enum AOW_HOST_PKT_TYPE {
    AOW_HOST_PKT_DATA,
    AOW_HOST_PKT_CTRL,
    AOW_HOST_PKT_EVENT,
}AOW_HOST_PKT_TYPE_T;    

typedef enum aow_cm_event {
    CM_CHANNEL_ADDRESS_SET_PASS = 22,
    CM_CHANNEL_ADDRESS_SET_FAIL,
    CM_SET_MEMORY_PASS,
    CM_SET_MEMORY_FAIL,
    CM_GET_MEMORY_PASS,
    CM_GET_MEMORY_FAIL,
    CM_REQUEST_VERSION_PASS,
    CM_REQUEST_VERSION_FAIL,
    CM_SEND_BUFFER_STATUS_PASS,
    CM_SEND_BUFFER_STATUS_FAIL,
    CM_STREAM_CHANGE_INDICATION,
    CM_UPDATE_VOLUME_PASS,
    CM_UPDATE_VOLUME_FAIL,
    CM_UPDATE_IE_PASS,
    CM_UPDATE_IE_FAIL,
    CM_VOLUME_CHANGE_INDICATION,
    CM_INIT_CONFIRMATION_PASS,
    CM_INIT_CONFIRMATION_FAIL,
    CM_START_SAIR_CONFIRMATION_PASS,
    CM_START_SAIR_CONFIRMATION_FAIL,
    CM_UPDATE_SECURITY_MODE_CONFIRMATION_PASS,
    CM_UPDATE_SECURITY_MODE_CONFIRMATION_FAIL,
    CM_CHANGE_SEURITY_MODE_CONFIRMATION_PASS,
    CM_CHANGE_SECURITY_MODE_CONFIRMATION_FAIL,
    CM_STA_CONNECTED,
    CM_STA_DISCONNECTED,
    CM_CHANGE_NW_ID_PASS,
    CM_CHANGE_NW_ID_FAIL,
    CM_STOP_NW_PASS,
    CM_STOP_NW_FAIL,
    CM_ADD_IE_PASS,
    CM_ADD_IE_FAIL,
}aow_cm_event_t;

typedef struct aow_ie {
    u_int8_t elemid;
    u_int8_t len;
    u_int8_t oui[3];
    u_int8_t oui_type;
    u_int8_t version;
    u_int16_t id;
    u_int8_t dev_type;
    u_int8_t audio_type;
    u_int8_t party_mode;
}__packed aow_ie_t;        

#define AOW_IE_ELEMID   0xdd
#define AOW_IE_LENGTH   (sizeof(aow_ie_t) - 2)
#define AOW_OUI                     0x4a0100    /* AoW OUI, workaround */
#define AOW_OUI_TYPE                    0x01
#define AOW_OUI_VERSION                 0x01

struct CM_INIT_SAIR_CONF {
    u_int32_t status;
};    

struct CM_START_SAIR_CONF {
    u_int32_t freq;
};

struct CM_STOP_SAIR_CONF {
    u_int32_t status;
};

struct CM_CONNECTED_IND {
    u_int32_t device_index;
    u_int32_t freq;
    u_int8_t addr[6];
};

struct CM_DISCONNECTED_IND {
    u_int32_t device_index;
    u_int8_t addr[6];
};

struct CM_BDV_NOT_FOUND_IND {
    u_int32_t status;
};

struct CM_CONNECTION_FAIL_IND {
    u_int32_t status;
};

struct CM_UPDATE_SEC_MODE_CNF {
    u_int32_t status;
};

struct CM_CHANGE_SEC_MODE_CNF {
    u_int32_t status;
    u_int8_t  pass_or_psk[64];
};

struct CM_CURRENT_STATE_IND {
    u_int32_t status;
};

struct CM_LINK_QUALITY_IND {
    u_int32_t link_quality;
};

struct CM_STATE_UPDATE_IND {
    u_int32_t status;
};    

struct CM_CHANGE_SAIR_CNF {
    u_int32_t status;
    u_int32_t network_id;
};    

typedef struct aow_eventdata {
    int event;
    union {
        struct CM_INIT_SAIR_CONF        m_INIT_SAIR_CONF;
        struct CM_START_SAIR_CONF       m_START_SAIR_CONF;
        struct CM_STOP_SAIR_CONF        m_STOP_SAIR_CONF;
        struct CM_CONNECTED_IND         m_CONNECTED_IND;
        struct CM_DISCONNECTED_IND      m_DISCONNECTED_IND;
        struct CM_BDV_NOT_FOUND_IND     m_BDV_NOT_FOUND_IND;
        struct CM_CONNECTION_FAIL_IND   m_CONNECTION_FAIL_IND;
        struct CM_UPDATE_SEC_MODE_CNF   m_UPDATE_SEC_MODE_CNF;
        struct CM_CHANGE_SEC_MODE_CNF   m_CHANGE_SEC_MODE_CNF;
        struct CM_CURRENT_STATE_IND     m_CURRENT_STATE_IND;
        struct CM_LINK_QUALITY_IND      m_LINK_QUALITY_IND;
        struct CM_STATE_UPDATE_IND      m_STATE_UPDATE_IND;
        struct CM_CHANGE_SAIR_CNF       m_CHANGE_SAIR_CNF;
    }u;
}aow_eventdata_t;    

#endif  /* IF_AOW_APP_H */
