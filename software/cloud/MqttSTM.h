#ifndef __MQTTPUB_H__
#define __MQTTPUB_H__
#include "mqttlib.h"

int Cloud_MQTT_initSocket( mqtt_broker_handle_t* broker,int flag );
int MQTT_readPacket(uint8_t *packetBuffer, int bufferLen);
int PubMsg( mqtt_broker_handle_t* broker, const char* topic, char* Payload, int PayLen, int flag );

#endif
