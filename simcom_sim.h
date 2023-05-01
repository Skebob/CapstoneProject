#ifndef __SIMCOM_SIM_H__

#include "inc/UART.H"

bool SIM7000_SyncBaud(void);

bool SIM7000_Init(void);

bool SIM7000_SendCommand(const char* cmd);

void SIM7000_MQTT_PUBLISH_4INT(int val, int topicID, char* pin_str);

void SIM7000_MQTT_CONNECT(void);

bool SIM7000_MQTT_CHECK_CONNECT(void);

bool MQTT_CHECK_TOPIC_WEBCMD(const char* str, int len);

bool MQTT_CHECK_TOPIC_WEBSTATE(const char* str, int len);

bool MQTT_GET_TOPIC_WEBCMD(char* src);

void SIM7000_MQTT_PUBLISH_STATE(char* state);

#endif
