#include "gagent.h"
#include "lib.h"
#include "mqttxpg.h"
#include "MqttSTM.h"
#include "mqttxpg.h"
#include "mqttlib.h"
#include "Wifi_mode.h"
#include "lan.h"
#include "lib.h"
#include "Socket.h"
#include "wifi.h"
#include "http.h"


#include "gagent_login_cloud.h"
#include "iof_import.h"

int g_ConCloud_Status = CONCLOUD_INVALID;

int Http_Recive_Did(char *DID)
{
    int ret;
    int response_code = 0;
    char httpReceiveBuf[1024] = {0};
    
    ret = Http_ReadSocket( g_Xpg_GlobalVar.http_socketid,httpReceiveBuf,1024 );    
    if(ret <=0 ) 
    {
        return -1;
    }

    response_code = Http_Response_Code( httpReceiveBuf );       
    if( response_code==201 )
    {
        return Http_Response_DID( httpReceiveBuf, DID);
    }
    else
    {
        GAgent_Printf(GAGENT_WARNING,"HTTP response_code:%d",response_code);
        return -1;
    }

}

int Http_Recive_M2minfo(char *server, int *port)
{
    
    int ret;
    int response_code = 0;
    char httpReceiveBuf[1024] = {0};

    ret = Http_ReadSocket( g_Xpg_GlobalVar.http_socketid,httpReceiveBuf,1024 );    
    if(ret <=0 ) 
    {
        return -1;
    }
    
    response_code = Http_Response_Code( httpReceiveBuf );  
    if( response_code!=200 ) 
    {
        return -1;
    }
    
    ret = Http_getdomain_port( httpReceiveBuf, server, port );

    return ret;
}

/**************************************************
 **************************************************
 建立和云端服务器的连接
 MQTT业务处理(订阅/登陆/数据等)由MQTT handle
 在此期间建立socket短链接
 WIFI                             CLOUD(HTTP)                       CLOUD(MQTT)
 =================================================
 REQ_DID         ------->    handle
 REQ_DID_ACK    <----    DID_INFO
 REQ_M2MINFO   ------>    handle
 REQ_M2MINFO_ACK  <--  M2M_INFO
 MQTT_LOGIN   --------     (END)            ----------->     mqtt handle(login)
 
 **************************************************/
void GAgent_Login_Cloud(void)
{
    int ret;
    int response_code=0;
    char httpReceiveBuf[1024];

    /* WIFI STATION模式下才能接入cloud */
    if( (wifiStatus&WIFI_MODE_STATION) != WIFI_MODE_STATION ||
        (wifiStatus&WIFI_STATION_STATUS) != WIFI_STATION_STATUS)
    {
        return ;
    }

    switch(g_ConCloud_Status)
    {
        case CONCLOUD_REQ_DID:
            if( DRV_GAgent_GetTime_S()-g_Xpg_GlobalVar.connect2CloudLastTime < 1 )
            {
                return ;
            }
            g_Xpg_GlobalVar.connect2CloudLastTime = DRV_GAgent_GetTime_S();
            if((Mqtt_Register2Server(&g_stMQTTBroker)==0))
            {
                g_ConCloud_Status = CONCLOUD_REQ_DID_ACK;
                g_Xpg_GlobalVar.send2HttpLastTime = DRV_GAgent_GetTime_S();
                GAgent_Printf(GAGENT_INFO,"MQTT_Register2Server OK !");
                
            }
            else
            {
                GAgent_Printf(GAGENT_INFO,"Mqtt_Register2Server Fail !");
            }
            break;
        case CONCLOUD_REQ_DID_ACK:
            /* time out, or socket isn't created , resent  CONCLOUD_REQ_DID */
            if( DRV_GAgent_GetTime_S()-g_Xpg_GlobalVar.send2HttpLastTime>=HTTP_TIMEOUT || 
                g_Xpg_GlobalVar.http_socketid<=0 )
            {
                g_ConCloud_Status = CONCLOUD_REQ_DID_ACK;
                break;
            }

            ret = Http_Recive_Did(g_Xpg_GlobalVar.DID);
            if(ret == 0)
            {
                char *p_Did = g_Xpg_GlobalVar.DID;
                DIdLen = GAGENT_DID_LEN;
                memcpy(g_stGAgentConfigData.Cloud_DId, p_Did, DIdLen);
                g_stGAgentConfigData.Cloud_DId[DIdLen] = '\0';
                DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);
                
                g_ConCloud_Status = CONCLOUD_REQ_M2MINFO;
                GAgent_Printf(GAGENT_INFO,"Http Receive DID is:%s[len:%d]",p_Did, DIdLen); 
                GAgent_Printf(GAGENT_INFO,"ConCloud Status[%04x]", g_ConCloud_Status);
            }
            break;

        case CONCLOUD_REQ_M2MINFO:
            if( DRV_GAgent_GetTime_S()-g_Xpg_GlobalVar.connect2CloudLastTime < 1 )
            {
                return ;
            }
            g_Xpg_GlobalVar.connect2CloudLastTime = DRV_GAgent_GetTime_S();
            if( Http_Sent_Provision()==0 )
            {
                g_Xpg_GlobalVar.send2HttpLastTime = DRV_GAgent_GetTime_S();
                g_ConCloud_Status = CONCLOUD_REQ_M2MINFO_ACK;
                GAgent_Printf( GAGENT_INFO,"Http_Sent_Provision OK! ");
            }                
            else
            {
                GAgent_Printf( GAGENT_INFO,"Http_Sent_Provision Fail!");
            }
            break;
        case CONCLOUD_REQ_M2MINFO_ACK:
            /* time out, or socket isn't created ,resent  CONCLOUD_REQ_M2MINFO */
            if( DRV_GAgent_GetTime_S()-g_Xpg_GlobalVar.send2HttpLastTime>=HTTP_TIMEOUT || 
                g_Xpg_GlobalVar.http_socketid<=0 )
            {
                g_ConCloud_Status = CONCLOUD_REQ_M2MINFO;
                break;
            }

            ret = Http_Recive_M2minfo(g_Xpg_GlobalVar.m2m_SERVER, &g_Xpg_GlobalVar.m2m_Port);
            if( ret==0 ) 
            {
    			g_ConCloud_Status = CONCLOUD_REQ_LOGIN;
                GAgent_Printf(GAGENT_WARNING,"--HTTP response OK. Goto MQTT LOGIN");

            }
            break;
        case CONCLOUD_REQ_LOGIN:
            if( DRV_GAgent_GetTime_S()-g_Xpg_GlobalVar.connect2CloudLastTime < 1 )
            {
                return ;
            }
            g_Xpg_GlobalVar.connect2CloudLastTime = DRV_GAgent_GetTime_S();
            if((Mqtt_Login2Server(&g_stMQTTBroker)==0))
            {
                g_ConCloud_Status = CONCLOUD_RUNNING;
                g_MQTTStatus = MQTT_STATUS_LOGIN;
                GAgent_Printf(GAGENT_INFO,"Mqtt_Login2Server OK!");
            }
            else
            {
                GAgent_Printf(GAGENT_INFO,"Mqtt_Login2Server Fail!");
            }
            break;
        case CONCLOUD_RUNNING:
            /* mqtt handle */
            if(g_MQTTStatus == MQTT_STATUS_LOGIN)
            {
                if( DRV_GAgent_GetTime_S()-g_Xpg_GlobalVar.connect2CloudLastTime >= MQTT_CONNECT_TIMEOUT )
                {
                    g_ConCloud_Status = CONCLOUD_REQ_LOGIN;
                    break;
                }
            }
            MQTT_handlePacket();
            break;
        case CONCLOUD_INVALID:
        default:
            break;
    }
}

