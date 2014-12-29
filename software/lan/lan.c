#ifdef  __cplusplus
extern "C"{
#endif

#include "gagent.h"
#include "lan.h"
#include "Wifi_mode.h"
#include "lib.h"
#include "cloud.h"

#include "iof_import.h"
#include "iof_export.h"
u8 g_Wifihotspots[1024]={0};
u16 g_WifihotspotsLen=0;
int g_SocketLogin[8]={0,0,0,0,0,0,0,0};//保存登陆成功的socketId; 最多有8个tcp socketid    

int g_passcodeEnable=1;

/******************************************************
 功能 : 响应APP 端请求WIFI模组 passcode 的请求
               向 APP 返回passcode
 ******************************************************/
void handlePasscode( int tcpclient,u8* buf, int bufferlen)
{
	int i;
    int ret;
	u8 Repasscode[20] = 
	{
		0x00,0x00,0x00,0x03,0x0F,0x00,0x00,0x07,
		0x00,0x0a,0x00,0x00,0x00,0x00,0x00,0x00,
		0x00,0x00,0x00,0x00
	};	
	for(i=0;i<Repasscode[9];i++)
	{
		Repasscode[10+i] = g_stGAgentConfigData.wifipasscode[i];
	}

	ret = send(tcpclient,Repasscode,20,0);
    GAgent_Printf(GAGENT_INFO,"Send passcode(%s) to client[%d][send data len:%d] ", 
        g_stGAgentConfigData.wifipasscode, tcpclient, ret);
    return;
}

void handleLogin( int tcpclient,u8* buf, int bufferlen)
{
    int i;
    int ret;
	u8 ReLogin[9] = 
	{
		0x00,0x00,0x00,0x03,0x04,0x00,0x00,0x09,0x00
	};

	if( !memcmp((buf+10), g_stGAgentConfigData.wifipasscode,10) )
	{
		//login success!
        ReLogin[8] = 0x00;
        GAgent_Printf(GAGENT_INFO, "LAN login success! clientid[%d] ",tcpclient);

        for(i=0;i<8;i++)
        {
            if( g_SocketLogin[i]==0 )
            {
                g_SocketLogin[i] = tcpclient;
                break;
            }
        }
				
        if( i==8 )
        {          
						GAgent_Printf(GAGENT_INFO," LAN_TCP login need to wait close!  ");
						return ;
        }
				
		ret = send(tcpclient,ReLogin,9,0);
        GAgent_Printf(GAGENT_INFO,"Send login status(0) to client[%d][send data len :%d] ",tcpclient, ret);
	}
	else
    {
        GAgent_Printf(GAGENT_INFO, "passcode:%s", g_stGAgentConfigData.wifipasscode);    
        GAgent_DebugPacket(buf, bufferlen);

        //login fail
        GAgent_Printf(GAGENT_WARNING,"LAN login fail. Datalen:%d", bufferlen);
        GAgent_Printf(GAGENT_WARNING,"your passcode:%s", buf+10);
        ReLogin[8] = 0x01;
        ret = send(tcpclient,ReLogin,9,0);
        GAgent_Printf(GAGENT_WARNING,"Send login status(1) to client[%d][send data len :%d] ",tcpclient, ret);
    }
}

void LAN_handleMCUData(int tcpclient, u8* P0data, int P0len)
{
    GAgentV4_Write2Mcu_with_p0( tcpclient, MCU_CTRL_CMD,P0data,P0len);
    return;
}

int Udp_Onboarding( u8* buf ,int datalength )
{
    unsigned char* passwdlength = NULL;
    char StaSsid[32];
    char StaPass[32];

    memset(StaSsid,0,32);
    memset(StaPass,0,32);	
    memset(g_stGAgentConfigData.wifi_ssid,0,32);
    memset(g_stGAgentConfigData.wifi_key,0,32);

    passwdlength = buf+(10+buf[9]+1);

    strncpy(StaSsid, buf+10,buf[9]);
    StaSsid[buf[9]+1] = '\0';
    strncpy(StaPass,buf+(10+buf[9]+2),*passwdlength);
    StaPass[(*passwdlength)+1]='\0';

    memcpy(g_stGAgentConfigData.wifi_ssid,StaSsid,buf[9]);
    memcpy(g_stGAgentConfigData.wifi_key,StaPass,*passwdlength);    

    g_stGAgentConfigData.flag |= XPG_CFG_FLAG_CONNECTED;
    DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);
    DRV_WiFi_StationCustomModeStart(StaSsid, StaPass);

    return 1;
}

void GetWifiHotspots( int tcpclient,u8* buf, int bufferlen )
{
    varc Uart_varatt;
    u16 TempVarLen;
    int i;
    int totalLen;
    u8 *WifiHotspots_Ack;
    if(g_WifihotspotsLen == 0)
    {
        return;
    }
    WifiHotspots_Ack  = (u8*)malloc(1024);
    if(WifiHotspots_Ack == NULL)
    {
        return;
    }
    //varlen = flag(1b)+cmd(2b)+g_WifihotspotsLen
    TempVarLen = 1+2+g_WifihotspotsLen;    
    Uart_varatt=Tran2varc(TempVarLen);
    totalLen=4+TempVarLen+Uart_varatt.varcbty;

    WifiHotspots_Ack[0]= 0x00;
    WifiHotspots_Ack[1]= 0x00;
    WifiHotspots_Ack[2]= 0x00;
    WifiHotspots_Ack[3]= 0x03;
    //varlen
    for(i=0;i<Uart_varatt.varcbty;i++)
	{
		WifiHotspots_Ack[4+i] = Uart_varatt.var[i];
	}
    //flag
    WifiHotspots_Ack[4+Uart_varatt.varcbty]=0x00;
    //cmd
    WifiHotspots_Ack[4+Uart_varatt.varcbty+1]= 0x00;
    WifiHotspots_Ack[4+Uart_varatt.varcbty+2]= 0x0D;
    //wifiHotspots
    memcpy( &WifiHotspots_Ack[4+Uart_varatt.varcbty+3],g_Wifihotspots,g_WifihotspotsLen);    
    GAgent_Printf(GAGENT_INFO,"g_WifihotspotsLen:%d",g_WifihotspotsLen);
    send(tcpclient,WifiHotspots_Ack,totalLen,0);
    
    free(WifiHotspots_Ack);
}


void GetWifiVersion( int nSocket,unsigned char *pData,int datalength )
{
    u8 WifiVersion[100];
    int i;
    u16 tempVarLen=0;
    varc temp_varatt;
    tempVarLen = 9+g_busiProtocolVerLen;
    //protocolver
    WifiVersion[0]=0x00;
    WifiVersion[1]=0x00;
    WifiVersion[2]=0x00;
    WifiVersion[3]=0x03;
    temp_varatt=Tran2varc(tempVarLen);
    //varLen
    for(i=0;i<temp_varatt.varcbty;i++)
	{
		WifiVersion[4+i] = temp_varatt.var[i];
	}    
    //flag
    WifiVersion[4+temp_varatt.varcbty]=0x00;
    //cmd 
    WifiVersion[4+temp_varatt.varcbty+1]=0x00;
    WifiVersion[4+temp_varatt.varcbty+2]=0x0b;
    //genProtocolVer
    WifiVersion[4+temp_varatt.varcbty+3]=0x00;
    WifiVersion[4+temp_varatt.varcbty+4]=0x00;
    WifiVersion[4+temp_varatt.varcbty+5]=0x00;
    WifiVersion[4+temp_varatt.varcbty+6]=0x03;
    //busiProtocolVerLen
    WifiVersion[4+temp_varatt.varcbty+7]=g_busiProtocolVerLen/255;
    WifiVersion[4+temp_varatt.varcbty+8]=g_busiProtocolVerLen%255;
    //busiProtocolVerLen
    memcpy( &WifiVersion[4+temp_varatt.varcbty+9],g_busiProtocolVer,g_busiProtocolVerLen );
    GAgent_Printf(GAGENT_INFO,"In GetWifiVersion sendLen:%d",tempVarLen+temp_varatt.varcbty+4);
    send( nSocket,WifiVersion,tempVarLen+temp_varatt.varcbty+4,0 );
}


int GetWifiInfo( int nSocket )
{
    int wifiInfoLen=0;    
    unsigned char *wifiInfoBuf=NULL;
    int pos,i=0;
    int TempFirmwareVerLen=0;    
    TempFirmwareVerLen = g_stGAgentConfigData.FirmwareVerLen[1];      
    if( (TempFirmwareVerLen>32)||(TempFirmwareVerLen<=0) )
    {
        //当固件版本信息长度超出规定范围 hard core一个版本信息。
        TempFirmwareVerLen=4;        
        memcpy( g_stGAgentConfigData.FirmwareVer,"V1.0",4);
    }    
    //g_stGAgentConfigData
    wifiInfoLen = 4+1+1+2+8*6+2+TempFirmwareVerLen+2+32;    
    wifiInfoBuf = (u8*)malloc( wifiInfoLen );
    if( wifiInfoBuf==NULL ) return 1;
    memset( wifiInfoBuf,0,wifiInfoLen );
    //protocol 4B
    wifiInfoBuf[0]=0x00;
    wifiInfoBuf[1]=0x00;
    wifiInfoBuf[2]=0x00;
    wifiInfoBuf[3]=0x03;
    //varlen 
    wifiInfoBuf[4]= wifiInfoLen-5;
    //flag 1B
    wifiInfoBuf[5]=0x00;
    //cmd 2B
    wifiInfoBuf[6]=0x00;
    wifiInfoBuf[7]=0x14;
    //wifiHardVar 8B
    memcpy( &wifiInfoBuf[8],WIFI_HARDVER,8 );
    pos= 16;
    //wifiSoftVer 8B
    memcpy( &wifiInfoBuf[pos],WIFI_SOFTVAR,8 );
    pos +=8;
    //mcuHardVer 8B
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.hard_ver,8 );
    pos +=8;
    //mcuSoftVer 8B;
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.soft_ver,8 );
    pos +=8;
    //mcup0Ver 8B;
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.p0_ver,8 );
    pos +=8;
    //firmwareid 8B
    memcpy( &wifiInfoBuf[pos],g_stGAgentConfigData.FirmwareId,8 );
    pos +=8;
    //firmwareVerLen 2B TempFirmwareVerLen    
    wifiInfoBuf[pos] = 0;
    wifiInfoBuf[pos+1] = TempFirmwareVerLen;
    pos +=2;
    //firmwareVer FirmwareVerLen B
    memcpy( &wifiInfoBuf[pos],g_stGAgentConfigData.FirmwareVer,TempFirmwareVerLen );
    pos +=TempFirmwareVerLen;
    // productKeyLen 2b
    wifiInfoBuf[pos]=0;
    wifiInfoBuf[pos+1]=32;
    pos+=2;
    //productkey     
    memcpy( &wifiInfoBuf[pos],g_Xpg_GlobalVar.Xpg_Mcu.product_key,32 );
    
    send( nSocket,wifiInfoBuf,wifiInfoLen,0);    
    free( wifiInfoBuf );
    return 0;
}


void LanAppTick_Ack( int nSocket )
{
    unsigned char AppTick_Ack[8]= 
    {
        0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x16
    };
    send( nSocket,AppTick_Ack,8,0);
}

#define LAN_BIND_CMD            0x0006
#define LAN_LOGIN_CMD           0x0008
#define LAN_MCU_CMD             0x0090
#define LAN_GET_WIFIVER_CMD     0x000A
#define LAN_GET_WIFIHOTS_CMD    0x000C
#define LAN_GET_WIFIINFO_CMD    0x0013
#define LAN_HEARTBEAT_CMD       0x0015
void DispatchTCPData( int nSocket, u8 *pData,int datalength)
{
    int varlen;
    int datalen;
    u8 cmd;
    u8 TcpCmd[2];
	int i;				
    int sendLen;
    unsigned char *pP0Data;

    if( !(pData[0]==0x00&&pData[1]==0x00&&pData[2]==0x00&&pData[3]==0x03)) return ;
    /*根据报文中的报文长度确定报文是否是一个有效的报文*/
    varlen = mqtt_num_rem_len_bytes(pData+4);
    /*这个地方+3是因为MQTT库里面实现把 UDP flag算到messagelen里面，这里为了跟mqtt库保持一致所以加3*/
    datalen = mqtt_parse_rem_len(pData+3); 

    cmd = pData[7+varlen-1];  
    GAgent_Printf(GAGENT_INFO,"LAN_TCP Receive Data  cmd:%2X\n", cmd);

	switch(cmd)//get cmd 
	{
		case 0x0006:  //user bind passcode
    		if( g_passcodeEnable==1 )
            {
                handlePasscode( nSocket,pData, datalength);
            }
		    break;
            
		case 0x0008: //user login 
			handleLogin( nSocket,pData, datalength);			
		    break;
            
		case 0x0090: // send p0 to uart
            for( i=0;i<8;i++ )
            {
            	if( nSocket==g_SocketLogin[i] )
            	{
                	pP0Data = pData+(7+varlen);                      
                	//datalen-3 才是P0的长度因为：datalen后面有falg(1B)+cmd(2B);
                	LAN_handleMCUData( nSocket,pP0Data, datalen-3);
                	break;
            	}
            }
			break;
    	case 0x000A:
            GetWifiVersion( nSocket,pData,datalength );
        	break;
    	case 0x000C:
            GetWifiHotspots( nSocket,pData,datalength );  
            break;
        //串口配置
        case 0x000E:
            break;
        //日志设置
        case 0x0010:
            break;
        //请求wifi模组信息
        case 0X0013:
            GetWifiInfo( nSocket );
            break;
        //心跳包
        case 0X0015:
            LanAppTick_Ack( nSocket );
            break;
		default: 
            GAgent_Printf(GAGENT_WARNING,"DispatchTCPData invalid cmd:%2x\n", cmd);
		break;
	}
    return;
}

#ifdef  __cplusplus
}
#endif /* end of __cplusplus */
