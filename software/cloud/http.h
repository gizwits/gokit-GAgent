#ifndef __HTTP_H_
#define __HTTP_H_

#define CLOUD_HTTP_CONTENT_MAX      (512)

int Http_InitSocket( int flag );
int Http_POST( const char *host,const char *passcode,const char *mac,const unsigned char *product_key );
int Http_GET( const char *host,const char *did );
int Http_ReadSocket( int socket,char *Http_recevieBuf,int bufLen );
int Http_Response_DID( char *Http_recevieBuf,char *DID );
int Http_getdomain_port( char *Http_recevieBuf,char *domain, int *port );
int Http_Sent_Provision();
extern int GAgent_Http_Get(const char *host, const char *content, const char *content_type);
#endif

