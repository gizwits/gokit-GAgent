
#include "gagent.h"
#include "lib.h"
#include "Wifi_mode.h"
#include "mqttxpg.h"
#include "wifi.h"
#include "mqtt.h"
#include "http.h"

#include "iof_import.h"
#include "gagent_login_cloud.h"

/************************************************/
short wifiStatus=0;

void GAgent_WiFiStatusCheckTimer(void)
{
    if((wifiStatus & WIFI_MODE_STATION) == WIFI_MODE_STATION
       && (wifiStatus & WIFI_STATION_STATUS) != WIFI_STATION_STATUS)
    {
        GAgent_Printf(GAGENT_LOG,"GAgent_WiFiStatusCheckTimer , Working at AP mode(%x)\r\n", wifiStatus);
        wifiStatus = WIFI_MODE_AP;
        DRV_WiFi_SoftAPModeStart();
    }
    return;
}

void handleWiFiEvent(void)
{

	/* wifi状态有变化后通知MCU */
    if( wifiStatus != g_Xpg_GlobalVar.lastWifiStatus )
    {                   
        short tempWifiStatus; 
        g_Xpg_GlobalVar.lastWifiStatus = wifiStatus;
        tempWifiStatus= htons( wifiStatus );        
        GAgent_Printf(GAGENT_INFO," 3.wifiStatus:%04X [cloud:%d]",wifiStatus, g_ConCloud_Status != CONCLOUD_RUNNING);        
        GAgentV4_Write2Mcu_with_p0( 0,WIFI_STATUS2MCU,(u8*)&tempWifiStatus,2);  
    } 

    return;
}

void GAgent_InitEthernet(void)
{
    /*设置相关状态*/
    wifiStatus = wifiStatus | WIFI_STATION_STATUS | WIFI_MODE_STATION;
    return;
}



/* 如果有SSID和KEY, 以Station模式运行，并连接到指定的SSID;
 * 否则作为AP模式运行；
 */
void Connect2LateWiFi()
{
	if ((g_stGAgentConfigData.flag & XPG_CFG_FLAG_CONNECTED) == XPG_CFG_FLAG_CONNECTED)//flash 里面有要加入的网络名称
	{
	    wifiStatus = WIFI_MODE_STATION;
        GAgent_Printf(GAGENT_INFO,"try to connect wifi...[%08x]", g_stGAgentConfigData.flag);
		GAgent_Printf(GAGENT_INFO,"SSID: %s",g_stGAgentConfigData.wifi_ssid);				
        GAgent_Printf(GAGENT_INFO,"KEY: %s",g_stGAgentConfigData.wifi_key);						
		DRV_WiFi_StationCustomModeStart(g_stGAgentConfigData.wifi_ssid,g_stGAgentConfigData.wifi_key);

	}
	else
	{
	    wifiStatus = WIFI_MODE_AP;
		GAgent_Printf(GAGENT_INFO,"Connect2LateWiFi, Working at AP mode\r\n");
        DRV_WiFi_SoftAPModeStart();
	}

    return;
}
