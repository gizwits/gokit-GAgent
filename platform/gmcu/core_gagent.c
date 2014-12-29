#ifdef  __cplusplus
extern "C"{
#endif

#include "gagent.h"
#include "lib.h"
    
typedef void (*pTimerHandler)(void);

typedef struct CORE_GAGNET_Timer{
    int nextSeconds;
    int initSecods;
    pTimerHandler pfFunc;
    int flag;
} CORE_GAGENT_TIMER_S;

#define GAGENT_MAXTIMER 32

static int g_iCoreTimerInterruptTime = 0;
CORE_GAGENT_TIMER_S g_stCoreGAgentTimer[GAGENT_MAXTIMER];

int GAgent_CreateTimer(int flag, unsigned long ms, void (*pTimerHandler)(void)) 
{
    int seconds = (ms+900)/1000;
    int i;

    if ((flag != GAGENT_TIMER_PERIOD) && (flag != GAGENT_TIMER_CLOCK))
    {
        return -1;
    }

    for (i = 0; i < GAGENT_MAXTIMER; i++)
    {
        if (g_stCoreGAgentTimer[i].flag == 0)
        {
            g_stCoreGAgentTimer[i].flag = flag;
            g_stCoreGAgentTimer[i].nextSeconds = seconds;
            g_stCoreGAgentTimer[i].initSecods = seconds;
            g_stCoreGAgentTimer[i].pfFunc = pTimerHandler;

            GAgent_Printf(GAGENT_DEBUG,"create timerid:%d , flag:%d, second:%d", 
                    i, flag, seconds);
            return i;
        }
    }

    return -1;
}

void GAgent_Timer()
{
    int i;
    //GAgent_Printf(GAGENT_PACKETDUMP,"1");
    for (i = 0; i < GAGENT_MAXTIMER; i++)
    {
        if (g_stCoreGAgentTimer[i].flag != 0)
        {
            g_stCoreGAgentTimer[i].nextSeconds --;

            if (g_stCoreGAgentTimer[i].nextSeconds == 0)
            {
                g_stCoreGAgentTimer[i].pfFunc();

                if (GAGENT_TIMER_CLOCK == g_stCoreGAgentTimer[i].flag)
                {
                    GAgent_Printf(GAGENT_DEBUG,"delete timerid:%d , flag:%d, second:%d", 
                            i, g_stCoreGAgentTimer[i].flag, g_stCoreGAgentTimer[i].initSecods);
                
                    g_stCoreGAgentTimer[i].flag = 0; 
                    g_stCoreGAgentTimer[i].initSecods = 0;
                }
                else
                {
                    g_stCoreGAgentTimer[i].nextSeconds = g_stCoreGAgentTimer[i].initSecods;    
                }
            }
        }
    }
    //GAgent_Printf(GAGENT_PACKETDUMP,"2");
    
    return;
}

void GAgent_TimerRun(void)
{
    while(g_iCoreTimerInterruptTime>0)
    {
        //GAgent_Printf(GAGENT_PACKETDUMP,".");
        g_iCoreTimerInterruptTime--;
        GAgent_Timer();
    }
}


void GAgent_TimerInit(void)
{
    memset(g_stCoreGAgentTimer, 0x0, sizeof(g_stCoreGAgentTimer));
    return;
}


void GAgent_TimerHandler(unsigned int alarm, void *data)
{
    g_iCoreTimerInterruptTime++;
}

#ifdef  __cplusplus
}
#endif /* end of __cplusplus */


