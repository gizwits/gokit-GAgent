#include "iof_import.h"
#include "x86_drv.h"

void IOF_Config_hook_init(void)
{
	DRV_GAgent_GetConfigData = X86_GetConfigData;
	DRV_GAgent_SaveConfigData  = X86_SaveConfigData;
}

void IOF_General_Hook_Init(void)
{
	DRV_GAgent_GetWiFiMacAddress = X86_GetWiFiMacAddress;

    pf_Gagent_Tx_To_Mcu = X86_Serial_Tx_To_Mcu;
    pf_Gagent_Rx_From_Mcu = X86_Serial_Rx_From_Mcu;

	DRV_Led_Green = X86_led_Green;
	DRV_Led_Red = X86_led_Red;

	DRV_GAgent_GetTime_MS = X86_GetTime_MS;
	DRV_GAgent_GetTime_S = X86_GetTime_S;

	DRV_GAgent_WiFiStartScan = X86_WiFiStartScan;

    DRV_GAgent_Reset = X86_Reset;

    pf_OTA_Upgrade = X86_Ota_Upgrade;
}

