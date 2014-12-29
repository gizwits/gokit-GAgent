#ifdef  __cplusplus
extern "C"{
#endif

#include "gagent.h"
#include "lan.h"
#include "lib.h"
#include "iof_import.h"

static unsigned char g_SN;
int encodeUInt8(unsigned char input, unsigned char *output)
{
    output[0] = (unsigned char)input;

    return 1;
}

int decodeInt16(const char *input, short *output)
{
    *output = (short)(input[0] << 8 | (input[1] & 0xFF));
    return 2;
}

int encodeUInt16(unsigned short input, char *output)
{
    output[0] = (unsigned char)(input >> 8);
    output[1] = (unsigned char)input;

    return 2;
}

int encodeInt32(int input, char *output)
{
    output[0] = (char)(input >> 24);
    output[1] = (char)(input >> 16);
    output[2] = (char)(input >> 8);
    output[3] = (char)input;
    return 4;
}

int decodeInt32(const char *input, int *output)
{
    *output = (int)(input[0] << 24 | input[1] << 16 | input[2] << 8 | (input[3] & 0xFF));

    return 4;
}
//生成一个 passcode[2];
void make_rand(char* a)
{
	int i;
    unsigned int seed;
    srand(DRV_GAgent_GetTime_MS());
    memset(a,0,10);
	for(i=0;i<10;i++)
	{
        a[i] = 65+rand()%(90-65);
		//a[i] = rand()%255;
	}
    a[10] = '\0';
}

 
void GAgent_String2MAC( unsigned char* string, unsigned char* mac )
{
		
	int j=0,i;
	
	for(i=0;i<12;i++)	
	{
        if (string[i] >= 97)
        {
            string[i] -= 87; /*X -a +10 */
        }
        else if(string[i] >= 65)
        {
            string[i] = string[i] - 55;  /*X -A +10 */
        }
        else
        {
            string[i] = string[i] - 48;
        }
        
		if( i%2==1 )
		{
			mac[j] = string[i-1]<<4 | string[i];
			j++;
		}
	}

    return;
}
/*********************************************************
*
*       buf     :   need to get checksum buf, need not include the checksum of 
received package;
*       bufLen  :   bufLen      
*       return  :   checksum
*                   add by Alex lin 2014-09-02
**********************************************************/
unsigned char GAgent_SetCheckSum(  unsigned char *buf,int packLen )
{
    unsigned char checkSum=0;
    int i;
    
    /*i=2, remove the first two byte*/
    for( i=2;i<(packLen);i++ )
    {
        checkSum = checkSum+buf[i];
    }

    return checkSum;
}

/************************************************
*
*		buf :   data need to send to mcu
*       return :  the sn number 
*		Add by Alex lin 	2014-09-03
*
*************************************************/
unsigned char GAgent_SetSN( unsigned char *buf )
{
    if( g_SN>=255 ) g_SN=1;

    buf[MCU_SN_POS]=g_SN;
    g_SN++;        
    return buf[MCU_SN_POS];
}

char* GAgent_strstr(const char*s1, const char*s2)
{
    int n;

    if(*s2 != NULL)
    {
        while(*s1)
        {
            for(n=0;*(s1+n)==*(s2+n);n++)
            {
                if(!*(s2+n+1))
                    return (char*)s1;
            }
            s1++;
        }
        return NULL;
    }
    else
    {
        return(char*)s1; 
    }
}


#ifdef  __cplusplus
}
#endif /* end of __cplusplus */

