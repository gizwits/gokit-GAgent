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
    
 
u8 g_productKey[35]={0};
int g_productKeyLen = 0;


u8 *g_busiProtocolVer; //业务逻辑协议版本号
u16 g_busiProtocolVerLen;

int (*pf_Gagent_Tx_To_Mcu)(char *data, int len);
int (*pf_Gagent_Rx_From_Mcu)(char *data, int len);

void MCU_PasscodeTimer()
{
    g_passcodeEnable=0;
    GAgent_Printf(GAGENT_INFO,"Passcode Disable.");
    
}



/*********************************************************
*
*		buf			:	uart receive data pointer
*		prodKey	    :	productKey	pointer
*		busiProtocolVer: business logic protocolVer
*		return	    :	productKey length
*		Add by Alex lin 		2014-03-20
*
***********************************************************/
int MUC_DoProductKeyPacket(const u8* buf,u8* prodKey )/*与协议不一致, 
no busiProtocolVer,  watson*/
{
	int productKeyLen=0;
	unsigned short busiProtocolVerLen=0;
    
	productKeyLen = buf[9];
	busiProtocolVerLen=buf[9+productKeyLen+1]*256+buf[9+productKeyLen+2];
    g_busiProtocolVerLen = busiProtocolVerLen;
    
	memcpy( prodKey,buf+10,productKeyLen);

    if (g_busiProtocolVer != NULL)
    {
        free( g_busiProtocolVer );
    }
    
	g_busiProtocolVer = (u8*)malloc(busiProtocolVerLen);
    if (g_busiProtocolVer == NULL)
    {
        return 0;
    }

	memcpy(g_busiProtocolVer,buf+(9+productKeyLen+2+1),busiProtocolVerLen);    	  
	
	return productKeyLen;
}

/*************************************************
*
*			FUNCTION : transtion u16 to 2 byte for mqtt remainlen
*			remainLen: src data to transtion
*			return	 : varc
*			Add by alex 2014-04-20
***************************************************/
varc Tran2varc(short remainLen)
{
	varc Tmp;
	
    if (remainLen <= 127) 
    {
        //fixed_header[0] = remainLen;	   	   
        Tmp.var[0] = remainLen;
        Tmp.varcbty = 1;
    } 
    else 
    {
        // first byte is remainder (mod) of 128, then set the MSB to indicate more bytes		 
        Tmp.var[0] = remainLen % 128;
        Tmp.var[0]=Tmp.var[0] | 0x80;
        // second byte is number of 128s          
        Tmp.var[1] = remainLen / 128;	 
        Tmp.varcbty=2;
    }
    return Tmp;
}


/*************************************************
*       FUNCTION :	send mcu cmd to cloud 
*       uartbuf	 :	uart receive buf 
*
*************************************************/
void MCU_SendPacket2Cloud( u8* uartbuf,int uartbufLen )
{
    u8 *SendPack=NULL;
    int SendPackLen=0;
    u8 *ClientId=NULL;
    int ClientIdLen=0;
    int UartP0Len=0;    
    u8 McuSendTopic[200]={0};
    
    short VarLen=0;
    varc SendVarc;
    int i;
    
    ClientIdLen = uartbuf[8]*256+uartbuf[9];
    ClientId = (u8*)malloc( ClientIdLen );
    if( ClientId==NULL )
    {
        return;
    }
    
    //protocolVer(4B)+len(2B)+cmd(2B)+phoneClientIdLen(2B)+phoneClientId+p0
    UartP0Len = uartbufLen-10-ClientIdLen;
    
    
    memset(ClientId,0,ClientIdLen);
    memcpy(ClientId,uartbuf+10,ClientIdLen);    
    
    memcpy( McuSendTopic,"dev2app/",strlen("dev2app/"));
    memcpy( McuSendTopic+strlen("dev2app/"),g_Xpg_GlobalVar.DID,strlen(g_Xpg_GlobalVar.DID));
    McuSendTopic[strlen("dev2app/")+strlen(g_Xpg_GlobalVar.DID)]='/';
    memcpy( McuSendTopic+strlen("dev2app/")+strlen(g_Xpg_GlobalVar.DID)+1,ClientId,ClientIdLen);
    
    //protocolVer(4B)+varLen(1~4B)+flag(1B)+cmd(2B)+P0
    VarLen=1+2+UartP0Len;
    SendVarc = Tran2varc(VarLen);
    SendPackLen = 4+SendVarc.varcbty+1+2+UartP0Len;
    
    SendPack = (u8*)malloc(SendPackLen);
    if( SendPack==NULL ) 
    {
        free(ClientId);
        return ;
    }
    
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
    memcpy( (SendPack+4+SendVarc.varcbty+3),uartbuf+10+ClientIdLen,UartP0Len);
    GAgent_Printf(GAGENT_INFO,"MCU data to Topic: %s",McuSendTopic);  
        
    PubMsg( &g_stMQTTBroker,McuSendTopic,SendPack,SendPackLen,0);
        
    free( SendPack );
    free( ClientId );
    
    return;
}



/***************************************************
*
*		FUNCTION	:		Into EasyLink mode 
*
****************************************************/
void IntoEasyLink()
{
	int ledflag=0;
#if (GAGENT_FEATURE_WXCHIP_EASYLINK ==  1)
	wlan_disconnect();    
    OpenEasylink2(60);
		while( g_Xpg_GlobalVar.AirLinkFlag==0) 
		{
			msleep(100);
			DRV_Led_Red(ledflag);
			DRV_Led_Green(1);
			ledflag=!ledflag;
		}
#endif
    return;
}

/***************************************
*
*   重置WiFi的 wifipasscode，wifi_ssid
*              wifi_key     Cloud_DId
*
****************************************/
void WiFiReset()
{
    memset( g_stGAgentConfigData.wifipasscode,0,16 );
    memset( g_stGAgentConfigData.wifi_ssid,0,32 );  
    memset( g_stGAgentConfigData.wifi_key,0,32 );
    memset( g_stGAgentConfigData.Cloud_DId,0,GAGENT_DID_LEN_MAX );
    
    make_rand(g_stGAgentConfigData.wifipasscode);
    //清楚标志位
    g_stGAgentConfigData.flag &=~XPG_CFG_FLAG_CONNECTED; 
    DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);
    DIdLen = 0;    
    DRV_GAgent_Reset();
    return;
}

void GAgent_SaveSSIDAndKey(char *pSSID, char *pKey)
{
	//memcpy(&wNetConfig, nwkpara, sizeof(network_InitTypeDef_st));
    memset(g_stGAgentConfigData.wifi_ssid,0,32);
    memset(g_stGAgentConfigData.wifi_key,0,32);
    
    memcpy(g_stGAgentConfigData.wifi_ssid, pSSID, 32);
    memcpy(g_stGAgentConfigData.wifi_key, pKey, 32);		
    DRV_WiFi_StationCustomModeStart(pSSID , pKey);
	g_stGAgentConfigData.flag=g_stGAgentConfigData.flag|XPG_CFG_FLAG_CONNECTED;
    DRV_GAgent_SaveConfigData(&g_stGAgentConfigData);	
      	
    GAgent_Printf(GAGENT_INFO,"Configuration is successful.");
    return;
}

void GAgent_DebugV4Packet(int flag, unsigned char *packet, int packetLen)
{
    if (flag == GAGENT_DEBUG_PACKET_SEND2MCU)
    {
        GAgent_Printf(GAGENT_DEBUG_PACKET,
                "Send to MCU Packet, SN:%d, CMD:%d, Len:%d",
                packet[MCU_SN_POS], packet[MCU_CMD_POS], packetLen);
    }
}


void MCU_SendData(unsigned char *buffer,int datalen)
{
    if (pf_Gagent_Tx_To_Mcu != NULL)
    {
        GAgent_DebugV4Packet(GAGENT_DEBUG_PACKET_SEND2MCU, buffer, datalen);
        
        pf_Gagent_Tx_To_Mcu(buffer, datalen);
    }    
    return;    
}


void Xpg_Send2AllClients( unsigned char Cmd )
{   
    
    
}

int MCU_CheckV4Packet(u8 *buffer, int readLen)
{
    int packlen;
    
    packlen = *(unsigned short *)&buffer[2];
    packlen = ntohs(packlen);
    
    packlen+=4;/*Add the first two bytes*/

    if (readLen < packlen)
    {
        GAgent_Printf(GAGENT_ERROR,"MCU_CheckV4Packet read[%d], packet[%d].", readLen, packlen);
	    return 0;
    }

    if (readLen >= packlen)
    {
        return packlen;
    }
}


int MCU_GetPacket( u8* buffer, int bufferMaxLen )
{
    int readlen;

    readlen = X86_Serial_Rx_From_Mcu( buffer, bufferMaxLen );
    if ((readlen < 4) || readlen >= bufferMaxLen)
    {
        return 0;    
    }

    return MCU_CheckV4Packet(buffer, readlen);
}

void UART_handlePacket()
{
    int PacketLen;
    PacketLen = MCU_GetPacket( g_stSocketRecBuffer, SOCKET_RECBUFFER_LEN);
    if( PacketLen>0 )
    {
        MCU_DispatchPacket( g_stSocketRecBuffer, PacketLen );
    }    
}

int GAgent_SendData2Client(char *buffer, int len)
{
    if ((buffer != NULL) && (len > 6))
    {
        MCU_DispatchPacket(buffer, len);
        return 0;
    }

    return 1;
}

#ifdef  __cplusplus
}
#endif /* end of __cplusplus */

