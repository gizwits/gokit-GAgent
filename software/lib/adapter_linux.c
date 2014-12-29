#ifdef  __cplusplus
extern "C"{
#endif

#include "gagent.h"
#include "lib.h"
#include "Socket.h"
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include "iof_import.h"
#include "iof_export.h"

/* zhou add adapt data hook start */
int (*DRV_GAgent_GetConfigData)(GAgent_CONFIG_S *pConfig);
int (*DRV_GAgent_SaveConfigData)(GAgent_CONFIG_S *pConfig);
void (*DRV_GAgent_GetWiFiMacAddress)(char *pMacAddress);
int (*DRV_Led_Green)(int onoff);
int (*DRV_Led_Red)(int onoff);
unsigned int (*DRV_GAgent_GetTime_MS)(void);
unsigned int (*DRV_GAgent_GetTime_S)(void);
void (*DRV_GAgent_WiFiStartScan)(void);
void (*DRV_GAgent_Reset)(void);
/* zhou add adapt data hook end */



int GAgent_GetHostByName(char *domain, char *IPAddress)
{
    int ret;
    struct hostent *hptr;
    char   *ptr, **pptr;
    char   str[32];

    memset(str, 0x0, sizeof(str));
    hptr = gethostbyname2(domain, AF_INET);
    if (hptr == NULL)
    {
        ret = -1;
        GAgent_Printf(GAGENT_DEBUG," resean : %s\n", hstrerror(h_errno));
        return ret;
    }
    pptr=hptr->h_addr_list;

    for(; *pptr!=NULL; pptr++)
    {
        inet_ntop(hptr->h_addrtype, *pptr, str, sizeof(str));
        GAgent_Printf(GAGENT_DEBUG, "Server name %s address:%s", domain, str );
    }
    inet_ntop(hptr->h_addrtype, hptr->h_addr, str, sizeof(str));
    GAgent_Printf(GAGENT_LOG,"Server name %s first address: %s", domain, str);
    memcpy(IPAddress, str, 32);
    ret = 0;

    return ret;
}

int Gagent_setsocketnonblock(int socketfd)
{
#if 0
	int flags = fcntl(socketfd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	return fcntl(socketfd, F_SETFL, flags | O_NONBLOCK);
#else
    return 0;
#endif
}

int Socket_sendto(int sockfd, u8 *data, int len, void *addr, int addr_size)
{
    return sendto(sockfd, data, len, 0, (const struct sockaddr*)addr, addr_size);
}
int Socket_accept(int sockfd, void *addr, int *addr_size)
{
    return accept(sockfd, (struct sockaddr*)addr, addr_size);
}
int Socket_recvfrom(int sockfd, u8 *buffer, int len, void *addr, int *addr_size)
{
    return recvfrom(sockfd, buffer, len, 0, (struct sockaddr *)addr, addr_size);
}
int connect_mqtt_socket(int iSocketId, struct sockaddr_t *Msocket_address, unsigned short port, char *MqttServerIpAddr)
{
    if(Gagent_setsocketnonblock(iSocketId) != 0)
    {
        GAgent_Printf(GAGENT_DEBUG,"MQTT Socket Gagent_setsocketnonblock fail.errno:%d",errno);
    }

    memset(Msocket_address, 0x0, sizeof(struct sockaddr_t));
    // Create the stuff we need to connect
    Msocket_address->sin_family = AF_INET;
    Msocket_address->sin_port= htons(port);// = port;
    Msocket_address->sin_addr.s_addr = inet_addr(MqttServerIpAddr);
    return connect(iSocketId, (struct sockaddr *)Msocket_address, sizeof(struct sockaddr_t));

}

void Socket_CreateTCPServer(int tcp_port)
{
    int bufferSize;
    struct sockaddr_t addr;

    if (g_TCPServerFd==-1)
    {
        g_TCPServerFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if(g_TCPServerFd < 0)
        {
            GAgent_Printf(GAGENT_DEBUG, "Create TCPServer failed.errno:%d", errno);
            g_TCPServerFd = -1;
            return;
        }
        if(Gagent_setsocketnonblock(g_TCPServerFd) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG,"TCP Server Gagent_setsocketnonblock fail.errno:%d", errno);
        }

        bufferSize = SOCKET_TCPSOCKET_BUFFERSIZE;
        if(setsockopt(g_TCPServerFd, SOL_SOCKET, SO_RCVBUF, &bufferSize, 4)<0)
        {
            GAgent_Printf(GAGENT_DEBUG,"TCP Server setsockopt fail.errno:%d", errno);
            return;
        }
        else
        {
        }

        bufferSize = SOCKET_TCPSOCKET_BUFFERSIZE;
        if(setsockopt(g_TCPServerFd,SOL_SOCKET,SO_SNDBUF,&bufferSize,4) < 0)
        {

            GAgent_Printf(GAGENT_DEBUG,"TCP Server setsockopt fail.errno:%d", errno);
            return;
        }
        else
        {
            ;
        }

        memset(&addr, 0x0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port=htons(tcp_port);
        addr.sin_addr.s_addr=INADDR_ANY;
        if(bind(g_TCPServerFd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG, "TCPSrever socket bind error,errno:%d", errno);
            close(g_TCPServerFd);
            g_TCPServerFd = -1;
            return;
        }

        if(listen(g_TCPServerFd, 0) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG, "TCPServer socket listen error,errno:%d", errno);
            close(g_TCPServerFd);
            g_TCPServerFd = -1;
            return;
        }

    }
    else
    {
        /**/
        ;
    }
    GAgent_Printf(GAGENT_DEBUG,"TCP Server socketid:%d on port:%d", g_TCPServerFd, tcp_port);
    return;
}

void Socket_CreateUDPServer(int udp_port)
{
    struct sockaddr_t addr;
    if (g_UDPServerFd==-1)
    {

        g_UDPServerFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if(g_UDPServerFd < 1)
        {
            GAgent_Printf(GAGENT_DEBUG, "UDPServer socket create error,errno:%d", errno);
            g_UDPServerFd = -1;
            return ;
        }
        //Only for Linux
        if(Gagent_setsocketnonblock(g_UDPServerFd) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG,"UDP Server Gagent_setsocketnonblock fail.errno:%d", errno);
        }
        memset(&addr, 0x0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(udp_port);
        addr.sin_addr.s_addr = INADDR_ANY;
        if(bind(g_UDPServerFd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG, "UDPServer socket bind error,errno:%d", errno);
            close(g_UDPServerFd);
            g_UDPServerFd = -1;
            return ;
        }

    }
    GAgent_Printf(GAGENT_DEBUG,"UDP Server socketid:%d on port:%d", g_UDPServerFd, udp_port);
    return;
}

void Socket_CreateUDPBroadCastServer( int udp_port )
{

    int udpbufsize=2;

    if( g_UDPBroadcastServerFd == -1 )
    {

        g_UDPBroadcastServerFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if(g_UDPBroadcastServerFd < 0)
        {
            GAgent_Printf(GAGENT_DEBUG, "UDP BC socket create error,errno:%d", errno);
            g_UDPBroadcastServerFd = -1;
            return ;
        }

        //Only for Linux
        if(Gagent_setsocketnonblock(g_UDPBroadcastServerFd) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG,"UDP BC Server Gagent_setsocketnonblock fail.errno:%d", errno);
        }
        g_stUDPBroadcastAddr.sin_family = AF_INET;
        g_stUDPBroadcastAddr.sin_port=htons(udp_port);
        g_stUDPBroadcastAddr.sin_addr.s_addr=htonl(INADDR_BROADCAST);

        if(setsockopt(g_UDPBroadcastServerFd, SOL_SOCKET, SO_BROADCAST, &udpbufsize,sizeof(int)) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG,"UDP BC Server setsockopt error,errno:%d", errno);
            //return;
        }
        if(bind(g_UDPBroadcastServerFd, (struct sockaddr *)&g_stUDPBroadcastAddr, sizeof(g_stUDPBroadcastAddr)) != 0)
        {
            GAgent_Printf(GAGENT_DEBUG,"UDP BC Server bind error,errno:%d", errno);
            close(g_UDPBroadcastServerFd);
            g_UDPBroadcastServerFd = -1;
            return;
        }

    }
    GAgent_Printf(GAGENT_DEBUG,"UDP BC Server socketid:%d on port:%d", g_UDPBroadcastServerFd, udp_port);
    return;
}


#ifdef  __cplusplus
}
#endif /* end of __cplusplus */
