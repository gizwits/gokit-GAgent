#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stdio.h"
#include "ctype.h"
#include "gagent.h"
#include "lib.h"
#include "wifi.h"
#include "mqttxpg.h"

#include <stdio.h>        //printf()
#include <unistd.h>        //pause()
#include <signal.h>        //signal()
#include <string.h>        //memset()
#include <sys/time.h>    //struct itimerval, setitimer()

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include "x86_drv.h"

#define GAGENT_TIMER_S_1        1
#define GAGENT_TIMER_US         0
int main(void)
{
	/* zhou stub */
    /* 创建gagent进程单独运行 */
    //CoreInit();
    X86_Serial_Init();
    IOF_Config_hook_init();
	IOF_General_Hook_Init();
    GAgent_TimerInit();
    X86_Timer_Creat(GAGENT_TIMER_S_1, GAGENT_TIMER_US, x86_timer_handle);       /* 1S */
	
    GAgent_Init();

    while(1)
    {
        GAgent_TimerRun();
        GAgent_DoBusiness();
        //sleep(1);
    }
}
