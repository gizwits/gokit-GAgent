#ifndef __X86_DRV_H__
#define __X86_DRV_H__

typedef void (*x86_timer_callback)(int signo);

extern int X86_GetConfigData(GAgent_CONFIG_S *pConfig);
extern int X86_SaveConfigData(GAgent_CONFIG_S *pConfig);
extern void X86_GetWiFiMacAddress(char *pMacAddress);
extern void X86_Serial_Init(void);
extern int X86_Serial_Tx_To_Mcu(char *buf, int len);
extern int X86_Serial_Rx_From_Mcu(char *buf, int len);
extern int X86_led_Green(int onoff);
extern int X86_led_Red(int onoff);
extern unsigned int X86_GetTime_MS(void);
extern unsigned int X86_GetTime_S(void);
extern void X86_WiFiStartScan(void);
extern void DRV_WiFi_StationCustomModeStart(char *StaSsid,char *StaPass );
extern void DRV_WiFi_SoftAPModeStart(void);
extern void x86_timer_handle(int signo);
extern void X86_Timer_Creat(int period_s, unsigned long period_us,
        x86_timer_callback p_callback);
extern void X86_Reset(void);
extern int X86_Ota_Upgrade(int offset, char *buf, int len);

#endif  /* __X86_DRV_H__ */