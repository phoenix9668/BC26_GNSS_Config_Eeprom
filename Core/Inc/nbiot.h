
#ifndef _NBIOT_H_
#define _NBIOT_H_

/*
 *	Author:     Nima Askari
 *	WebSite:    https://www.github.com/NimaLTD
 *	Instagram:  https://www.instagram.com/github.NimaLTD
 *	LinkedIn:   https://www.linkedin.com/in/NimaLTD
 *	Youtube:    https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw
 */

/*
 * Version:	5.1.2
 *
 * History:
 *
 * (5.1.2): Fix read MQTT message. Add MQTT disconnect callback.
 * (5.1.2): Fix HTTP GET/POST.
 * (5.1.0): Add MQTT. 
 * (5.0.1): Fix GPRS connecting. 
 * (5.0.0):	Rewrite again. Support NONE-RTOS, RTOS V1 and RTOS V2.
 */

#include "nbiotConfig.h"
#include "atcConfig.h"
#include "atc.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "adxl362.h"
#include "adc.h"

#if (_NBIOT_DEBUG == 1)
#define nbiot_printf(...)     			printf(__VA_ARGS__)
#define nbiot_info(format,...)   		printf("[info] %s() %d "format"\r\n",__func__,__LINE__,##__VA_ARGS__)
#define nbiot_error(format,...)  		printf("[error] %s %s()%d "format"\r\n",__FILE__,__func__,__LINE__,##__VA_ARGS__)

#else
#define nbiot_printf(...)     			{};
#define nbiot_info(format,...)     	{};
#define nbiot_error(format,...)    	{};
#endif

#if (_NBIOT_ALIYUN == 1)
#define ALIYUN_MQTT_BROKER_ADDRESS "%s.iot-as-mqtt.cn-shanghai.aliyuncs.com:%d"
#define ALIYUN_MQTT_CLIENT_ID "%s|securemode=3,signmethod=hmacsha1|"
#define ALIYUN_MQTT_USERNAME "%s&%s"
#define ALIYUN_MQTT_HMACSHA1_DATA "clientId%sdeviceName%sproductKey%s"
#endif

#if (_NBIOT_DMP == 1)
#define DMP_MQTT_CLIENT_ID "%s|%s|0|0|%d"
#define DMP_MQTT_USERNAME "%s|%s"

#define DMP_TOPIC_PROPERTY_PUB						"$sys/%s/%s/property/pub"
#define DMP_TOPIC_PROPERTY_PUB_REPLY			"$sys/%s/%s/property/pub_reply"
#define DMP_TOPIC_PROPERTY_Batch					"$sys/%s/%s/property/batch"
#define DMP_TOPIC_PROPERTY_Batch_REPLY		"$sys/%s/%s/property/batch_reply"
#define DMP_TOPIC_PROPERTY_SET						"$sys/%s/%s/property/set"
#define DMP_TOPIC_PROPERTY_SET_REPLY			"$sys/%s/%s/property/set_reply"

#define DMP_TOPIC_SERVICE_PUB							"$sys/%s/%s/service/pub"
#define DMP_TOPIC_SERVICE_PUB_REPLY				"$sys/%s/%s/service/pub_reply"

#define DMP_TOPIC_SYNC_PUB								"$sys/%s/%s/sync/pub"
#define DMP_TOPIC_SYNC_PUB_REPLY					"$sys/%s/%s/sync/pub_reply"

#define DMP_TOPIC_EVENT_PUB								"$sys/%s/%s/event/pub"
#define DMP_TOPIC_EVENT_PUB_REPLY					"$sys/%s/%s/event/pub_reply"
#define DMP_TOPIC_EVENT_BATCH							"$sys/%s/%s/event/batch"
#define DMP_TOPIC_EVENT_BATCH_REPLY				"$sys/%s/%s/event/batch_reply"

#endif

typedef enum
{
  PROPERTY_PUB             				= 0x00,
  PROPERTY_PUB_REPLY             	= 0x01,
  PROPERTY_Batch              		= 0x02,
  PROPERTY_Batch_REPLY          	= 0x03,
  PROPERTY_SET           					= 0x04,
  PROPERTY_SET_REPLY           	 	= 0x05,
	
  SERVICE_PUB                     = 0x06,
  SERVICE_PUB_REPLY               = 0x07,
	
  SYNC_PUB            						= 0x08,
  SYNC_PUB_REPLY                  = 0x09,
	 
	EVENT_PUB                       = 0x0A,
	EVENT_PUB_REPLY                 = 0x0B,
	EVENT_BATCH                     = 0x0C,
	EVENT_BATCH_REPLY								= 0x0D
	
} DMP_TopicTypeDef;

//###############################################################################################################
typedef enum
{
  ACC_ARRAY1_TRANS                = 0x01,
  ACC_ARRAY2_TRANS                = 0x02,
  ACC_ARRAY3_TRANS                = 0x03,
  ACC_ARRAY4_TRANS                = 0x04,
  ACC_ARRAY5_TRANS                = 0x05,
  ACC_ARRAY6_TRANS                = 0x06,
  ACC_ARRAY7_TRANS                = 0x07,
  ACC_ARRAY8_TRANS                = 0x08,
  ACC_ARRAY9_TRANS                = 0x09,
  ACC_ARRAY10_TRANS               = 0x0A,
  ACC_ARRAY11_TRANS               = 0x0B,
  ACC_ARRAY12_TRANS               = 0x0C,
  ACC_ARRAY13_TRANS               = 0x0D,
  ACC_ARRAY14_TRANS               = 0x0E,
  ACC_ARRAY15_TRANS               = 0x0F,
  ACC_ARRAY16_TRANS               = 0x10,
	
} ACC_TransTypeDef;

typedef struct  {
	bool gnss_data_ready;
	bool gnss_trans_success;
}gnss_status_t;

typedef struct  {
	bool pregnant_data_ready;
	bool pregnant_trans_success;
	bool acc1info_trans_success;
	bool acc2info_trans_success;
	bool acc3info_trans_success;
	bool acc4info_trans_success;
	bool acc5info_trans_success;
	bool acc6info_trans_success;
	bool acc7info_trans_success;
	bool acc8info_trans_success;
	bool acc9info_trans_success;
	bool acc10info_trans_success;
	bool acc11info_trans_success;
	bool acc12info_trans_success;
	bool acc13info_trans_success;
	bool acc14info_trans_success;
	bool acc15info_trans_success;
	bool acc16info_trans_success;
	ACC_TransTypeDef acc_trans;
}pregant_status_t;

typedef struct  {
	bool freefall_data_ready;
	bool freefall_trans_success;
	bool gnss_trans_success;
}freefall_status_t;

extern gnss_status_t gnss_status;
extern pregant_status_t pregant_status;
extern freefall_status_t freefall_status;

extern __IO uint16_t mqttMessageID;
extern __IO uint8_t transProcessNum;

//###############################################################################################################
typedef struct  {
		uint16_t TCP_connectID;
		uint32_t port;
		uint8_t mqttclientIdOperator;
    char url[64];
    char clientID[128];
    char username[128];
    char password[128];
}dmp_mqtt_con_parm_t;

typedef struct  {
    char productKey[20];
    char deviceKey[36];
    char deviceSecret[36];
}dmp_mqtt_triads_t;

typedef struct
{
  char           		IMEI[17];
  char              ICCID[22];
	char           		IMSI[17];

}nbiot_dev_info_t;

#if (_NBIOT_HTTP == 1)
typedef struct
{
  bool              connect;
  bool              connected;
  bool              connectedLast;
  char              ip[16];
  uint32_t          dataLen;
  uint32_t          dataCurrent;
  int16_t           code;
  uint8_t           tcpConnection;
  uint8_t           gotData;
  uint32_t          ftpExtOffset;

}nbiot_http_t;
#endif

#if (_NBIOT_MQTT == 1)
typedef struct
{
  bool              connect;
  bool              connected;
  bool              connectedLast;
  char              ip[16];
	uint16_t          mqttMsgID;
	uint8_t           mqttQos;
	bool              mqttRetain;
  char              mqttTopic[64];
  char              mqttMessage[768];
  uint8_t           mqttData;
	bool           		mqttOpened;
  bool           		mqttConnected;
  bool              mqttConnectedLast;

}nbiot_mqtt_t;
#endif

typedef struct
{
  uint8_t           power:1;
  uint8_t           registerd:1;
  uint8_t           netReg:1;
	uint8_t           sigCon :1;
  uint8_t           netChange:1;
  uint8_t           simcardChecked:1;
  uint8_t           turnOff:1;
  uint8_t           turnOn:1;

}nbiot_status_t;

typedef struct
{
	nbiot_dev_info_t  nbiot_dev_info;
  bool              inited;
  uint8_t           lock;
	uint8_t           error;
	uint8_t           times;
	uint8_t           restart; 
  uint8_t           signal;
	uint16_t          voltage;
  nbiot_status_t    status;
  atc_t             atc;
  uint8_t           buffer[_ATC_RXSIZE - 16];
	
#if (_NBIOT_HTTP == 1)
  nbiot_http_t      http;
#endif
#if (_NBIOT_MQTT == 1)
  nbiot_mqtt_t      mqtt;
	dmp_mqtt_triads_t     dmp_mqtt_triads;
	dmp_mqtt_con_parm_t	  dmp_mqtt_con_parm;
	DMP_TopicTypeDef    dmp_topictype;
#endif

}nbiot_t;

extern nbiot_t nbiot;
//###############################################################################################################
#define         nbiot_delay(x)            atc_delay(x)
#define         nbiot_command(...)        atc_command(&nbiot.atc, __VA_ARGS__)
#define         nbiot_transmit(data,len)  atc_transmit(&nbiot.atc,data,len)
#define         nbiot_rxCallback()        atc_rxCallback(&nbiot.atc)

void            vcc_bc26_power_ctrl(bool on_off);
void            open_bc26(void);
void            reset_bc26(void);
bool            nbiot_init(void);
void            nbiot_clean(void);
void            nbiot_erase(void);
void            nbiot_loop(void);
bool            nbiot_process(void);
bool            nbiot_power(bool on_off);
bool            nbiot_lock(uint32_t timeout_ms);
void            nbiot_unlock(void);

bool            nbiot_registered(void);
bool            nbiot_getIMEI(char* string, uint8_t sizeOfString);
bool            nbiot_getIMSI(char *string, uint8_t sizeOfString);
bool            nbiot_getICCID(char *string, uint8_t sizeOfString);
uint8_t         nbiot_getVOLTAGE(void);
uint8_t         nbiot_getSignalQuality_0_to_100(void);
bool            nbiot_waitForRegister(uint8_t waitSecond);
//###############################################################################################################
bool            nbiot_connect(void);
bool            nbiot_disconnect(void);

bool            nbiot_httpInit(void);
bool            nbiot_httpSetContent(const char *content);
bool            nbiot_httpSetUserData(const char *data);
bool            nbiot_httpSendData(const char *data, uint16_t timeout_ms);
int16_t         nbiot_httpGet(const char *url, bool ssl, uint16_t timeout_ms);
int16_t         nbiot_httpPost(const char *url, bool ssl, uint16_t timeout_ms);
uint32_t        nbiot_httpDataLen(void);
uint16_t        nbiot_httpRead(uint8_t *data, uint16_t len);
bool            nbiot_httpTerminate(void);

bool            nbiot_ntpServer(char *server, int8_t time_zone_in_quarter);
bool            nbiot_ntpSyncTime(void);
bool            nbiot_ntpGetTime(char *string);

void            dmp_mqttTriadsInit(void);
void            dmp_mqttConParmGenerate(void);
bool            nbiot_mqttClientOpen(uint16_t keepAliveSec, uint16_t pkt_timeout, uint16_t retry_times, uint16_t version, uint16_t echo_mode);
bool            nbiot_mqttConnect(void);
bool            nbiot_mqttDisConnect(void);
bool            nbiot_mqttSubscribe(const char *topic, bool qos);
bool            nbiot_mqttUnSubscribe(const char *topic);
bool            nbiot_mqttPublish(void);
void            nbiot_mqttTopic(void);
bool            nbiot_mqttMessage(void);
bool            nbiot_mqttTransmit(void);
//###############################################################################################################
void            nbiot_callback_simcardReady(void);
void            nbiot_callback_simcardPinRequest(void);
void            nbiot_callback_simcardPukRequest(void);
void            nbiot_callback_simcardNotInserted(void);
void            nbiot_callback_networkRegister(void);
void            nbiot_callback_networkUnregister(void);
void            nbiot_callback_connected(void);
void            nbiot_callback_disconnected(void);
void            nbiot_callback_mqttMessage(char *topic, char *message);
void            nbiot_callback_mqttDisconnect(void);
//###############################################################################################################
#endif /* _NBIOT_H_ */
