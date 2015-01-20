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
#include "string.h"

#include "gagent_login_cloud.h"
#include "iof_import.h"
#include "iof_export.h"
#include "cloud.h"
#include "md5.h"

int g_ConCloud_Status = CONCLOUD_INVALID;
int (*pf_OTA_Upgrade)(int offset, char *buf, int len);

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

static int Cloud_Http_GetFid(void)
{
    char *content = NULL;
    int len = 0;
    unsigned int fid;
    int ret = 0;
    char content_type[] = "Content-Type: application/text";

    /* 创建到GServer 的短连接 */
    ret = Http_InitSocket(1);
    if(ret != 0)
    {
        GAgent_Printf(GAGENT_INFO, "[CLOUD]%s init socket fail.ret:%d", __func__, ret);
        return ret;
    }

    content = malloc(CLOUD_HTTP_CONTENT_MAX);
    if(NULL == content)
    {
        GAgent_Printf(GAGENT_INFO, "[CLOUD]%s malloc fail!len:%d", __func__, CLOUD_HTTP_CONTENT_MAX);
        return -1;
    }
    len += sprintf(content + len, "/dev/ota/target_fid?");
    len += sprintf(content + len, "did=%s", g_stGAgentConfigData.Cloud_DId);
    len += sprintf(content + len, "&product_key=%s", g_Xpg_GlobalVar.Xpg_Mcu.product_key);
    len += sprintf(content + len, "&type=%d", OTA_TAGET_WIFI);
    len += sprintf(content + len, "&hard_version=%s", WIFI_HARD_VERSION);
    len += sprintf(content + len, "&soft_version=%s", WIFI_SOFTVAR);
    /* 当前fid为1，协议约定上传为1 */
    fid = g_stGAgentConfigData.ota_fid;
    if(fid == 0)
    {
        fid = 1;
    }
    len += sprintf(content + len, "&current_fid=%d", fid);
    content[len] = 0;

    ret = GAgent_Http_Get(HTTP_SERVER, content, content_type);
    free(content);

    return ret;
}

/**********************************************************
 * char *buf: input param
 * int *target_fid: ouput param
 * char *download_url: output param
 **********************************************************/
static int parse_fid_response(char *buf, int *target_fid, char **ppurl)
{
    char *p_start = NULL;
    char *p_end =NULL;
    char fid[10]={0};
    char *url = NULL;

    /* 解析target_fid */
    p_start = GAgent_strstr( buf,"target_fid=");
    if( p_start==NULL )
        return 1;
    p_start =  p_start+(sizeof( "target_fid=" )-1);

    p_end = GAgent_strstr( p_start,"&");
    if( p_end==NULL )
        return 1;
    memcpy( fid,p_start,(p_end-p_start));

    *target_fid = atoi(fid);

    /* 解析url  */
    p_start = GAgent_strstr(p_end,"/dev/ota/download/");
    if(p_start==NULL )
        return 1;

    p_end = GAgent_strstr(p_start, "&");
    if( p_end==NULL )
        return 1;

    url = malloc(p_end - p_start + 1);
    if(NULL == url)
        return 1;
    memcpy( url, p_start, (p_end-p_start));
    url[p_end-p_start] = '\0';

    if(NULL != *ppurl)
    {
        free(*ppurl);
    }
    *ppurl = url;
    GAgent_Printf(GAGENT_INFO, "target fid :%d, url:%s", *target_fid, url);

    return 0;
}

static int Cloud_Http_Rec_Fid(int *fid, char **ppurl)
{
    int ret;
    char *httpReceiveBuf = NULL;

    httpReceiveBuf = malloc(SOCKET_RECBUFFER_LEN);
    if(httpReceiveBuf == NULL)
    {
        GAgent_Printf(GAGENT_INFO, "[CLOUD]%s malloc fail!len:%d", __func__, SOCKET_RECBUFFER_LEN);
        return -1;
    }

    ret = Http_ReadSocket( g_Xpg_GlobalVar.http_socketid, httpReceiveBuf, SOCKET_RECBUFFER_LEN );    
    if(ret <=0 ) 
    {
        free(httpReceiveBuf);
        return -1;
    }
    
    ret = Http_Response_Code( httpReceiveBuf );
    if(CLOUD_HTTP_FID_RESPON != ret)
    {
        free(httpReceiveBuf);
        return -1;
    }

    httpReceiveBuf[SOCKET_RECBUFFER_LEN - 1] = 0;
    /* 解析ota download content  */
    ret = parse_fid_response(httpReceiveBuf, fid, ppurl);

    free(httpReceiveBuf);

    return ret;
}

static int Cloud_Http_Ota_download(char *purl)
{
    int ret = 0;
    char content_type[] = "Content-Type: application/text";

    /* 创建到GServer 的短连接 */
    ret = Http_InitSocket(1);
    if(ret != 0)
    {
        GAgent_Printf(GAGENT_INFO, "[CLOUD]%s init socket fail.ret:%d", __func__, ret);
        return ret;
    }

    return GAgent_Http_Get(HTTP_SERVER, purl, content_type);
}

extern int Http_HeadLen( char *httpbuf );
extern int Http_BodyLen( char *httpbuf );
extern int Http_GetFV(char *httpbuf,char *FV );
extern int Http_GetMD5( char *httpbuf,char *MD5);

static int Cloud_Http_Ota_Update(void)
{
    int ret;
    char *httpReceiveBuf = NULL;
    int headlen = 0;
    char MD5[16] = {0};
    char md5_calc[16] = {0};
    char GAgent_FV[FIRMWARE_LEN + 1]={0};
    unsigned int filelen = 0;
    int offset = 0;
    char *buf = NULL;
    int writelen = 0;
    MD5_CTX ctx;

    httpReceiveBuf = malloc(SOCKET_RECBUFFER_LEN);
    if(httpReceiveBuf == NULL)
    {
        GAgent_Printf(GAGENT_INFO, "[CLOUD]%s malloc fail!len:%d", __func__, SOCKET_RECBUFFER_LEN);
        return -1;
    }

    ret = Http_ReadSocket( g_Xpg_GlobalVar.http_socketid, httpReceiveBuf, SOCKET_RECBUFFER_LEN );    
    if(ret <=0 ) 
    {
        free(httpReceiveBuf);
        return -1;
    }
    
    ret = Http_Response_Code( httpReceiveBuf );
    if(CLOUD_HTTP_OTA_DOWN_RESPON != ret)
    {
        free(httpReceiveBuf);
        return -1;
    }

    headlen = Http_HeadLen( httpReceiveBuf );
    filelen = Http_BodyLen( httpReceiveBuf );
    Http_GetMD5( httpReceiveBuf,MD5);
    Http_GetFV( httpReceiveBuf,GAgent_FV );
    GAgent_FV[FIRMWARE_LEN] = 0;

    offset = 0;
    buf = httpReceiveBuf + headlen;
    writelen = SOCKET_RECBUFFER_LEN - headlen;
    MD5Init(&ctx);
    do
    {
        ret = pf_OTA_Upgrade(offset, buf, writelen);
        if(ret < 0)
        {
            GAgent_Printf(GAGENT_INFO, "[CLOUD]%s OTA upgrad fail at off:0x%x", __func__, offset);
            free(httpReceiveBuf);
            return -1;
        }
        offset += writelen;

        MD5Update(&ctx, buf, writelen);
        
        writelen = filelen - offset;
        if(0 == writelen)
            break;
        if(writelen > SOCKET_RECBUFFER_LEN)
        {
            writelen = SOCKET_RECBUFFER_LEN;
        }
        writelen = Http_ReadSocket( g_Xpg_GlobalVar.http_socketid, httpReceiveBuf, writelen );    
        if(writelen <= 0 )
        {
            GAgent_Printf("[CLOUD]%s, socket rec ota file fail!recived:0x%x", __func__, offset);
            free(httpReceiveBuf);
            return -1;
        }
        buf = httpReceiveBuf;
    }while(offset < filelen);
    
    MD5Final(&ctx, md5_calc);
    if(memcmp(MD5, md5_calc, 16) != 0)
    {
        /* md5 校验失败 */
        GAgent_Printf(GAGENT_INFO, "[CLOUD]md5 fail!");
        return -1;
    }
    
    memcpy(g_stGAgentConfigData.FirmwareVer, GAgent_FV, FIRMWARE_LEN);
    g_stGAgentConfigData.FirmwareVerLen = FIRMWARE_LEN;

    free(httpReceiveBuf);

    GAgent_Printf(GAGENT_INFO, "GAgent_FV:%s", GAgent_FV);
    
    return 0;
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
    			g_ConCloud_Status = CONCLOUD_REQ_FID;
                GAgent_Printf(GAGENT_WARNING,"--HTTP response OK.");

            }
            break;
        case CONCLOUD_REQ_FID:
            GAgent_Printf(GAGENT_INFO, "CONCLOUD_REQ_FID");
            ret = Cloud_Http_GetFid();
            if(ret != 0)
            {
                /* 获取ota fid 失败, 不重试直接进行登录 */
                g_ConCloud_Status = CONCLOUD_REQ_LOGIN;
                GAgent_Printf(GAGENT_INFO, "[CLOUD]%s get:req fid fail!");
            }
            else
            {
                /* 等待服务器返回ota fid 相关信息  */
                g_ConCloud_Status = CONCLOUD_REQ_FID_ACK;
            }
            break;
        case CONCLOUD_REQ_FID_ACK:
            /* 接收并解析服务器返回的ota fid相关信息 */
            GAgent_Printf(GAGENT_INFO, "CONCLOUD_REQ_FID_ACK");
            ret = Cloud_Http_Rec_Fid(&(Gagent_Cloud_status.ota_fid), &(Gagent_Cloud_status.otaUrlHttpCont));
            if(ret != 0)
            {
                /* 获取ota fid失败, 不重试直接进行登录 */
                g_ConCloud_Status = CONCLOUD_REQ_LOGIN;
                GAgent_Printf(GAGENT_INFO, "[CLOUD]%s get:fid ack parse fail!", __func__);
                break;
            }
            
            GAgent_Printf(GAGENT_INFO,"[CLOUD]get OTA url success.");
            if(Gagent_Cloud_status.ota_fid != g_stGAgentConfigData.ota_fid)
            {
                GAgent_Printf(GAGENT_INFO,"[CLOUD]firmware file id unmatch,update!");
                g_ConCloud_Status = CONCLOUD_REQ_OTA_FILE;
            }
            else
            {
                GAgent_Printf(GAGENT_INFO,"[CLOUD]don't need update firmware.");
                g_ConCloud_Status = CONCLOUD_REQ_LOGIN;
            }
            break;
        case CONCLOUD_REQ_OTA_FILE:
            ret = Cloud_Http_Ota_download(Gagent_Cloud_status.otaUrlHttpCont);
            if(ret != 0)
            {
                /* 发送get请求失败  */
                g_ConCloud_Status = CONCLOUD_REQ_LOGIN;
                GAgent_Printf(GAGENT_INFO, "get firmware req fail");
            }
            else
            {
                /* 发送get请求成功 */
                g_ConCloud_Status = CONCLOUD_REQ_OTA_FILE_ACK;
                GAgent_Printf(GAGENT_INFO, "get firmware req success");
            }

            break;
        case CONCLOUD_REQ_OTA_FILE_ACK:
            GAgent_Printf(GAGENT_INFO,"[CLOUD]update OTA start.");
            ret = Cloud_Http_Ota_Update();
            if(0 == ret)
            {
                /* ota成功后释放url  */
                free(Gagent_Cloud_status.otaUrlHttpCont);
                Gagent_Cloud_status.otaUrlHttpCont = NULL;
                /* 更新fid, fireware version信息 */
                g_stGAgentConfigData.ota_fid = Gagent_Cloud_status.ota_fid;
                DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);
                GAgent_Printf(GAGENT_INFO, "[OTA]Upgrade complete");

                /* 向云端更新当前的target_fid */
                Cloud_Http_GetFid();
				
				/* 必要时重启wifi模组或应用程序 */
				
            }

            g_ConCloud_Status = CONCLOUD_REQ_LOGIN;
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

