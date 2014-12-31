#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>    /* POSIX terminal control definitions */
#include <time.h>
#include <sys/time.h>

#include "gagent.h"
#include "x86_drv.h"

extern int g_X86_SerialWithMcu_fd;

#define SERIAL_READ_LEN     255
int main(void)
{
    unsigned char i = 0;
    int result = 0;
    unsigned char buf[SERIAL_READ_LEN] = {0};
    
    X86_Serial_Init();

    for(i = 0; i < SERIAL_READ_LEN; i++)
        buf[i] = i;
    
    X86_Serial_Tx_To_Mcu(buf, SERIAL_READ_LEN);

    memset(buf, 0, SERIAL_READ_LEN);
    X86_Serial_Read_bytes(g_X86_SerialWithMcu_fd, buf, SERIAL_READ_LEN);
    for(i = 0; i < SERIAL_READ_LEN; i++)
    {
        if(buf[i] != i)
        {
            result = -1;
            break;
        }
    }

    if(result == -1)
    {
        printf("serial test fail!byte %d, expect:0x%x, in fact:0x%x\r\n", i, i, buf[i]);
    }
    else
    {
        printf("serial test success!\r\n");
    }

    return 0;
}

