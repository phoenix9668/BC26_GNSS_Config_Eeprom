
#include "nbiot.h"
#if (_NBIOT_HTTP == 1 || _NBIOT_MQTT == 1 )
//###############################################################################################################
void nbiot_callback_simcardReady(void)
{
  nbiot_printf("CALLBACK SIMCARD READY\r\n");
}
//###############################################################################################################
void nbiot_callback_simcardPinRequest(void)
{
  nbiot_printf("CALLBACK SIMCARD PIN\r\n");
}
//###############################################################################################################
void nbiot_callback_simcardPukRequest(void)
{
  nbiot_printf("CALLBACK SIMCARD PUK\r\n");
}
//###############################################################################################################
void nbiot_callback_simcardNotInserted(void)
{
  nbiot_printf("CALLBACK SIMCARD NOT DETECT\r\n");
}
//###############################################################################################################
void nbiot_callback_networkRegister(void)
{
  nbiot_printf("CALLBACK NETWORK REGISTER\r\n");
}
//###############################################################################################################
void nbiot_callback_networkUnregister(void)
{
  nbiot_printf("CALLBACK NETWORK UNREGISTER\r\n");
}
//###############################################################################################################
#if (_NBIOT_MQTT == 1)
void nbiot_callback_connected(void)
{
  nbiot_printf("CALLBACK CONNECTED, IP: %s\r\n", nbiot.mqtt.ip);
}
//###############################################################################################################
void nbiot_callback_disconnected(void)
{
  nbiot_printf("CALLBACK DISCONNECTED\r\n");
}
//###############################################################################################################
void nbiot_callback_mqttMessage(char *topic, char *message)
{
  nbiot_printf("CALLBACK GPRS MQTT TOPIC: %s   ----   MESSAGE: %s\r\n", topic, message);
}
//###############################################################################################################
void nbiot_callback_mqttDisconnect(void)
{
  nbiot_printf("CALLBACK GPRS MQTT DISCONNECT\r\n");
}
//###############################################################################################################
#endif
#endif
