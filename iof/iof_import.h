/*******************************************************************
 *******************************************************************
 ***creat new file
 ***LIB EXPORT FUNCS FOR IOF
 * 该文件用于申明LIB定义的函数钩子。在IOF中进行挂接
 * 该文件由IOF引用
 * 由于历史原因，这一版暂时不改函数变量名
 *******************************************************************
 ******************************************************************/
#ifndef IOF_IMPORT_H
#define IOF_IMPORT_H
#include <sys/wait.h>

#include "gagent.h"


/*****************************************************************
 *****************************************************************
 ********* hardware adapter function variable
 *****************************************************************
 ****************************************************************/

/* 复位WIFI模组 */
extern void (*DRV_GAgent_Reset)();
/* 获取WIFI MAC地址 */
extern void (*DRV_GAgent_GetWiFiMacAddress)( char *pMacAddress);

/* 保存/读取配置数据(到/从)非易失性存储器 */
extern int (*DRV_GAgent_GetConfigData)(GAgent_CONFIG_S *pConfig);
extern int (*DRV_GAgent_SaveConfigData)(GAgent_CONFIG_S *pConfig);
/* ???????? */
extern void (*DRV_GAgent_WiFiStartScan)();

/* WIFI发送/接收数据到MCU */
extern int (*pf_Gagent_Tx_To_Mcu)(char *data, int len);
extern int (*pf_Gagent_Rx_From_Mcu)(char *data, int len);

/*  */
extern int (*DRV_Led_Red)( int onoff );
extern int (*DRV_Led_Green)( int onoff );


/*****************************************************************
 *****************************************************************
 ********* functins depend on system
 *****************************************************************
 ****************************************************************/
#define msleep(ms)      usleep((unsigned long)(ms)*1000)

/* 获取系统运行时间 */
extern unsigned int (*DRV_GAgent_GetTime_S)();
extern unsigned int (*DRV_GAgent_GetTime_MS)();


#endif /* IOF_EXPORT_H */
