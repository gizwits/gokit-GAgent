
#include "gagent.h"
#include "lib.h"
#include "Wifi_mode.h"
#include "wifi.h"
#include "mqttxpg.h"
#include "Socket.h"
#include "lan.h"
#include "mcu.h"
#include "http.h"
#include "iof_import.h"
#include "iof_export.h"
#include "gagent_login_cloud.h"

#define GAGENT_VERSION "B4R012D0"

GAgent_CONFIG_S g_stGAgentConfigData;
XPG_GLOBALVAR g_Xpg_GlobalVar;  
u8 Service_mac[6];


/*问题 定时器和回调 main三个处理程序的运行时机*/
/*设计说明
 * 定时器仅仅处理周期性事件
 * main函数中的while循环处理所有外部事件
 */
void GAgent_MainTimer(void)
{
    
    //DRV_MainTimer();
    return ;
}

void GAgent_Global_Status_Init(void)
{
    memset( &g_stGAgentConfigData, 0, sizeof(g_stGAgentConfigData));
    memset( &g_Xpg_GlobalVar,0,sizeof(g_Xpg_GlobalVar));

    g_Xpg_GlobalVar.connect2CloudLastTime = DRV_GAgent_GetTime_S();

    memset( &g_stMQTTBroker, 0, sizeof(g_stMQTTBroker.socketid) );
    g_stMQTTBroker.socketid = -1;
}

int GAgent_Config_Status_Init()
{
	//从存储获取保存的配置信息
	if(DRV_GAgent_GetConfigData(&g_stGAgentConfigData) == 0)
    {
        /* 判断数据是否有效 */
        if( g_stGAgentConfigData.magicNumber == GAGENT_MAGIC_NUMBER )
        {
            /* 判断DID是否有效 */
            DIdLen = strlen(g_stGAgentConfigData.Cloud_DId);
            if(DIdLen == GAGENT_DID_LEN)
            {
                /* 有效DID */
                strcpy(g_Xpg_GlobalVar.DID, g_stGAgentConfigData.Cloud_DId);
                GAgent_Printf(GAGENT_LOG, "with DID:%s,[%s] len:%d", g_Xpg_GlobalVar.DID,g_stGAgentConfigData.Cloud_DId, DIdLen);
                return 0;
            }
        }
    }

    /* 需要初始化配置信息 */
    DIdLen = 0;
    memset(&g_stGAgentConfigData, 0x0, sizeof(g_stGAgentConfigData));
    g_stGAgentConfigData.magicNumber = GAGENT_MAGIC_NUMBER;
    make_rand(g_stGAgentConfigData.wifipasscode);
    DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);

    return 1;
}

void GAgent_Init(void)
{
    int uartPacketLen=0;     
    GAgent_Printf(GAGENT_INFO,"GAgent Version: %s.", GAGENT_VERSION);
    GAgent_Printf(GAGENT_INFO,"Product Version: %s.", WIFI_HARD_VERSION);
    GAgent_Printf(GAGENT_INFO,"GAgent Compiled Time: %s, %s.\r\n",__DATE__, __TIME__);

    /*Set Gloabal varialbes*/
    GAgent_Global_Status_Init();

    if(GAgent_Config_Status_Init() == 0)
    {
        g_ConCloud_Status = CONCLOUD_REQ_M2MINFO;
        GAgent_Printf(GAGENT_LOG,"MQTT_STATUS_PROVISION");
    }
    else
    {
        g_ConCloud_Status = CONCLOUD_REQ_DID;
        GAgent_Printf(GAGENT_INFO,"MQTT_STATUS_START");
    }
    GAgent_Printf(GAGENT_INFO, "passcode:%s,len:%d", g_stGAgentConfigData.wifipasscode,strlen(g_stGAgentConfigData.wifipasscode));

	Init_Service();

	//获取网卡MAC 地址
    DRV_GAgent_GetWiFiMacAddress(Service_mac);
    sprintf(g_Xpg_GlobalVar.a_mac,"%02x%02x%02x%02x%02x%02x",Service_mac[0],Service_mac[1],Service_mac[2],
                                                 Service_mac[3],Service_mac[4],Service_mac[5]);
    g_Xpg_GlobalVar.a_mac[12]='\0';

    GAgent_Printf(GAGENT_INFO, "WiFi MAC:%s", g_Xpg_GlobalVar.a_mac);

    GAgent_GetMCUInfo( DRV_GAgent_GetTime_MS() );
    
#if (GAGENT_CONNECT_WITH_ETHERNET != 1)
    {
        DRV_GAgent_WiFiStartScan();

        //启动Wifi 连接， 根据是否获得保存数据，
	    //决定使用AP  或者Station 模式
        Connect2LateWiFi();        
    }
#else
    {
        GAgent_InitEthernet();
    }
#endif

	//初始化监听端口
    Socket_Init();      
    signal(SIGPIPE, SIG_IGN);

    GAgent_Printf(GAGENT_INFO, "GAGENT init Over, Create MainTimer now");
	//启动Wifi 处理定时器
    GAgent_CreateTimer(GAGENT_TIMER_PERIOD,1000*(3), GAgent_MainTimer);

    return;
}

/*这个函数需要完成那些功能，特别是初始化部分*/
/*需要设备g_stDefaultConfigParas*/
void GAgent_DoBusiness(void)
{
    /* 建立到云端的链接 */
    GAgent_Login_Cloud();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    handleWiFiEvent();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    Socket_CheckNewTCPClient();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    Socket_DoTCPServer();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    Socket_DoUDPServer();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    UART_handlePacket();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);
    GAgent_Ping_McuTick();
    //GAgent_Printf(GAGENT_PACKETDUMP,"  %04d", __LINE__);

    return;
}

