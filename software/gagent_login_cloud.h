#ifndef __GAGENT_LOGIN_CLOUD_H__
#define __GAGENT_LOGIN_CLOUD_H__

#define CONCLOUD_REQ_DID                1
#define CONCLOUD_REQ_DID_ACK            2
#define CONCLOUD_REQ_M2MINFO            3
#define CONCLOUD_REQ_M2MINFO_ACK        4
#define CONCLOUD_REQ_TARGETINFO         5
#define CONCLOUD_REQ_TARGETINFO_ACK     6
#define CONCLOUD_REQ_LOGIN              7
#define CONCLOUD_RUNNING                8
#define CONCLOUD_REQ_FID                9
#define CONCLOUD_REQ_FID_ACK            10
#define CONCLOUD_REQ_OTA_FILE           11
#define CONCLOUD_REQ_OTA_FILE_ACK       12
#define CONCLOUD_INVALID               (-1)

/* 云端服务器返回响应码 */
#define CLOUD_HTTP_FID_RESPON           200
#define CLOUD_HTTP_OTA_DOWN_RESPON      200

#define OTA_TAGET_WIFI                  1
#define OTA_TAGET_MCU                   2

#define MQTT_CONNECT_TIMEOUT        300

extern int g_ConCloud_Status;

#endif /* __GAGENT_LOGIN_CLOUD_H__ */
