#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>    /* POSIX terminal control definitions */
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <string.h>


#include "gagent.h"
#include "lib.h"
#include "wifi.h"
#include "drv.h"
#include "Socket.h"
#include "iof_import.h"
#include "x86_drv.h"
#include "drv.h"

/*zhou add new file. some func stub*/

#define GAGENT_CONFIG_FILE      "/opt/gagent_config_tmp.config"
// 读取gagent配置信息
int X86_GetConfigData(GAgent_CONFIG_S *pConfig)
{
    int fd = open(GAGENT_CONFIG_FILE, O_RDONLY);
    if(-1 == fd)
    {
        printf("open file fail\r\n");
        return -1;
    }

    read(fd, pConfig, sizeof(GAgent_CONFIG_S));
    
    close(fd);
    return 0;
}
// 保存gagent配置信息
int X86_SaveConfigData(GAgent_CONFIG_S *pConfig)
{
    int fd = open(GAGENT_CONFIG_FILE, O_RDWR | O_CREAT);
    if(-1 == fd)
    {
        printf("open file fail\r\n");
        return -1;
    }

    write(fd, pConfig, sizeof(GAgent_CONFIG_S));
    
    close(fd);

    return 0;
}

void X86_GetWiFiMacAddress(char *pMacAddress)
{     
    int sock_mac;
    struct ifreq ifr_mac;

    if(pMacAddress == NULL)
    {
        return ;
    }
    sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_mac == -1)
    {
        perror("create socket falise...mac/n");
        return ; 
    }

    memset(&ifr_mac,0,sizeof(ifr_mac));
    strncpy(ifr_mac.ifr_name, "eth1", sizeof(ifr_mac.ifr_name)-1);
    
    if( (ioctl( sock_mac, SIOCGIFHWADDR, &ifr_mac)) < 0)
    {
        printf("mac ioctl error/n");
        return ;
    }

    pMacAddress[0] = ifr_mac.ifr_hwaddr.sa_data[0];
    pMacAddress[1] = ifr_mac.ifr_hwaddr.sa_data[1];
    pMacAddress[2] = ifr_mac.ifr_hwaddr.sa_data[2];
    pMacAddress[3] = ifr_mac.ifr_hwaddr.sa_data[3];
    pMacAddress[4] = ifr_mac.ifr_hwaddr.sa_data[4];
    pMacAddress[5] = ifr_mac.ifr_hwaddr.sa_data[5];

    close( sock_mac );
}

/* 串口初始化 */
#define X86_SERIAL_NAME			"/dev/ttyS1"
/* 9600 ;8-bit data; 1-bit stop; No parity; No hw flow control*/
#define X86_SERIAL_BAUD			B9600
int g_X86_SerialWithMcu_fd = -1;
void X86_Serial_Init(void)
{
    struct termios options;

    g_X86_SerialWithMcu_fd = open(X86_SERIAL_NAME, O_RDWR | O_NOCTTY | O_NDELAY);
    if(g_X86_SerialWithMcu_fd < 0)
    {
        GAgent_Printf(GAGENT_ERROR, "open %s fail", X86_SERIAL_NAME);
    }

    tcgetattr(g_X86_SerialWithMcu_fd, &options);
    /*
     * set baud rate
     */
    cfsetispeed(&options, X86_SERIAL_BAUD);
    cfsetospeed(&options, X86_SERIAL_BAUD);

    options.c_cflag &= ~PARENB; /* no parity*/
    options.c_cflag &= ~CSTOPB; /* 1-bit stop bit */
    options.c_cflag &= ~CSIZE; /* Mask the character size bits */
    options.c_cflag |= CS8;    /* Select 8 data bits */
    #ifdef CNEW_RTSCTS
    options.c_cflag &= ~CNEW_RTSCTS;
    #endif

    options.c_oflag &= ~(OPOST | ONLCR | OCRNL);
    options.c_iflag = 0;

    /*
     *  Enable the receiver and set local mode...
     */
    options.c_cflag |= (CLOCAL | CREAD);

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);     /* raw data */
    /*
     * set the new attr for port
     */
    tcsetattr(g_X86_SerialWithMcu_fd, TCSANOW, &options);
}


int X86_Serial_Tx_To_Mcu(char *buf, int len)
{
	return write(g_X86_SerialWithMcu_fd, buf, len);
}

int X86_Serial_Read_bytes(int fd, char *buf, int len)
{
    int ret;
    int readed = 0;
    fd_set rfds;
    struct timeval tv;

    if(buf == NULL || len == 0)
    {
        return 0;
    }    

    FD_ZERO(&rfds); // 清空串口接收端口集
    FD_SET(fd, &rfds);// 设置串口接收端口集

    tv.tv_sec = 2;
    tv.tv_usec = 200*1000;

    while(FD_ISSET(fd,& rfds))
    {
        FD_ZERO(&rfds); // 清空串口接收端口集
        FD_SET(fd, &rfds);// 设置串口接收端口集

        ret = select(fd+1,&rfds,NULL,NULL,&tv);
        if(ret < 0)
        {
            return -1;
        }

        ret = read(fd, buf + readed, len - readed);
        if(ret == 0)
            continue;
        readed += ret;
        if(readed >= len)
            return readed;
    }

    return -1;

}
/***************************************
 |--head(2B)--|--len(2B)--|--cmd(1B)--|--sn(1B)--|--flag(2B)--|--payload(xB)--|--checksum--|
 head:0xfffff
 len: len from cmd to checksum
 payload:depend on valid cmd.defined by manufactory.
 checksum:sum(len to payload)
 ***************************************/
int X86_Serial_Rx_From_Mcu(char *buf, int len)
{
    int i = 0;
    unsigned char c_data;
    short u16_data;
    short rec_len;
    int ret;

    while(1)
    {
        /* 1.sync head */
        ret = X86_Serial_Read_bytes(g_X86_SerialWithMcu_fd, &c_data, 1);
        if(ret != 1)
        {
            return -1;
        }
        if(c_data != (unsigned char)0xFF)
        {
            return -1;
        }

        ret = X86_Serial_Read_bytes(g_X86_SerialWithMcu_fd, &c_data, 1);
        if(ret != 1)
        {
            return -1;
        }
        if(c_data != 0xff)
        {
            continue;
        }
        buf[0] = buf[1] = c_data;

        /* 2.rec len */
        X86_Serial_Read_bytes(g_X86_SerialWithMcu_fd, (char *)&u16_data, 2);
        rec_len = ntohs(u16_data);
        buf[2] = (char)(u16_data&0xff);
        buf[3] = (char)((u16_data&0xff00) >> 8);

        /* 3.rec others(cmd + sn + flag + payload + checksum) */
        ret = X86_Serial_Read_bytes(g_X86_SerialWithMcu_fd, buf + 4, rec_len);
        if(ret != rec_len)
        {
            return 0;
        }

        return rec_len + 4;
        
    }
}

/* stub */
int X86_led_Green(int onoff)
{
	return 0;
}

/* stub */
int X86_led_Red(int onoff)
{
	return 0;
}

unsigned int X86_GetTime_MS(void)
{
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	return (unsigned int)(tv.tv_usec/1000);
}

unsigned int X86_GetTime_S(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (unsigned int)(tv.tv_sec);
}

void X86_WiFiStartScan(void)
{
	return ;
}

/* stub */
void DRV_WiFi_StationCustomModeStart(char *StaSsid,char *StaPass )
{
	return ;
}

/* stub */
void DRV_WiFi_SoftAPModeStart(void)
{
	return ;
}

void x86_timer_handle(int signo)
{
    switch(signo)
    {
        case SIGALRM:
            GAgent_TimerHandler(0, NULL);
            break;
        default:
            printf("invalid signo\r\n");
            break;
    }
}

void X86_Timer_Creat(int period_s, unsigned long period_us, x86_timer_callback p_callback)
{
    struct itimerval value;

    value.it_value.tv_sec = period_s;
    value.it_value.tv_usec = period_us;
    value.it_interval.tv_sec = period_s;
    value.it_interval.tv_usec = period_us;
    signal(SIGALRM, p_callback);
    setitimer(ITIMER_REAL, &value, NULL);

    return ;
}

void X86_Reset(void)
{
//    system("reboot");
    return ;
}


