#ifndef __GAGENT_H_
#define __GAGENT_H_

#include "platform.h"

#define GAGENT_MAGIC_NUMBER 0x12345678

#define GAGENT_DID_LEN_MAX          24
#define GAGENT_DID_LEN              22
/* XPG GAgent Global Config data*/
typedef struct GAGENT_CONFIG {
#if (GAGENT_FEATURE_OTA == 1)
    /*OTA options*/
  boot_table_t bootTable;
#endif
  unsigned int magicNumber;
  unsigned int flag;
  char wifipasscode[16]; /* gagent passcode */
  char wifi_ssid[32]; /* WiFi AP SSID */
  char wifi_key[32]; /* AP key */
    char FirmwareId[8];  /* Firmware ID,identity application version */
    char Cloud_DId[GAGENT_DID_LEN_MAX]; /* Device, generate by server, unique for devices */
  char FirmwareVerLen[2];
  char FirmwareVer[32];
}GAgent_CONFIG_S;

// Upgrade data struct
typedef struct _xpgupgrade_t {  
  unsigned char XPG_version[4];
  unsigned char XPG_cmd[2];
  unsigned char XPG_FirmwareId[8];
  unsigned char XPG_FirmwareVerLen[2];
  unsigned char XPG_FirmwareVer[32];   
  unsigned char XPG_md5[16];  
  unsigned int XPG_FirmwareBinlen;
  unsigned char *pXPG_FirmwareBin;// 
} xpgota_upgrate_t;


typedef struct _XPG_MCU
{
    //XPG_Ping_McuTick
    unsigned int XPG_PingTime;
    char loseTime;
    //8+1('\0') for printf.
    unsigned char   protocol_ver[8+1];
    unsigned char   p0_ver[8+1];
    unsigned char   hard_ver[8+1];
    unsigned char   soft_ver[8+1];    
    unsigned char   product_key[32+1];    
    unsigned short  passcodeEnableTime;

}XPG_MCU;

typedef struct  _XPG_GLOBALVAR
{
    XPG_MCU Xpg_Mcu;
    short lastWifiStatus;
    char connect2Cloud;
    unsigned int connect2CloudLastTime;
    unsigned int send2HttpLastTime;
    unsigned char logSwitch[2];
    char m2m_SERVER[100];    
    int m2m_Port;
    char a_mac[16];
    int http_socketid;
    char http_sockettype;
    char DID[GAGENT_DID_LEN_MAX];    
    char phoneClientId[32];

    /*From V3*/
    char ReConnect2CloudTime;
    char AirLinkFlag;
    char M2Mip[17];
    char M2Mip_Flag;    
    char HTTPip[17];
    char HTTPip_Flag;
    
    int RefreshM2MipTime;
    char wifiStrength;
    int cloudSocketFlag;
    int httpSocketFlag;
    
}XPG_GLOBALVAR;


#define ONE_SECOND  1000
#define ONE_MINUTE  60*ONE_SECOND
#define ONE_HOUR    60*ONE_MINUTE

/* GAgent_CONFIG_S flag */
#define XPG_CFG_FLAG_CONNECTED   0X00000001 /* 脫脙脌麓卤锚脢露脢脟路帽脕卢陆脫wifi */
#define XPG_CFG_FLAG_CHANGEPW    0X00000002 /* 脨猫脪陋赂眉脨脗passcode */


#define XPG_WIFI_SCAN_TIME        3*ONE_MINUTE
#define XPG_WIFI_STATUS_TIME      10*ONE_MINUTE

#define WIFI_LEVEL_0                0
#define WIFI_LEVEL_1                20
#define WIFI_LEVEL_2                40
#define WIFI_LEVEL_3                50
#define WIFI_LEVEL_4                60
#define WIFI_LEVEL_5                70
#define WIFI_LEVEL_6                80

extern GAgent_CONFIG_S g_stGAgentConfigData;

void GAgent_Init(void);
void GAgent_DoBusiness(void);

extern void GAgent_SaveSSIDAndKey(char *pssid, char *pkey);

#define HTTP_SERVER         "api.gizwits.com"

/*For GAgent Defined SoftAP*/
#define AP_NAME             "XPG-GAgent"
#define AP_PASSWORD         "123456789"
#define AP_LOCAL_IP         "192.168.1.2"
#define AP_NET_MASK         "255.255.255.0"
#define ADDRESS_POOL_START  "192.168.1.10"
#define ADDRESS_POOL_END    "192.168.1.110"

#define WIFI_MODE_AP            (1<<0)
#define WIFI_MODE_STATION       (1<<1)
#define WIFI_MODE_ONBOARDING    (1<<2)
#define WIFI_MODE_BINDING       (1<<3)
#define WIFI_STATION_STATUS     (1<<4)

/* CMD of P0 */
#define MCU_INFO_CMD        0X01
#define MCU_INFO_CMD_ACK    0X02

#define MCU_CTRL_CMD        0X03
#define MCU_CTRL_CMD_ACK    0X04

#define MCU_REPORT          0X05
#define MCU_REPORT_ACK      0X06

#define WIFI_PING2MCU       0X07
#define WIFI_PING2MCU_ACK   0X08

#define MCU_CONFIG_WIFI     0X09
#define MCU_CONFIG_WIFI_ACK 0X0A

#define MCU_RESET_WIFI      0X0B
#define MCU_RESET_WIFI_ACK  0X0C 

#define WIFI_STATUS2MCU     0X0D
#define WIFI_STATUS2MCU_ACK 0X0E


#define MCU_LEN_POS            2
#define MCU_CMD_POS            4
#define MCU_SN_POS             5
#define MCU_HDR_LEN_NO_PIECE   8
#define MCU_LEN_NO_PIECE       9
#define MCU_HDR_FF             0xFF
#define MCU_NO_PIECE_P0_POS     8
#define MCU_BYTES_NO_SUM        3


/*Gizwits heartbeat with eath others, Cloud, SDK/Demo, GAgent and MCU*/
#define MCU_HEARTBEAT           55
#define LAN_HEARTBEAT           1
#define MQTT_HEARTBEAT          50
#define HTTP_TIMEOUT            60

/*For V4, GAgent waiting for MCU response of very CMD send by GAgent, Unit: ms*/
#define MCU_ACK_TIME    200

extern XPG_GLOBALVAR g_Xpg_GlobalVar;   


#define GAGENT_ERROR       0X01
#define GAGENT_WARNING     0X02
#define GAGENT_INFO        0X03
#define GAGENT_CRITICAL    0x00
#define GAGENT_LOG         0x05
#define GAGENT_DEBUG       0x10
#define GAGENT_PACKETDUMP  0x11
#define GAGENT_DUMP        0x12
#define GAGENT_DEBUG_PACKET      0x13


#define GAGENT_ERROR_BIT   (1)
#define GAGENT_WARNING_BIT (1<<1)
#define GAGENT_INFO_BIT    (1<<2)    
#define GAGENT_LED_BIT     (1<<3)

#define GAGENT_DEBUG_PACKET_SEND2MCU    0X01

/* zhou mask for debug */
#if 0
void GAgent_Printf(unsigned char level, char *fmt, ...);
#else
#define GAgent_Printf(level, fmt, args...)  printf(fmt, ##args);printf("\r\n")
#endif
void GAgent_DebugPacket(unsigned char *pData, int len);

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

void GAgent_DebugDisable(void);
void GAgent_DebugEnable(void);


extern unsigned char Service_mac[6];

#undef xpg_assert

//#define NDEBUG

#ifdef NDEBUG

#define xpg_assert(test) ((void)0)
#else
#define _STR(x) _VAL(x)
#define _VAL(x) #x
#define xpg_assert(test) ((test) ? (void)0 :GAgent_Printf(GAGENT_DEBUG, __FILE__ ":" _STR(__LINE__) ":" _STR(__FUNC__) " " #test))
#endif

#define AS //xpg_assert(0)
#define TS (GAgent_Printf(GAGENT_LOG, __FILE__ ":" _STR(__LINE__) ":" _STR(__FUNC__) "Flag:%x,MN:%x ...", g_stGAgentConfigData.flag, g_stGAgentConfigData.magicNumber))
#define MS //(GAgent_Printf(GAGENT_LOG, __FILE__ ":" _STR(__LINE__) ":" _STR(__FUNC__) "TOP Level Error:malloc"))

char *GAgent_GetVersion(void);
extern int DIdLen;

extern short wifiStatus;
extern int g_productKeyLen;


void GAgent_SendAPStatusTimer(void);

#endif

