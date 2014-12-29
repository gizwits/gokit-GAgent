
#ifndef __GAGENT_DRV_H_
#define __GAGENT_DRV_H_

int  DRV_OTAPacketHandle( unsigned char *buf,int Len,int socketid);
void DRV_ConAuxPrint(char *buffer, int len,int level );
void DRV_MainTimer(void);


void GAgent_WiFiStatusCheckTimer(void);


void GAgent_TimerRun(void);
extern void GAgent_TimerHandler(unsigned int alarm, void *data);

#endif


