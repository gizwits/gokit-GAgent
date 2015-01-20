#ifndef _GAGENT_CLOUD_H_
#define _GAGENT_CLOUD_H_

#define FIRMWARE_LEN            8

typedef struct
{
    unsigned int pingTime;      /* 同mqtt 心跳计时, 计时到发送心跳包 */
    unsigned int loseTime;      /* 心跳丢失次数计数  */

    char *otaUrlHttpCont;       /* 保存ota firmware下载地址,用于HTTP get的content */
    int ota_fid;                /* ota firmware id,从云端获取  */
}GAGENT_CLOUD_TYPE;

extern GAGENT_CLOUD_TYPE Gagent_Cloud_status;

unsigned char mqtt_num_rem_len_bytes(const unsigned char *buf);
unsigned short mqtt_parse_rem_len(const unsigned char* buf);

extern void GAgent_Cloud_Timer(void);

#endif


