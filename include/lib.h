/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef __LIBXPG_H_
#define __LIBXPG_H_

#ifndef NULL
#define NULL   0
#endif

#ifndef u8
#define u8 unsigned char
#endif
#ifndef u16
#define u16 unsigned short
#endif

#ifndef u32
#define u32 unsigned int
#endif



unsigned char * base64_encode(const unsigned char *src, int len, int *out_len);
unsigned char * base64_decode(const unsigned char *src, int len, int *out_len);

u16 calc_sum(void *data, u32 len);
int check_sum(void *data, u32 len);

int encodeUInt8(unsigned char input, unsigned char *output);
int encodeUInt16(unsigned short input, char *output);
int decodeInt16(const char *input, short *output);
int encodeInt32(int input, char *output);
int decodeInt32(const char *input, int *output);
void make_rand( char* a );

void GAgent_String2MAC( unsigned char* sting, unsigned char* mac);
unsigned char GAgent_SetCheckSum(  unsigned char *buf, int packLen );
unsigned char GAgent_SetSN( unsigned char *buf );

extern int GAgent_Ping_McuTick( );
extern int GAgentV4_Write2Mcu_with_p0(int fd, unsigned char cmd, unsigned char *data, unsigned short data_len);
       
extern int GAgent_GetHostByName( char *domain,char *IPAddress);
extern int Socket_sendto(int sockfd, u8 *data, int len, void *addr, int addr_size);
extern int Socket_accept(int sockfd, void *addr, int *addr_size);
extern int Socket_recvfrom(int sockfd, u8 *buffer, int len, void *addr, int *addr_size);
//char* strchr(char* s,char c);

#define GAGENT_TIMER_PERIOD 0X01
#define GAGENT_TIMER_CLOCK  0x02
int GAgent_CreateTimer(int flag, unsigned long ms, void (*psysTimerHandler)(void));
char* GAgent_strstr(const char*s1, const char*s2);

#endif

