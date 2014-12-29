
#ifndef __MQTTLIB_H__
#define __MQTTLIB_H__

#include <stdint.h>

#include "mqttlib.h"
#include "MqttSTM.h"
#include "mqtt.h"

int Mqtt_Register2Server( mqtt_broker_handle_t *Reg_broker);
int Mqtt_Login2Server( mqtt_broker_handle_t *Reg_broker );
int Mqtt_DispatchPublishPacket( mqtt_broker_handle_t *pstMQTTBroker, u8 *packetBuffer,int packetLen );
void MQTT_handlePacket(void);

extern int DIdLen;
extern mqtt_broker_handle_t g_stMQTTBroker;

#endif // __LIBEMQTT_H__
