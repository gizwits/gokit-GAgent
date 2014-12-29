
#ifndef __MQTT_PUB_H__
#define __MQTT_PUB_H__

#define MQTT_STATUS_LOGIN           (1<<1)
#define MQTT_STATUS_LOGINTOPIC1     (1<<4)
#define MQTT_STATUS_LOGINTOPIC2     (1<<5)
#define MQTT_STATUS_LOGINTOPIC3     (1<<6)
#define MQTT_STATUS_RUNNING         (1<<7)

extern int g_MQTTStatus;
#endif // __MQTT_PUB_H__

