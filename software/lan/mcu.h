
#ifndef  __HANDLE_UART_H_
#define  __HANDLE_UART_H_
#include "lan.h"

#define PASSCODE_TIME  10  //10s

void handleUartre( u8* uartbuf,int uartlen );

int MCU_DispatchPacket( u8* buffer,int bufferlen );

void MCU_SendPacket2Phone(  u8* uartbuf,int uartbufLen,unsigned char Cmd,int fd);
int Add_W2PHead( short ULen,u8*PHead,unsigned char Cmd );

void MCU_SendPacket2Cloud( u8* uartbuf,int uartbufLen );
int MCU_SendPublishPacket(u8 *uartbuf,int uartbufLen );
int MUC_DoProductKeyPacket(const u8* buf,u8* prodKey);
extern void IntoEasyLink(void);
void WiFiReset();
int GAgent_GetMCUInfo( u32 Time );
int GAgent_CheckAck( int fd,unsigned char *buf,int bufLen, u32 time );
int GAgent_Ack2Mcu(int fd, char sn,char cmd);
void UART_handlePacket();

void MCU_PasscodeTimer();

#endif
