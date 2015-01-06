#ifdef  __cplusplus
extern "C"{
#endif

#include "gagent.h"
#include "lib.h"
#include "mcu.h"
#include "lan.h"
#include "Wifi_mode.h"
#include "main.h"
#include "mqttxpg.h"
#include "MqttSTM.h" 
#include "wifi.h" 
#include "Socket.h"    
#include "iof_import.h"
#include "local.h"

void MCU_ResetPingTime()
{
    g_Xpg_GlobalVar.Xpg_Mcu.XPG_PingTime = 0;
    g_Xpg_GlobalVar.Xpg_Mcu.loseTime = 0;
}

int GAgent_Ack2Mcu(int fd, char sn,char cmd)
{
    int ret = 0;  
    int len = MCU_LEN_NO_PIECE; 
    unsigned short p0_len = htons(5);    
    unsigned char buf[MCU_LEN_NO_PIECE];
    
    memset(buf, 0, len);
    buf[0] = MCU_HDR_FF;
    buf[1] = MCU_HDR_FF;
    memcpy(&buf[MCU_LEN_POS], &p0_len, 2);
    buf[MCU_CMD_POS] = cmd;
    buf[MCU_SN_POS] = sn;
    buf[MCU_LEN_NO_PIECE-1]=GAgent_SetCheckSum((unsigned char *)buf, (MCU_LEN_NO_PIECE-1));
    MCU_SendData( buf,len );

    return  0;
}


void XPG_Wifi_Config( unsigned char  wifi_config_type ,short wifiStatus )
{      
    
    if( (wifiStatus&WIFI_STATION_STATUS) == WIFI_STATION_STATUS )    
    {       
        //into passcode enable
        if(g_Xpg_GlobalVar.Xpg_Mcu.passcodeEnableTime>0)
        {
            g_passcodeEnable=1;
            GAgent_CreateTimer(GAGENT_TIMER_CLOCK, ONE_SECOND*g_Xpg_GlobalVar.Xpg_Mcu.passcodeEnableTime, MCU_PasscodeTimer);                
            GAgent_Printf(GAGENT_INFO,"Passcode Enable %d s.",g_Xpg_GlobalVar.Xpg_Mcu.passcodeEnableTime);
        }
        else
        {
            g_passcodeEnable=1;
        }
    }
    else
    {
        GAgent_Printf(GAGENT_INFO,"Call XPG_Wifi_Config, wifiStatus:%08x, wifiConfigType:%d.", 
                wifiStatus, wifi_config_type);

        switch( wifi_config_type )
        {
            //softap mode
            case 1:
                break;
            //airlink mode
            case 2:
							
                IntoEasyLink();
                break;
            //wps mode
            case 3:
                break;
            //nfc mode
            case 4:
                break;
            //wac mode
            case 5:
                break;
            default:
                break;
        }
    }
}


/******************************************************
*
*		Re_buf      : uart receive data pointer,	
*		Pload       :	uart receive playload data pointer
*       flag        :   0:cmd payload
                        1:debug payload;
*		return      :	the playload length 
*		Add by Alex lin  2014-03-20
*
*******************************************************/
int get_uart_Pload( u8* Re_buf,u8* Pload,int Plen,unsigned char  Cmd )
{	
    if( Cmd==0x91)
        memcpy( Pload,Re_buf+8,Plen ); 
    else 
        memcpy( Pload,Re_buf,Plen );
    
	return Plen;
}

/**********************************************************
*
*				Function     :		add wifi to phone head
*				buf			 :		uart receive buf
*				PHead		 :		the head to phone
*				return 	     :		the head length 
*				Add by Alex 	2014-04-21
***********************************************************/
int Add_W2PHead( short ULen,u8*PHead,unsigned char Cmd )
{
	char W2PHead[100]= {0};
	char TmpLen[2];		
	short packLen;
	int HeadLen;
	varc Uart_varatt;			
	int i;		

	packLen = ULen;
	//packLen = packLen-4+1;
	Uart_varatt=Tran2varc	(packLen);
	W2PHead[0] = 0x00;
	W2PHead[1] = 0x00;
	W2PHead[2] = 0x00;
	W2PHead[3] = 0x03;
	for(i=0;i<Uart_varatt.varcbty;i++)
	{
		W2PHead[4+i] = Uart_varatt.var[i];
	}
	W2PHead[4+Uart_varatt.varcbty] = 0x00;
	W2PHead[4+Uart_varatt.varcbty+1] = 0x00;
	W2PHead[4+Uart_varatt.varcbty+2] = Cmd;
	HeadLen = 4+Uart_varatt.varcbty+2+1;				
	memcpy(PHead,W2PHead,HeadLen);				
	return HeadLen;
}

/***************************************************************************************
*
*				FUNCTION :	send mcu cmd to phone 
*				uartbuf	 :	uart receive buf 
*				fd		 :  fd>0    :   socketid 
                            0       :   send data to all phone
*				Add by Alex 2014-04-21
*
*****************************************************************************************/
void MCU_SendPacket2Phone(  u8* uartbuf,int uartbufLen,unsigned char Cmd,int fd )
{
	u8 HPhead[100];
	char sId[4];
	int socketId;	
	int HPheadLen=0;
	u8 *SendPack=NULL;
	int SendPackLen=0;	
	int PLen=0;
	short ULen;
	char* Pload=NULL;
    int i;

    if( Cmd==0x91 )
    {
        PLen = uartbuf[2]*256+uartbuf[3]-5;            
    }
    if( Cmd==0x12 )
    {
        PLen = uartbufLen;        
    }
    ULen = PLen+3;
	Pload = (char *)malloc( PLen );
    if (Pload == NULL)
    {
        return;
    }
	memset(Pload,0,PLen);
		
    socketId = fd; 
	
	get_uart_Pload( uartbuf,Pload,PLen,Cmd);
	
	HPheadLen = Add_W2PHead( ULen,HPhead,Cmd );
	
	SendPackLen = HPheadLen+PLen;
	SendPack = (u8*)malloc(SendPackLen);
    if (SendPack == NULL)
    {
        free(Pload);
        return;
    }
	
	memcpy(SendPack,HPhead,HPheadLen);
	memcpy(SendPack+HPheadLen,Pload,PLen);

	if ( fd >0 )
	{
        for( i=0;i<8;i++ )
        {
            if( socketId==g_SocketLogin[i] )
            {
                send(socketId,SendPack,SendPackLen,0); 
                break;
            }
        }
    }
	else
	{
	    Socket_SendData2Client( SendPack,SendPackLen,Cmd );
	}
	free(SendPack) ;
	free(Pload);
    
    return;
}

/**************************************************************************
*
*   Function    :   MCU_SendPublishPacket()
*                   send data to cloud 
*   uartbuf     :   uart receive data
*   uartbufLen  :   uart receive data len 
*
*
**************************************************************************/
int MCU_SendPublishPacket(u8 *uartbuf,int uartbufLen )
{
    varc SendVarc;
    u8 *SendPack=NULL;
    int SendPackLen=0;   
    int UartP0Len=0;    
    u8 McuSendTopic[200]={0};
    short VarLen=0;
    int i;
   
    if ( g_MQTTStatus!=MQTT_STATUS_RUNNING ) 
    {
        GAgent_Printf(GAGENT_INFO, "MCU_SendPublishPacket:%d\n",g_MQTTStatus);
        return  1;
    }

    //head(2b)+len(2b)+sn(1B)+cmd(1B)+flags(2b)+p0
    if( uartbuf[0]==0xff&&uartbuf[1]==0xff )
	{
		UartP0Len = uartbufLen-8;
 	}         
    else
	{
		return 1;
	}
    memcpy( McuSendTopic,"dev2app/",strlen("dev2app/"));
    memcpy( McuSendTopic+strlen("dev2app/"),g_Xpg_GlobalVar.DID,strlen(g_Xpg_GlobalVar.DID));              
    McuSendTopic[strlen("dev2app/")+strlen(g_Xpg_GlobalVar.DID)]='\0';
    
    //protocolVer(4B)+varLen(1~4B)+flag(1B)+cmd(2B)+P0
    VarLen=1+2+UartP0Len;
    
    SendVarc = Tran2varc(VarLen);
    SendPackLen = 4+SendVarc.varcbty+1+2+UartP0Len;
    
    SendPack = (u8*)malloc(SendPackLen);
    if( SendPack==NULL ) return 1;
    
    //protocolVer
    SendPack[0] = 0x00;
    SendPack[1] = 0x00;
    SendPack[2] = 0x00;
    SendPack[3] = 0x03;
    //varLen
    for(i=0;i<SendVarc.varcbty;i++)
	{
		SendPack[4+i] = SendVarc.var[i];
	}
     //flag   
    SendPack[4+SendVarc.varcbty] = 0x00;
    //CMD
    SendPack[4+SendVarc.varcbty+1]=0x00;
    SendPack[4+SendVarc.varcbty+2]=0x91;
    //p0

    memcpy( (SendPack+4+SendVarc.varcbty+3),uartbuf+8,UartP0Len);
    GAgent_Printf(GAGENT_INFO, "MCU Send Topic : %s\r\n",McuSendTopic);  
        
    PubMsg( &g_stMQTTBroker,McuSendTopic,SendPack,SendPackLen,0);   
    free( SendPack );       
    
    return 0;
}


void MCU_OutputLOGInfo(char *log, int len, int level)
{
    unsigned char InfoPayload[512];    
    unsigned char logLevel;
    unsigned short tagLen;
    unsigned short sourceLen;
    unsigned short contentLen;
    int LogLen = len;

    if (LogLen > 500)
    {
        LogLen = 500;
    }
    
    tagLen = 0;
    sourceLen = 0;
    memset(InfoPayload, 0x0, sizeof(InfoPayload)); 
    InfoPayload[0] = level;
    //tagLen 2B
    InfoPayload[1] = 0;
    InfoPayload[2] = 0;
    //sourceLen 2B
    InfoPayload[3] = 0;
    InfoPayload[4] = 0;
    //contentLen 2B
    InfoPayload[5] = (len >> 8) & 0xFF;
    InfoPayload[6] = len & 0xFF;
    memcpy(InfoPayload + 7, log, LogLen);
    LogLen += 7;
    MCU_SendPacket2Phone(InfoPayload, LogLen, 0x12, 0);

    return;
}

int GAgent_GetMCUInfo( u32 Time )
{    
    unsigned char checksum=0; 
    short passcodeEnableTime=0;
    u16 *pTime=NULL;
    unsigned int i=0;         
    int pos=0;
    unsigned char get_Mcu_InfoBuf[9]=
    {
        0xff,0xff,0x00,0x05,0x01,0x01,0x00,0x00,0x07
    }; 

    
    checksum = GAgent_SetCheckSum( get_Mcu_InfoBuf, 8);
    get_Mcu_InfoBuf[8] = checksum;                                
    MCU_SendData( get_Mcu_InfoBuf,9 );

    DRV_Led_Green(0);
    for( i=0;i<20;i++ )
    {
        if(GAgent_CheckAck( 0,get_Mcu_InfoBuf,9, DRV_GAgent_GetTime_MS())==0)
        {   
            pos = pos+8;
            memcpy( g_Xpg_GlobalVar.Xpg_Mcu.protocol_ver,g_stSocketRecBuffer,8 );
            //for printf.
            g_Xpg_GlobalVar.Xpg_Mcu.protocol_ver[8]='\0';
            pos = pos+8;
            memcpy( g_Xpg_GlobalVar.Xpg_Mcu.p0_ver,g_stSocketRecBuffer+pos,8 );
            g_Xpg_GlobalVar.Xpg_Mcu.p0_ver[8]='\0';
            pos+=8;
            memcpy( g_Xpg_GlobalVar.Xpg_Mcu.hard_ver,g_stSocketRecBuffer+pos,8 );
            g_Xpg_GlobalVar.Xpg_Mcu.hard_ver[8]='\0';
            pos+=8;
            memcpy( g_Xpg_GlobalVar.Xpg_Mcu.soft_ver,g_stSocketRecBuffer+pos,8 );
            g_Xpg_GlobalVar.Xpg_Mcu.soft_ver[8]='\0';
            pos+=8;
            memcpy( g_Xpg_GlobalVar.Xpg_Mcu.product_key, g_stSocketRecBuffer+pos,32);
            g_Xpg_GlobalVar.Xpg_Mcu.product_key[32]='\0';
            g_productKeyLen=32;
            
            pos+=32;
            pTime = (u16*)&g_stSocketRecBuffer[pos];             
            passcodeEnableTime = ntohs(*pTime);
            g_Xpg_GlobalVar.Xpg_Mcu.passcodeEnableTime=passcodeEnableTime;

            //always enable passcode
            g_passcodeEnable=1;
            
            if( g_Xpg_GlobalVar.Xpg_Mcu.passcodeEnableTime>0)
            {
                GAgent_CreateTimer(GAGENT_TIMER_CLOCK, ONE_SECOND*g_Xpg_GlobalVar.Xpg_Mcu.passcodeEnableTime, MCU_PasscodeTimer);                
                GAgent_Printf(GAGENT_INFO,"Passcode Enable %d s.",g_Xpg_GlobalVar.Xpg_Mcu.passcodeEnableTime);
            }

            GAgent_Printf(GAGENT_INFO,"MCU Protocol Vertion:%s.",g_Xpg_GlobalVar.Xpg_Mcu.protocol_ver);
            GAgent_Printf(GAGENT_INFO,"MCU P0 Vertion:%s.",g_Xpg_GlobalVar.Xpg_Mcu.p0_ver);
            GAgent_Printf(GAGENT_INFO,"MCU Hard Vertion:%s.",g_Xpg_GlobalVar.Xpg_Mcu.hard_ver);
            GAgent_Printf(GAGENT_INFO,"MCU Soft Vertion:%s.",g_Xpg_GlobalVar.Xpg_Mcu.soft_ver);
            GAgent_Printf(GAGENT_INFO,"MCU Product Vertion:%s.",g_Xpg_GlobalVar.Xpg_Mcu.product_key);

            DRV_Led_Green(1);
            break;
        }        
        else
        {
            msleep(400);
            DRV_Led_Red( i%2 );
        }
    }

    if (i == 20)
    {
        DRV_Led_Red(1);
        DRV_Led_Green(1);
        msleep(4000);
        DRV_GAgent_Reset();
    }
   
    MCU_ResetPingTime();
    
    return 0;
}


/*****************************************************************
*   fd      :   fd>0 is a socketid
*   buf     :   wifi 2 mcu buf.
*   bufLen  :   wifi 2 mcu bufLen
*   return  :   0 check success   1 check fail
*       add by alex lin 2014-09-02
*   
*******************************************************************/
int GAgent_CheckAck( int fd,unsigned char *buf,int bufLen, u32 time )
{
    int i=0;
    int PacketLen=0;
    int resend_time=1;
    unsigned char checksum=0;
    int sum=0;

    while(1)
    {
        PacketLen = MCU_GetPacket( g_stSocketRecBuffer, SOCKET_RECBUFFER_LEN );
        if( PacketLen>0 )
        {
            checksum = GAgent_SetCheckSum(g_stSocketRecBuffer,(PacketLen-1));        
            /*(PacketLen-1) remove the last byte, checksum*/

            if( (g_stSocketRecBuffer[4]==(buf[4]+1))&& /*need careful about protocol*/
                (buf[5]==g_stSocketRecBuffer[5]))
            {
                if ((checksum==g_stSocketRecBuffer[PacketLen-1]))
                {
                    //send data to app.
                    if(  PacketLen>9 && fd>0 )
                    {
                        MCU_SendPacket2Phone( g_stSocketRecBuffer,PacketLen,0x91,fd );                        
                    }
                }
                else
                {
                    GAgent_Printf(GAGENT_INFO, "Get ACK failed, checksum info: %d:%d %d", 
                            checksum, g_stSocketRecBuffer[PacketLen-1], PacketLen);
                    GAgent_Printf(GAGENT_INFO, "Send Packet Info:");
                    GAgent_DebugPacket(buf, bufLen);
                    GAgent_Printf(GAGENT_INFO, "Received Packet Info:");
                    GAgent_DebugPacket(g_stSocketRecBuffer, PacketLen);
                }
                return 0;            
            }
            else
            {
                MCU_DispatchPacket( g_stSocketRecBuffer,PacketLen ); 
            }
        }
        else if( (DRV_GAgent_GetTime_MS()-time) >= MCU_ACK_TIME )
        {
            time=DRV_GAgent_GetTime_MS();
            if( resend_time>=3 )
            {
                GAgent_Printf(GAGENT_INFO, "Get ACK failed at %s:%d, packet info:", __FUNCTION__, __LINE__);
                GAgent_DebugPacket(buf, bufLen);
                return 1;
            }
            MCU_SendData( buf,bufLen );            
            resend_time += 1;            
        }
        msleep(10);
    }
}


/************************************************************************************************
*
*       FUNCTION    :   XPG_Write2Mcu_with_p0() send data to MCU ,this function will auto add uart
*                       head(8B).
*       fd          :   alway 0 in mxchip 3162
*       cmd         :   "MCU_CTRL_CMD"  or "WIFI_STATUS2MCU"
*       data        :   need to send to uart data
*       data_len    :   the len of data
*
************************************************************************************************/
int GAgentV4_Write2Mcu_with_p0(int fd, unsigned char cmd, unsigned char *data, unsigned short data_len)
{
    int ret = 0;  
    int len = MCU_LEN_NO_PIECE; 
    unsigned short p0_len;    
    unsigned char *buf = NULL;

    
    if(data_len > (4*1024))
    {
        GAgent_Printf(GAGENT_INFO, "%s:%d P0 is too long[%08x]", 
            __FUNCTION__, __LINE__, data_len);
        return -1;
    }
    else
    {
        len += data_len;
        buf = (unsigned char*)malloc(len);
        if( buf==NULL ) 
        {
            GAgent_Printf(GAGENT_INFO, "%s:%d malloc error", __FUNCTION__, __LINE__);        
            return 1;
        }
        
        memset(buf, 0, len);        
        buf[0] = MCU_HDR_FF;
        buf[1] = MCU_HDR_FF;
        p0_len = htons(len-4);
        memcpy(&buf[MCU_LEN_POS], &p0_len, 2);
        buf[MCU_CMD_POS] = cmd;             
        memcpy(&buf[MCU_NO_PIECE_P0_POS],data,data_len);
        
        GAgent_SetSN(buf);          
        buf[len-1] = GAgent_SetCheckSum( buf, (len-1) );
        MCU_SendData( buf,len );
    }
    
    if(GAgent_CheckAck( fd,buf,len,DRV_GAgent_GetTime_MS() )==0)
    {
        ret=0;
    }
    else
    {
        ret=1;
    }
    free( buf );

    GAgent_Printf(GAGENT_INFO, "%s:%d send [%02x] to mcu", __FUNCTION__, __LINE__, cmd);

    return ret;
}

/**************************************************
*
*       wifi send Uart HeartBeat to MCU    
*       
***************************************************/
static void Local_HB_Send(void)
{
    unsigned char pingbuf[9] = { 0xFF,0xFF,0x00,0x05,0x07,0x00,0x00,0x00,0x00 };

    pingbuf[MCU_CMD_POS] = WIFI_PING2MCU;
    GAgent_SetSN( pingbuf );
    pingbuf[8] = GAgent_SetCheckSum( pingbuf,8);
    MCU_SendData( pingbuf,9 );
    return ;
}

void Local_HB_Timer(void)
{
    g_Xpg_GlobalVar.Xpg_Mcu.XPG_PingTime++;
    if(g_Xpg_GlobalVar.Xpg_Mcu.XPG_PingTime >= LOCAL_HB_TIMEOUT_ONESHOT)
    {
        Local_HB_Send();
        g_Xpg_GlobalVar.Xpg_Mcu.loseTime++;
        g_Xpg_GlobalVar.Xpg_Mcu.XPG_PingTime = 0;
    }
    if(g_Xpg_GlobalVar.Xpg_Mcu.loseTime >= LOCAL_HB_TIMEOUT_CNT_MAX &&
        g_Xpg_GlobalVar.Xpg_Mcu.XPG_PingTime >= LOCAL_HB_REDUNDANCE)
    {
        GAgent_Printf(GAGENT_INFO, "[Local] heart beat timeout over %d times:%d S, reboot",
                    LOCAL_HB_TIMEOUT_CNT_MAX, DRV_GAgent_GetTime_S());
        DRV_GAgent_Reset();
        MCU_ResetPingTime();
    }
}

void Local_Timer(void)
{
    Local_HB_Timer();
}

/************************************************
*
*		uartbuf:	uart receive data pointer,
*		uartlen:	uart receive data length
*       return :    0 handle success 1 fail
*		Add by Alex lin 	2014-03-20
*
*************************************************/

int MCU_DispatchPacket( u8* buffer,int bufferlen )
{
    unsigned char CMD,SN,CheckSum;	       
    unsigned char WiFi_ConfigType;
    int ProdLen;
    int i;
    int ret=1;
    
    CMD = buffer[4];
    SN = buffer[5];
    CheckSum = GAgent_SetCheckSum(  buffer, (bufferlen-1) );    
    if( CheckSum!=buffer[bufferlen-1] )
    {
        return 1;
    }

    MCU_ResetPingTime();
    GAgent_Printf(GAGENT_DEBUG,"Get MCU PACKET, SN:%d, CMD=%02X, Len: %2x",SN, CMD, bufferlen);
    
    switch( CMD )
    {
        case MCU_REPORT:
            //send data to all clients.
            GAgent_Ack2Mcu( 0,SN, MCU_REPORT_ACK ); 
            MCU_SendPacket2Phone( buffer,bufferlen,0x91,0 );   
            MCU_SendPublishPacket(buffer, bufferlen);
            break;
        
        case MCU_CONFIG_WIFI:
            GAgent_Ack2Mcu( 0,SN, MCU_CONFIG_WIFI_ACK );
            WiFi_ConfigType = buffer[8];
            XPG_Wifi_Config( WiFi_ConfigType, wifiStatus);
            break;
        
        case MCU_RESET_WIFI:
            GAgent_Ack2Mcu( 0,SN, MCU_RESET_WIFI_ACK );
            WiFiReset();            
            break;

        case WIFI_PING2MCU_ACK:
            MCU_ResetPingTime();
            break;

        case MCU_CTRL_CMD_ACK:
            MCU_SendPacket2Phone( buffer,bufferlen,0x91,0 );
            break;
            /**/
        default:
            GAgent_Printf(GAGENT_INFO,"DO MCU PACKET, Invalid CMD=%02X[len: %2x]",CMD, bufferlen);
            break;
    }

    return ret;
}

#ifdef  __cplusplus
}
#endif /* end of __cplusplus */

