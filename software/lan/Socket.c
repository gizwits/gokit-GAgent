#ifdef  __cplusplus
extern "C"{
#endif


#include "gagent.h"
#include "lib.h"
#include "lan.h"
#include "wifi.h"     
#include "Socket.h"
#include "mqttlib.h"
#include "mqttxpg.h"
#include "iof_import.h"

/***********          LOCAL MACRO               ***********/
#define XPG_PACKET_VERSIONLEN 4
#define SOCKET_MAXTCPCLENT 8    
#define LAN_CLIENT_MAXLIVETIME 80   /*60S, timeout*/

/***********          LOCAL VARIABLES           ***********/
typedef struct SocketClent
{
    int SocketFD;
    struct sockaddr_t TCPClientAddr;
    int TimeOut; 
} SOCKET_CLIENT_S;
static SOCKET_CLIENT_S g_stTCPSocketClients[SOCKET_MAXTCPCLENT];

/***********          GLOBAL VARIABLES           ***********/
u8 *g_stSocketRecBuffer=NULL;
int g_TCPServerFd = -1;
int g_UDPServerFd = -1;
int g_UDPBroadcastServerFd=-1;
int g_TCPSocketMaxClientFd=0;
struct sockaddr_t g_stUDPBroadcastAddr;//用于UDP广播

void Socket_ResetClientTimeout(int socketfd)
{
    int i;

    for (i = 0; i < SOCKET_MAXTCPCLENT; i++)
    {
        if (g_stTCPSocketClients[i].SocketFD == socketfd)
        {
            g_stTCPSocketClients[i].TimeOut = LAN_CLIENT_MAXLIVETIME;
            break;
        }
    }
    return;
}

void Socket_ClientTimeoutTimer(void)
{
    int i,j;

    for (i = 0; i < SOCKET_MAXTCPCLENT; i++)
    {
        if (g_stTCPSocketClients[i].TimeOut > 0)
        {
            g_stTCPSocketClients[i].TimeOut --;
            
            if (g_stTCPSocketClients[i].TimeOut ==0)
            {
                GAgent_Printf(GAGENT_DEBUG, 
                    "Close socket(%d) now[time:%08x second: %08x MS] ", 
                g_stTCPSocketClients[i].SocketFD, DRV_GAgent_GetTime_S(), 
                DRV_GAgent_GetTime_MS());
                close(g_stTCPSocketClients[i].SocketFD);
								for( j=0;j<8;j++ )
								{
										if(g_SocketLogin[j]==g_stTCPSocketClients[i].SocketFD)
											g_SocketLogin[j]=0;
								}
                g_stTCPSocketClients[i].SocketFD = -1;
								
            }
        }
    }

    return;
}


void Init_Service(void)
{
    int i;

    memset((void *)&g_stTCPSocketClients, 0x0, sizeof(g_stTCPSocketClients));

    for (i = 0; i < SOCKET_MAXTCPCLENT; i++)
    {
        g_stTCPSocketClients[i].SocketFD = -1;
        g_stTCPSocketClients[i].TimeOut = 0;
    }

    g_stSocketRecBuffer = (unsigned char *) malloc(SOCKET_RECBUFFER_LEN);
    if (g_stSocketRecBuffer == NULL)
    {
        /**/
        ;
    }        
    GAgent_CreateTimer(GAGENT_TIMER_PERIOD, 1000, Socket_ClientTimeoutTimer);
    return;    
}


int Socket_Init(void)
{
    Socket_CreateTCPServer(12416);
    Socket_CreateUDPServer(12414);
    Socket_CreateUDPBroadCastServer(2415);
    return 0;
}




/***********************************************
*
*   UdpFlag:    0 respond udp discover 
*               1 wifi send udp broadcast
*
***********************************************/
void Build_BroadCastPacket(  int UdpFlag,u8* Udp_Broadcast )
{
    int i;    
    int TempFirmwareVerLen=0;
    int len;

    TempFirmwareVerLen = g_stGAgentConfigData.FirmwareVerLen[1];      
    if( (TempFirmwareVerLen>32)||(TempFirmwareVerLen<=0) )
    {
        //当固件版本信息长度超出规定范围 hard core一个版本信息。
        Udp_Broadcast[9+Udp_Broadcast[9]+2+8] = 4;
        TempFirmwareVerLen=4;        
        memcpy( g_stGAgentConfigData.FirmwareVer,"V1.0",4);
    }
        
    //protocolver
    Udp_Broadcast[0]=0x00;
    Udp_Broadcast[1]=0x00;
    Udp_Broadcast[2]=0x00;
    Udp_Broadcast[3]=0x03;
        
    //varlen =flag(1b)+cmd(2b)+didlen(2b)+did(didLen)+maclen(2b)+mac+firwareverLen(2b)+firwarever+2+productkeyLen
    Udp_Broadcast[4] =1+2+2+DIdLen+2+6+2+TempFirmwareVerLen+2+g_productKeyLen; 
    //flag
    Udp_Broadcast[5] = 0x00;
    //cmd 
    if( UdpFlag==1 ) 
    {
        Udp_Broadcast[6] = 0x00; 
        Udp_Broadcast[7] = 0x05;
    }
    if( UdpFlag==0 ) 
    {
        Udp_Broadcast[6] = 0x00; 
        Udp_Broadcast[7] = 0x04;
    }
    // didlen
    Udp_Broadcast[8]=0x00; 
    Udp_Broadcast[9]=DIdLen; 
    //did
    for(i=0;i<DIdLen;i++)
    {
        Udp_Broadcast[10+i]=g_Xpg_GlobalVar.DID[i];
    }                
    //maclen
    Udp_Broadcast[9+Udp_Broadcast[9]+1]=0x00;
    Udp_Broadcast[9+Udp_Broadcast[9]+2]=0x06;//macLEN    
    //mac    
    for(i=0;i<6;i++)
    {
        Udp_Broadcast[9+Udp_Broadcast[9]+3+i] = Service_mac[i];
    }

    //firmwarelen
    Udp_Broadcast[9+Udp_Broadcast[9]+2+7]=0x00;
    Udp_Broadcast[9+Udp_Broadcast[9]+2+8]=TempFirmwareVerLen;//firmwareVerLen    
    //firmware
    memcpy( &Udp_Broadcast[9+Udp_Broadcast[9]+2+9],g_stGAgentConfigData.FirmwareVer,TempFirmwareVerLen );      
    len = 9+Udp_Broadcast[9]+2+8+TempFirmwareVerLen+1;
    //productkeylen
    Udp_Broadcast[len]=0x00;
    Udp_Broadcast[len+1]=g_productKeyLen;
    len=len+2;
    memcpy( &Udp_Broadcast[len],g_Xpg_GlobalVar.Xpg_Mcu.product_key,32 );
    len=len+g_productKeyLen;
}
/*****************************************
*
*   time    :   send broadcast time
*
*******************************************/
void Socket_SendBroadCastPacket( int time )
{
    int i;
    int ret;
    u8 Udp_Broadcast[200];
    
    if (g_UDPBroadcastServerFd == -1)
    {
        return;
    }
    
    Build_BroadCastPacket( 1,Udp_Broadcast );   

    for( i=0;i<time;i++)
    {
        if(g_UDPBroadcastServerFd>-1)
        {
            ret = Socket_sendto(g_UDPBroadcastServerFd, Udp_Broadcast,Udp_Broadcast[4]+5, &g_stUDPBroadcastAddr, sizeof(g_stUDPBroadcastAddr));            
        	GAgent_Printf(GAGENT_DEBUG, "Socket_SendBroadCastPacket, sendto ret %d", ret);
		}
    }
    return;
}

int Socket_CheckNewTCPClient(void)
{
    fd_set readfds, exceptfds;
    int ret;
    int newClientfd;
    int i;
    struct sockaddr_t addr;
	struct timeval_t t;
    int addrlen= sizeof(addr);
    t.tv_sec = 0;//秒
    t.tv_usec = 0;//微秒
    /*Check status on erery sockets */
    FD_ZERO(&readfds);
    FD_SET(g_TCPServerFd, &readfds);

    ret = select((g_TCPServerFd+1), &readfds, NULL, &exceptfds, &t);

    if (ret > 0)
    {
        newClientfd = Socket_accept(g_TCPServerFd, &addr, &addrlen);
        GAgent_Printf(GAGENT_DEBUG, "detected new client as %d", newClientfd);
        if (newClientfd > 0)
        {
            if(g_TCPSocketMaxClientFd<newClientfd)
            {
                g_TCPSocketMaxClientFd=(newClientfd+1); /*Select接口要求+1*/
            }
            for(i=0;i<SOCKET_MAXTCPCLENT;i++) 
            {
                if (g_stTCPSocketClients[i].SocketFD == -1) 
                {
                    memcpy(&g_stTCPSocketClients[i].TCPClientAddr, &addr, sizeof(addr));
                    g_stTCPSocketClients[i].SocketFD = newClientfd;
                    g_stTCPSocketClients[i].TimeOut = LAN_CLIENT_MAXLIVETIME;
                    GAgent_Printf(GAGENT_INFO, "new tcp client, clientid:%d[%d] [time:%08x second: %08x MS]", 
                        newClientfd, i, DRV_GAgent_GetTime_S(), DRV_GAgent_GetTime_MS() );
                    
                    return newClientfd; //返回连接ID
                }
            }
            /*此时连接个数大于系统定义的最大个数,应该关闭释放掉对应的socket资源*/           
            close( newClientfd );
        }
        else
        {
            GAgent_Printf(GAGENT_DEBUG, "Failed to accept client");
        }
    }
    return -1;
}


/*****************************************************************
*        Function Name    :    Socket_DoTCPServer
*        add by Alex     2014-02-12
*      读取TCP链接客户端的发送的数据，并进行相应的处理
*******************************************************************/
void Socket_DoTCPServer(void)
{
    int i,j;
    fd_set readfds, exceptfds;
    int recdatalen;
    int ret;
    struct timeval_t t;
    t.tv_sec = 0;//秒
    t.tv_usec = 0;//微秒
    FD_ZERO(&readfds);
    FD_ZERO(&exceptfds);

    for (i = 0; i < SOCKET_MAXTCPCLENT; i++)
    {
        if (g_stTCPSocketClients[i].SocketFD != -1)
        {
            FD_SET(g_stTCPSocketClients[i].SocketFD, &readfds);
        }
    }
    /*因为是循环处理，所以这个地方是立即返回*/
    ret = select(g_TCPSocketMaxClientFd+1, &readfds, NULL, &exceptfds, &t);
    if (ret <0)  //0 成功 -1 失败
    {
        /*没有TCP Client有数据*/
        return;
    }

    /*Read data from tcp clients and send data back */ 
    for(i=0;i<SOCKET_MAXTCPCLENT;i++) 
    {    
        if ((g_stTCPSocketClients[i].SocketFD != -1) && (FD_ISSET(g_stTCPSocketClients[i].SocketFD, &readfds)))        
        {
            memset(g_stSocketRecBuffer, 0x0, SOCKET_RECBUFFER_LEN);
            recdatalen = recv(g_stTCPSocketClients[i].SocketFD, g_stSocketRecBuffer, SOCKET_RECBUFFER_LEN, 0);
            if (recdatalen > 0)
            {
                if (recdatalen < SOCKET_RECBUFFER_LEN)
                {
                    /*一次已经获取所有的数据，处理报文*/
                    Socket_ResetClientTimeout(g_stTCPSocketClients[i].SocketFD);
                    DispatchTCPData(g_stTCPSocketClients[i].SocketFD, g_stSocketRecBuffer, recdatalen);                    
                }
                else
                {
                    GAgent_Printf(GAGENT_WARNING,"Invalid tcp packet length.");
                    /*此时recdatalen == SOCKET_RECBUFFER_LEN，还有数据需要处理。不过根据目前的协议，不可能出现这种情况的*/
                }
            }
        }
    }
    return;
}


int Socket_DispatchUDPRecvData( u8 *pData, int varlen, int messagelen, struct sockaddr_t addr, socklen_t addrLen)
{
    int ret;
    u8 Udp_ReDiscover[200] =  {0};
    char Udp_ReOnBoarding[8] = {0x00,0x00,0x00,0x03,0x03,0x00,0x00,0x02};

    if( pData[7]==0x01 )//OnBoarding
    {

        GAgent_Printf(GAGENT_INFO,"OnBoarding");

        Udp_Onboarding(pData,messagelen);
        Socket_sendto(g_UDPServerFd, Udp_ReOnBoarding, 8, &addr, sizeof(struct sockaddr_t));
        DRV_GAgent_Reset();
        return 0;
    }
    if( pData[7]==0x03 ) //Ondiscover
    {
        GAgent_Printf(GAGENT_INFO,"Ondiscover");
        Build_BroadCastPacket( 0, Udp_ReDiscover );
        ret = Socket_sendto(g_UDPServerFd, Udp_ReDiscover,Udp_ReDiscover[4]+5, &addr, sizeof(struct sockaddr_t));
        if(ret != Udp_ReDiscover[4]+5)
        {
            GAgent_Printf(GAGENT_ERROR,"send discover response fail.ret:0x%x", ret);
        }
        return 0;
    }
    GAgent_Printf(GAGENT_WARNING,"invalid udp packet [%d]", pData[7]);
    return -1;
}

void Socket_DoUDPServer(void)
{
    int readnum;
    int ret;
    fd_set readfds, exceptfds;
    struct sockaddr_t addr;
    socklen_t addrLen = sizeof(struct sockaddr_t);
    int messagelen; /*报文长度*/
    int varlen;     /*可变数据长度字段的长度*/

    struct timeval_t t;
    t.tv_sec = 0;//秒
    t.tv_usec = 0;//微秒

    if( g_UDPServerFd <=0 )
    {
        return ;
    }
    FD_ZERO(&readfds);    
    FD_SET(g_UDPServerFd, &readfds);    
    ret = select((g_UDPServerFd+1), &readfds, NULL, &exceptfds, &t);
    if(ret<0) 
    {
        return ;
    }

    memset(g_stSocketRecBuffer, 0x0, SOCKET_RECBUFFER_LEN);
    if( FD_ISSET(g_UDPServerFd, &readfds) )
    {
        readnum = Socket_recvfrom(g_UDPServerFd, g_stSocketRecBuffer, SOCKET_RECBUFFER_LEN, &addr, &addrLen);

        if (readnum <= 0)
        {
            return;
        }

        /*根据报文中的报文长度确定报文是否是一个有效的报文*/
        varlen = mqtt_num_rem_len_bytes(g_stSocketRecBuffer+4);
        //这个地方+3是因为MQTT库里面实现把 UDP flag算到messagelen里面，这里为了跟mqtt库保持一致所以加3
        messagelen = mqtt_parse_rem_len(g_stSocketRecBuffer+3); 

        if ((messagelen+varlen+XPG_PACKET_VERSIONLEN) != readnum)
        {
            /*报文长度错误*/
            GAgent_Printf(GAGENT_WARNING, "Invalid UDP packet length");
            return;
        }

        if (readnum < SOCKET_RECBUFFER_LEN)
        {
            Socket_DispatchUDPRecvData(g_stSocketRecBuffer, varlen, messagelen, addr, addrLen);
            return;
        }

        if (readnum >= SOCKET_RECBUFFER_LEN)
        {
        /*根据目前的情况，不可能出现这个问题。增加调试信息*/
            GAgent_Printf(GAGENT_WARNING, "TOO LENGTH OF UDP Packet Size.");
        ;
        }
    }
    return;               
}

/****************************************************************
*        Description        :    send data to cliets
*        pdata              :    uart pdata
*        datalength         :    uart data length 
*           cmd             :   0x12:all clients include no login 
                                0x91:just login clients
*   Add by Alex : 2014-02-13
*
****************************************************************/
int Socket_SendData2Client(u8* pdata, int datalength,unsigned char Cmd )
{
    int j,i;   
    int ret;
    if (pdata == NULL)
    {
        return -1;
    }

    //send data to all udp and tcp clients 
    for(j=0;j<8;j++)
    {
        if(g_stTCPSocketClients[j].SocketFD>0)
        {
            if(Cmd==0x91)
            {
                for( i=0;i<8;i++ )
                {
                    if((g_stTCPSocketClients[j].SocketFD == g_SocketLogin[i]) )
                    {
                        GAgent_Printf(GAGENT_INFO,"send data(len:%d) to TCP client [%d]", 
                        datalength, g_stTCPSocketClients[j].SocketFD);
                        ret = send(g_stTCPSocketClients[j].SocketFD, pdata, datalength,0);
                    }
                }
            }
            else
            {
                ret = send( g_stTCPSocketClients[j].SocketFD, pdata, datalength,0);                
            }
            
            if (ret != datalength)
            {
                /*增加Log处理*/
                ;
            }
        }
    }

    return 1;
}

#ifdef  __cplusplus
}
#endif /* end of __cplusplus */

