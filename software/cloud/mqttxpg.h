
#ifndef __MQTTLIB_H__
#define __MQTTLIB_H__

#include <stdint.h>

#include "mqttlib.h"
#include "MqttSTM.h"
#include "mqtt.h"

#define CLOUD_HB_TIMEOUT_ONESHOT        55
#define CLOUD_HB_TIMEOUT_MAX            120
#define CLOUD_HB_TIMEOUT_CNT_MAX        (CLOUD_HB_TIMEOUT_MAX/CLOUD_HB_TIMEOUT_ONESHOT)
#define CLOUD_HB_TIMEOUT_REDUNDANCE     (CLOUD_HB_TIMEOUT_MAX%55)

int Mqtt_Register2Server( mqtt_broker_handle_t *Reg_broker);
int Mqtt_Login2Server( mqtt_broker_handle_t *Reg_broker );
int Mqtt_DispatchPublishPacket( mqtt_broker_handle_t *pstMQTTBroker, u8 *packetBuffer,int packetLen );
void MQTT_handlePacket(void);

extern void Cloud_HB_Status_init(void);

extern int DIdLen;
extern mqtt_broker_handle_t g_stMQTTBroker;

#endif // __LIBEMQTT_H__
