#include "gagent.h"

#include "lib.h"
#include "http.h"
#include "string.h"
#include "mqtt.h"
#include "iof_import.h"

/********************************************************
 ********************************************************
 ******                      LOCAL  MACRO                       ************
 ********************************************************
 ********************************************************/
#define kCRLFNewLine     "\r\n"
#define kCRLFLineEnding  "\r\n\r\n"


/******************************************************
*
*   flag            :   1,create socket 
*                       2,provision socket
*                       3,OTA socket
*   return          :   0 success 
                        other fail
*   Add by Alex lin  --2014-09-12
*
********************************************************/
int Http_InitSocket( int flag )
{
    int ret=0;
    int iSocketId=0;
    struct sockaddr_t Msocket_address;
    char HttpServerIpAddr[32]={0};
    unsigned short port = 80;
    
    ret = GAgent_GetHostByName( HTTP_SERVER,HttpServerIpAddr );    
    if( ret!=0 )
    {
        GAgent_Printf(GAGENT_ERROR,"gethostbyname http serverIP fail ret:%d", ret);
        return 1;
    }
    
    GAgent_Printf(GAGENT_INFO,"domain:%s ip:%s",HTTP_SERVER, HttpServerIpAddr);
    if( g_Xpg_GlobalVar.http_socketid > 0 )
    {
        GAgent_Printf(GAGENT_INFO, "Close HTTP SocketID:[%d]", g_Xpg_GlobalVar.http_socketid);
        close( g_Xpg_GlobalVar.http_socketid );
        g_Xpg_GlobalVar.http_socketid=0;
    }
    
    if( (iSocketId = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
    {
        GAgent_Printf(GAGENT_ERROR," Http socket init fail");
        return 1;
    }    
    
    GAgent_Printf(GAGENT_INFO, "New Http socketID [%d]",iSocketId);        
    ret = connect_mqtt_socket(iSocketId, &Msocket_address, port, HttpServerIpAddr);

    if (ret < 0)
    {
        GAgent_Printf(GAGENT_WARNING, "http socket connect fail with:%d", ret);
        close(iSocketId);
        return 3;
    }
 

    g_Xpg_GlobalVar.http_socketid = iSocketId;  

    return 0;
}

int Http_POST( const char *host,const char *passcode,const char *mac,const unsigned char *product_key )
{
    int ret=0;
    char *postBuf=NULL;
    char *url = "/dev/devices"; 
    char Content[100]={0};
    int ContentLen=0;
    int totalLen=0;
    char *contentType="application/x-www-form-urlencoded";
    
    postBuf = (unsigned char*)malloc(400);
    if (postBuf==NULL) return 1;

    sprintf(Content,"passcode=%s&mac=%s&product_key=%s",passcode,mac,product_key);
    ContentLen=strlen(Content);
    snprintf( postBuf,400,"%s %s %s%s%s %s%s%s %d%s%s%s%s%s",
                            "POST" ,url,"HTTP/1.1",kCRLFNewLine,
                            "Host:",host,kCRLFNewLine,
														//"Connection: keepalive",kCRLFNewLine,
														//"Keep-Alive：10",kCRLFNewLine,
                            "Content-Length:",ContentLen,kCRLFNewLine,
                            "Content-Type: application/x-www-form-urlencoded",kCRLFNewLine,
                            kCRLFNewLine,
                            Content
            );
    totalLen = strlen( postBuf );
    GAgent_Printf(GAGENT_INFO,"http_post:%s %d",postBuf,totalLen);    
    ret = send( g_Xpg_GlobalVar.http_socketid,postBuf,totalLen,0 );
    GAgent_Printf(GAGENT_INFO,"http_post ret: %d",ret);    
    free( postBuf );    
    return 0;       
}

int Http_GET( const char *host,const char *did )
{
    char *getBuf=NULL;
    int totalLen=0;
    int ret=0;
    char *url = "/dev/devices/";
    getBuf = (char*)malloc( 200 );
    if( getBuf==NULL ) 
    {
        return 1;
    }
    
    memset( getBuf,0,200 );
    snprintf( getBuf,200,"%s %s%s %s%s%s %s%s%s%s%s",
                            "GET",url,did,"HTTP/1.1",kCRLFNewLine,
                            "Host:",host,kCRLFNewLine
                            "Cache-Control: no-cache",kCRLFNewLine,
                            "Content-Type: application/x-www-form-urlencoded",kCRLFLineEnding);
    totalLen =strlen( getBuf );
    ret = send( g_Xpg_GlobalVar.http_socketid, getBuf,totalLen,0 );
	GAgent_Printf(GAGENT_INFO,"Sent provision:\n %s\n", getBuf);
    free(getBuf);    

    if(ret<=0 ) 
    {
        return 1;
    }
    else
    {
        return 0;
    }    
}
int Http_ReadSocket( int socket,char *Http_recevieBuf,int bufLen )
{
    fd_set readfds;
    int i=0;
    int bytes_rcvd = 0;
    struct timeval_t tmv;
    tmv.tv_sec = 1;
	tmv.tv_usec = 0;    
    memset(Http_recevieBuf, 0, bufLen);
    
	FD_ZERO (&readfds);
	FD_SET (socket, &readfds);
    
	if(select((socket+1), &readfds, NULL, NULL, &tmv)==-1)
    {      
	    return -1;
    }
    
    if(FD_ISSET(socket, &readfds))    
    {
        bytes_rcvd = recv(socket, Http_recevieBuf, bufLen, 0 );        
		if((bytes_rcvd) <= 0)
		{
			GAgent_Printf(GAGENT_INFO,"Close HTTP Socket[%d] Now At Http_ReadSocket.", socket);
    		close( socket );
    		g_Xpg_GlobalVar.http_socketid = -1;
			return -2;
		}         
    }
    return bytes_rcvd;
}
/******************************************************
*
*   Http_recevieBuf :   http receive data
*   return          :   http response code
*   Add by Alex lin  --2014-09-11
*
********************************************************/
int Http_Response_Code( char *Http_recevieBuf )
{
    int response_code=0;
    char *p_start = NULL;
    char *p_end =NULL; 
    char re_code[10] ={0};
    
    //取回HTTP回复的状态码
    p_start = GAgent_strstr( Http_recevieBuf," ");
    p_end   = GAgent_strstr(++p_start," ");    
    if(p_end)
    {
        memcpy( re_code,p_start,(p_end-p_start) );
    }
    
    response_code = atoi(re_code); 
	GAgent_Printf( GAGENT_INFO,"\nHttp_Response_Code %d, re_code %s\n", response_code, re_code );

    return response_code;
}
int Http_Response_DID( char *Http_recevieBuf,char *DID )
{
    char *p_start = NULL;
    char *p_end =NULL;
    memset(DID,0,GAGENT_DID_LEN_MAX);
    //取回HTTP回复的DID
    p_start = GAgent_strstr( Http_recevieBuf,"did=");
    if( p_start==NULL )
        return 1;
    p_start = p_start+strlen("did=");
    memcpy(DID,p_start,GAGENT_DID_LEN);
    DID[GAGENT_DID_LEN] ='\0';             
    return 0;    
}
int Http_getdomain_port( char *Http_recevieBuf,char *domain,int *port )
{
    char *p_start = NULL;
    char *p_end =NULL;
    char Temp_port[10]={0};
    memset( domain,0,100 );
    p_start = GAgent_strstr( Http_recevieBuf,"host=");
    if( p_start==NULL ) return 1;
    p_start = p_start+strlen("host=");
    p_end = GAgent_strstr(p_start,"&");
    if( p_end==NULL )   return 1;
    memcpy( domain,p_start,( p_end-p_start ));
    domain[p_end-p_start] = '\0';
    p_start = GAgent_strstr((++p_end),"port=");
    if( p_start==NULL ) return 1;
    p_start =p_start+strlen("port=");
    p_end = GAgent_strstr( p_start,"&" ); 
    memcpy(Temp_port,p_start,( p_end-p_start));
    *port = atoi(Temp_port);
    GAgent_Printf( GAGENT_INFO,"1.domain: %s ",domain );
    GAgent_Printf( GAGENT_INFO,"2.domain port: %d ",*port );
    return 0;
}
int Http_Sent_Provision()
{
    int ret=0;
    
    if( DIdLen<=0 )
    {   
        return 1;
    }

    ret = Http_InitSocket(1);
    if( ret!=0 )
    {   
        return 1;
    }

    ret = Http_GET( HTTP_SERVER, g_Xpg_GlobalVar.DID);
    if( ret!=0 )
    {
        return 1;
    }
    
    return 0;
}

