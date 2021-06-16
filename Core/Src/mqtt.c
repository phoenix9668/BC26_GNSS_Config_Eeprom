#include "nbiot.h"
#include "gnss.h"

#if (_NBIOT_MQTT == 1)

//###############################################################################################################
void dmp_mqttTriadsInit(void)
{
	char dmp_eeprom[144];
//	sprintf(dmp_mqtt_triads->productKey, DMP_MQTT_PASSWORD, "cu1y0lj7lzt82qZn");
//	sprintf(dmp_mqtt_triads->deviceKey, DMP_MQTT_PASSWORD, "70BiH93YXzJIT6b");
//	sprintf(dmp_mqtt_triads->deviceSecret, DMP_MQTT_PASSWORD, "2AAFC60975E66B6A6B9E651D27FDCB30");
	
	for (uint8_t i=0; i < (sizeof(dmp_eeprom)/4); i++)
	{
		dmp_eeprom[4*i] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i));
		dmp_eeprom[4*i+1] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i)>>8);
		dmp_eeprom[4*i+2] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i)>>16);
		dmp_eeprom[4*i+3] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i)>>24);
	}
	for (uint8_t i=0; i < sizeof(nbiot.dmp_mqtt_triads.productKey); i++)
		nbiot.dmp_mqtt_triads.productKey[i] = dmp_eeprom[i];
	for (uint8_t i=0; i < sizeof(nbiot.dmp_mqtt_triads.deviceKey); i++)
		nbiot.dmp_mqtt_triads.deviceKey[i] = dmp_eeprom[20+i];
	for (uint8_t i=0; i < sizeof(nbiot.dmp_mqtt_triads.deviceSecret); i++)
		nbiot.dmp_mqtt_triads.deviceSecret[i] = dmp_eeprom[56+i];
	nbiot_printf("productKey = %s\n",nbiot.dmp_mqtt_triads.productKey);
	nbiot_printf("deviceKey = %s\n",nbiot.dmp_mqtt_triads.deviceKey);
	nbiot_printf("deviceSecret = %s\n",nbiot.dmp_mqtt_triads.deviceSecret);
}
//###############################################################################################################
void dmp_mqttConParmGenerate(void)
{
	int32_t status = HASH_SUCCESS;
	char InputMessage[128];
	uint8_t MAC[CRL_SHA256_SIZE];
	int32_t MACLength = 0;

	nbiot.dmp_mqtt_con_parm.TCP_connectID = 0;
  nbiot.dmp_mqtt_con_parm.port = 1883;
	nbiot.dmp_mqtt_con_parm.mqttclientIdOperator = 1;//1:Unicom; 2:Mobile; 3:Telecom; 4:guangdian

	dmp_mqttTriadsInit();
	nbiot_getIMEI(nbiot.nbiot_dev_info.IMEI,sizeof(nbiot.nbiot_dev_info.IMEI));
	nbiot_getIMSI(nbiot.nbiot_dev_info.IMSI,sizeof(nbiot.nbiot_dev_info.IMSI));
	nbiot_getICCID(nbiot.nbiot_dev_info.ICCID,sizeof(nbiot.nbiot_dev_info.ICCID));
	sprintf(InputMessage, "%s%s%s", nbiot.nbiot_dev_info.ICCID, nbiot.dmp_mqtt_triads.deviceKey, nbiot.dmp_mqtt_triads.productKey);
	nbiot_printf("InputMessage = %s\n",InputMessage);
	
	status = STM32_SHA256_HMAC_Compute((uint8_t*)InputMessage,
                                   strlen(InputMessage),
                                   (uint8_t*)nbiot.dmp_mqtt_triads.deviceSecret,
                                   strlen(nbiot.dmp_mqtt_triads.deviceSecret),
                                   (uint8_t*)MAC,
                                   &MACLength);
  if (status == HASH_SUCCESS)
  {
		memset(nbiot.dmp_mqtt_con_parm.password, 0, sizeof(nbiot.dmp_mqtt_con_parm.password));
		strncpy(nbiot.dmp_mqtt_con_parm.password, (char*)MAC, MACLength);
		for (uint8_t i=0; i < MACLength; i++)
			sprintf(nbiot.dmp_mqtt_con_parm.password + i*2, "%x%x", (MAC[i] >> 4) & 0x0F, MAC[i] & 0x0F);
	}

	sprintf(nbiot.dmp_mqtt_con_parm.url, "dmp-mqtt.cuiot.cn");
  sprintf(nbiot.dmp_mqtt_con_parm.clientID, DMP_MQTT_CLIENT_ID, nbiot.nbiot_dev_info.ICCID, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_con_parm.mqttclientIdOperator);
  sprintf(nbiot.dmp_mqtt_con_parm.username, DMP_MQTT_USERNAME, nbiot.dmp_mqtt_triads.deviceKey, nbiot.dmp_mqtt_triads.productKey);

  nbiot_printf("broker: %s\n", nbiot.dmp_mqtt_con_parm.url);
  nbiot_printf("clientID: %s\n", nbiot.dmp_mqtt_con_parm.clientID);
  nbiot_printf("username: %s\n", nbiot.dmp_mqtt_con_parm.username);
  nbiot_printf("password: %s\n", nbiot.dmp_mqtt_con_parm.password);
}
//###############################################################################################################
bool nbiot_mqttClientOpen(uint16_t keepAliveSec, uint16_t pkt_timeout, uint16_t retry_times, uint16_t version, uint16_t echo_mode)
{
	char str[64];
  if ((nbiot.mqtt.connected == false) || (*nbiot.dmp_mqtt_con_parm.url == NULL))
  {
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() connected failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() lock failed!\r\n");
    return false;
  }
  //  set dataformat
  sprintf((char*)nbiot.buffer, "AT+QMTCFG=\"dataformat\",%d,0,0\r\n",nbiot.dmp_mqtt_con_parm.TCP_connectID);
  if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  //  set keep alive time
  if (keepAliveSec > 3600)
    keepAliveSec = 3600;
  sprintf((char*)nbiot.buffer, "AT+QMTCFG=\"keepalive\",%d,%d\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, keepAliveSec);
  if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  //  set session
  sprintf((char*)nbiot.buffer, "AT+QMTCFG=\"session\",%d,1\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID);
  if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  //  set timeout
  sprintf((char*)nbiot.buffer, "AT+QMTCFG=\"timeout\",%d,%d,%d,0\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, pkt_timeout, retry_times);
  if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  //  set version
  sprintf((char*)nbiot.buffer, "AT+QMTCFG=\"version\",%d,%d\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, version);
  if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  //  set echomode
	sprintf((char*)nbiot.buffer, "AT+QMTCFG=\"echomode\",%d,%d\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, echo_mode);
	if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
	{
		nbiot_printf("[NBIOT] nbiot_mqttClientOpen() failed!\r\n");
		nbiot_unlock();
		return false;
	}
  //  open mqtt client
	sprintf((char*)nbiot.buffer, "AT+QMTOPEN=%d,\"%s\",%d\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, nbiot.dmp_mqtt_con_parm.url, nbiot.dmp_mqtt_con_parm.port);
	sprintf(str, "\r\n+QMTOPEN: %d,0\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID);
  if (nbiot_command((char*)nbiot.buffer, pkt_timeout* retry_times * 1000 , NULL, 0, 2, str, "\r\nERROR\r\n") != 1)
  {
		nbiot.mqtt.mqttOpened = false;
    nbiot_printf("[NBIOT] nbiot_mqttClientOpen() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot.mqtt.mqttOpened = true;
  nbiot_printf("[NBIOT] nbiot_mqttClientOpen() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_mqttConnect(void)
{
	char str[64];
  if ((nbiot.mqtt.connected == false) || (nbiot.mqtt.mqttOpened == false) ||(*nbiot.dmp_mqtt_con_parm.clientID == NULL))
  {
    nbiot_printf("[NBIOT] nbiot_mqttConnect() connected failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] nbiot_mqttConnect() lock failed!\r\n");
    return false;
  }
  //  connect to server
	sprintf((char*)nbiot.buffer, "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, nbiot.dmp_mqtt_con_parm.clientID, nbiot.dmp_mqtt_con_parm.username, nbiot.dmp_mqtt_con_parm.password);
	sprintf(str, "\r\n+QMTCONN: %d,0,0\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID);
  if (nbiot_command((char*)nbiot.buffer, 10000, NULL, 0, 2, str, "\r\nERROR\r\n") != 1)
  {
		nbiot.mqtt.mqttConnected = false;
    nbiot_printf("[NBIOT] nbiot_mqttConnect() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot.mqtt.mqttConnected = true;
  nbiot_printf("[NBIOT] nbiot_mqttConnect() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_mqttDisConnect(void)
{
	char str[64];
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] nbiot_mqttDisConnect() lock failed!\r\n");
    return false;
  }
	sprintf((char*)nbiot.buffer, "AT+QMTDISC=%d\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID);
	sprintf(str, "\r\n+QMTDISC: %d,0\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID);
  if (nbiot_command((char*)nbiot.buffer, 10000 , NULL, 0, 2, str, "\r\nERROR\r\n") != 1)
  {
		nbiot.mqtt.mqttConnected = false;
		nbiot.mqtt.mqttConnectedLast = false;
    nbiot_printf("[NBIOT] nbiot_mqttDisConnect() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot.mqtt.mqttConnected = false;
	nbiot.mqtt.mqttConnectedLast = false;
  nbiot_printf("[NBIOT] nbiot_mqttDisConnect() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_mqttSubscribe(const char *topic, bool qos)
{
  if (nbiot.mqtt.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_mqttSubscribe() connected failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_mqttSubscribe() lock failed!\r\n");
    return false;
  }
  sprintf((char*)nbiot.buffer, "AT+SMSUB=\"%s\",%d\r\n", topic, qos);
  if (nbiot_command((char*)nbiot.buffer, 65000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_mqttSubscribe() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] gprs_mqttSubscribe() done\r\n");
  nbiot_unlock();
  return true;   
}
//###############################################################################################################
bool nbiot_mqttUnSubscribe(const char *topic)
{
  if (nbiot.mqtt.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_mqttUnSubscribe() connected failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  { 
    nbiot_printf("[NBIOT] gprs_mqttUnSubscribe() lock failed!\r\n");
    return false;
  }
  sprintf((char*)nbiot.buffer, "AT+SMUNSUB=\"%s\"\r\n", topic);
  if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_mqttUnSubscribe() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] gprs_mqttUnSubscribe() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_mqttPublish(void)
{
	char str[64];
  if (nbiot.mqtt.connected == false)
  {
    nbiot_printf("[NBIOT] nbiot_mqttPublish() connected failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] nbiot_mqttPublish() lock failed!\r\n");
    return false;
  }
  sprintf((char*)nbiot.buffer, "AT+QMTPUB=%d,%d,%d,%d,\"%s\",\"%s\"\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, nbiot.mqtt.mqttMsgID, nbiot.mqtt.mqttQos, nbiot.mqtt.mqttRetain, nbiot.mqtt.mqttTopic, nbiot.mqtt.mqttMessage);
	sprintf(str, "\r\n+QMTPUB: %d,%d,0\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID, nbiot.mqtt.mqttMsgID);
  if (nbiot_command((char*)nbiot.buffer , 31000 , NULL, 0, 2, str, "\r\nERROR\r\n") != 1)
  {
		nbiot.mqtt.mqttConnectedLast = false;
    nbiot_printf("[NBIOT] nbiot_mqttPublish() failed!\r\n");
    nbiot_unlock();
    return false;
  }
	nbiot.mqtt.mqttConnectedLast = true;
  nbiot_printf("[NBIOT] nbiot_mqttPublish() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
void nbiot_mqttTopic(void)
{
	if(nbiot.dmp_topictype == PROPERTY_PUB)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_PROPERTY_PUB, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == PROPERTY_PUB_REPLY)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_PROPERTY_PUB_REPLY, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == PROPERTY_Batch)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_PROPERTY_Batch, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == PROPERTY_Batch_REPLY)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_PROPERTY_Batch_REPLY, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == PROPERTY_SET)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_PROPERTY_SET, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == PROPERTY_SET_REPLY)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_PROPERTY_SET_REPLY, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == SERVICE_PUB)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_SERVICE_PUB, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == SERVICE_PUB_REPLY)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_SERVICE_PUB_REPLY, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == SYNC_PUB)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_SYNC_PUB, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == SYNC_PUB_REPLY)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_SYNC_PUB_REPLY, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == EVENT_PUB)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_EVENT_PUB, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == EVENT_PUB_REPLY)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_EVENT_PUB_REPLY, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == EVENT_BATCH)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_EVENT_BATCH, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	else if(nbiot.dmp_topictype == EVENT_BATCH_REPLY)
		sprintf(nbiot.mqtt.mqttTopic, DMP_TOPIC_EVENT_BATCH_REPLY, nbiot.dmp_mqtt_triads.productKey, nbiot.dmp_mqtt_triads.deviceKey);
	nbiot_printf("Topic: %s\n", nbiot.mqtt.mqttTopic);
}
//###############################################################################################################
bool nbiot_mqttMessage(void)
{
	mqttMessageID++;
	nbiot.mqtt.mqttMsgID = mqttMessageID;
	nbiot.mqtt.mqttQos = 1;
	nbiot.mqtt.mqttRetain = 0;
	if (adc.alarmBattery == true)
	{
		sprintf(nbiot.mqtt.mqttMessage, "{\"messageId\": \"%d\",\"params\": {\"key\":\"alarm\",\"info\": [{\"key\":\"alarm_battery\",\"value\":\"true\"},{\"key\":\"alarm_free_fall\",\"value\":\"false\"}]}}", 
		nbiot.mqtt.mqttMsgID);
		nbiot.dmp_topictype = EVENT_PUB;
		nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
		return true;
	}
	if (freefall_status.freefall_data_ready == true)
	{
		if (freefall_status.gnss_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"UTC_time\",\"value\":%lf},{\"key\":\"UT_date\",\"value\":%d},{\"key\":\"signal_rssi\",\"value\":%d},{\"key\":\"battery_level\",\"value\":%d},{\"key\":\"longitude\",\"value\":%lf},{\"key\":\"longitude_direction\",\"value\":%d},{\"key\":\"latitude\",\"value\":%lf},{\"key\":\"latitude_direction\",\"value\":%d},{\"key\":\"fix_status\",\"value\":%d},{\"key\":\"sat_in_use\",\"value\":%d},{\"key\":\"altitude\",\"value\":%lf},{\"key\":\"speed\",\"value\":%lf},{\"key\":\"HDOP\",\"value\":%f}]}}", 
							nbiot.mqtt.mqttMsgID, gps_valid_data.UTC_time, gps_valid_data.UT_date, nbiot.signal, nbiot.voltage, gps_valid_data.longitude,
							gps_valid_data.longitude_direction, gps_valid_data.latitude, gps_valid_data.latitude_direction, 
							gps_valid_data.fix_status, gps_valid_data.sat_in_use, gps_valid_data.altitude, gps_valid_data.speed_knots, 
							gps_valid_data.HDOP);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (freefall_status.freefall_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, "{\"messageId\": \"%d\",\"params\": {\"key\":\"alarm\",\"info\": [{\"key\":\"alarm_battery\",\"value\":\"false\"},{\"key\":\"alarm_free_fall\",\"value\":\"true\"}]}}", 
			nbiot.mqtt.mqttMsgID);
			nbiot.dmp_topictype = EVENT_PUB;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
	}
	if (gnss_status.gnss_data_ready == true)
	{
		if (gnss_status.gnss_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"UTC_time\",\"value\":%lf},{\"key\":\"UT_date\",\"value\":%d},{\"key\":\"signal_rssi\",\"value\":%d},{\"key\":\"battery_level\",\"value\":%d},{\"key\":\"longitude\",\"value\":%lf},{\"key\":\"longitude_direction\",\"value\":%d},{\"key\":\"latitude\",\"value\":%lf},{\"key\":\"latitude_direction\",\"value\":%d},{\"key\":\"fix_status\",\"value\":%d},{\"key\":\"sat_in_use\",\"value\":%d},{\"key\":\"altitude\",\"value\":%lf},{\"key\":\"speed\",\"value\":%lf},{\"key\":\"HDOP\",\"value\":%f}]}}", 
							nbiot.mqtt.mqttMsgID, gps_valid_data.UTC_time, gps_valid_data.UT_date, nbiot.signal, nbiot.voltage, gps_valid_data.longitude,
							gps_valid_data.longitude_direction, gps_valid_data.latitude, gps_valid_data.latitude_direction, 
							gps_valid_data.fix_status, gps_valid_data.sat_in_use, gps_valid_data.altitude, gps_valid_data.speed_knots, 
							gps_valid_data.HDOP);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
	}
	if (pregant_status.pregnant_data_ready == true)
	{
		if (pregant_status.pregnant_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"IMEI\",\"value\":%s},{\"key\":\"IMSI\",\"value\":%s},{\"key\":\"ICCID\",\"value\":%s},{\"key\":\"step_stage\",\"value\":%d},{\"key\":\"step_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, nbiot.nbiot_dev_info.IMEI, nbiot.nbiot_dev_info.IMSI, nbiot.nbiot_dev_info.ICCID, step.stepStage, 
							step.stepArray[0], step.stepArray[1], step.stepArray[2], step.stepArray[3], step.stepArray[4], step.stepArray[5], 
							step.stepArray[6], step.stepArray[7], step.stepArray[8], step.stepArray[9], step.stepArray[10], step.stepArray[11], 
							step.stepArray[12], step.stepArray[13], step.stepArray[14], step.stepArray[15], step.stepArray[16], step.stepArray[17], 
							step.stepArray[18], step.stepArray[19], step.stepArray[20], step.stepArray[21], step.stepArray[22], step.stepArray[23], 
							step.stepArray[24], step.stepArray[25], step.stepArray[26], step.stepArray[27], step.stepArray[28], step.stepArray[29], 
							step.stepArray[30], step.stepArray[31], step.stepArray[32], step.stepArray[33], step.stepArray[34], step.stepArray[35]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc1info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_sum\",\"value\":%d},{\"key\":\"accelerometer_1_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, step.acc.accIndex,
							step.acc.accArray[0], step.acc.accArray[1], step.acc.accArray[2], step.acc.accArray[3], step.acc.accArray[4], 
							step.acc.accArray[5], step.acc.accArray[6], step.acc.accArray[7], step.acc.accArray[8], step.acc.accArray[9], 
							step.acc.accArray[10], step.acc.accArray[11], step.acc.accArray[12], step.acc.accArray[13], step.acc.accArray[14], 
							step.acc.accArray[15], step.acc.accArray[16], step.acc.accArray[17], step.acc.accArray[18], step.acc.accArray[19], 
							step.acc.accArray[20], step.acc.accArray[21], step.acc.accArray[22], step.acc.accArray[23], step.acc.accArray[24], 
							step.acc.accArray[25], step.acc.accArray[26], step.acc.accArray[27], step.acc.accArray[28], step.acc.accArray[29], 
							step.acc.accArray[30], step.acc.accArray[31], step.acc.accArray[32], step.acc.accArray[33], step.acc.accArray[34], 
							step.acc.accArray[35], step.acc.accArray[36], step.acc.accArray[37], step.acc.accArray[38], step.acc.accArray[39],
							step.acc.accArray[40], step.acc.accArray[41], step.acc.accArray[42], step.acc.accArray[43], step.acc.accArray[44],
							step.acc.accArray[45], step.acc.accArray[46], step.acc.accArray[47], step.acc.accArray[48], step.acc.accArray[49],
							step.acc.accArray[50], step.acc.accArray[51], step.acc.accArray[52], step.acc.accArray[53], step.acc.accArray[54],
							step.acc.accArray[55], step.acc.accArray[56], step.acc.accArray[57], step.acc.accArray[58], step.acc.accArray[59],
							step.acc.accArray[60], step.acc.accArray[61], step.acc.accArray[62], step.acc.accArray[63], step.acc.accArray[64],
							step.acc.accArray[65], step.acc.accArray[66], step.acc.accArray[67], step.acc.accArray[68], step.acc.accArray[69],
							step.acc.accArray[70], step.acc.accArray[71], step.acc.accArray[72], step.acc.accArray[73], step.acc.accArray[74],	
							step.acc.accArray[75], step.acc.accArray[76], step.acc.accArray[77], step.acc.accArray[78], step.acc.accArray[79],
							step.acc.accArray[80], step.acc.accArray[81], step.acc.accArray[82], step.acc.accArray[83], step.acc.accArray[84],	
							step.acc.accArray[85], step.acc.accArray[86], step.acc.accArray[87], step.acc.accArray[88], step.acc.accArray[89],
							step.acc.accArray[90], step.acc.accArray[91], step.acc.accArray[92], step.acc.accArray[93], step.acc.accArray[94],	
							step.acc.accArray[95]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc2info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_2_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[96], step.acc.accArray[97], step.acc.accArray[98], step.acc.accArray[99],		
							step.acc.accArray[100], step.acc.accArray[101], step.acc.accArray[102], step.acc.accArray[103], step.acc.accArray[104],	
							step.acc.accArray[105], step.acc.accArray[106], step.acc.accArray[107], step.acc.accArray[108], step.acc.accArray[109],
							step.acc.accArray[110], step.acc.accArray[111], step.acc.accArray[112], step.acc.accArray[113], step.acc.accArray[114],	
							step.acc.accArray[115], step.acc.accArray[116], step.acc.accArray[117], step.acc.accArray[118], step.acc.accArray[119],			  
	            step.acc.accArray[120], step.acc.accArray[121], step.acc.accArray[122], step.acc.accArray[123], step.acc.accArray[124],	
							step.acc.accArray[125], step.acc.accArray[126], step.acc.accArray[127], step.acc.accArray[128], step.acc.accArray[129],
							step.acc.accArray[130], step.acc.accArray[131], step.acc.accArray[132], step.acc.accArray[133], step.acc.accArray[134],	
							step.acc.accArray[135], step.acc.accArray[136], step.acc.accArray[137], step.acc.accArray[138], step.acc.accArray[139],
							step.acc.accArray[140], step.acc.accArray[141], step.acc.accArray[142], step.acc.accArray[143], step.acc.accArray[144],	
							step.acc.accArray[145], step.acc.accArray[146], step.acc.accArray[147], step.acc.accArray[148], step.acc.accArray[149],
							step.acc.accArray[150], step.acc.accArray[151], step.acc.accArray[152], step.acc.accArray[153], step.acc.accArray[154],	
							step.acc.accArray[155], step.acc.accArray[156], step.acc.accArray[157], step.acc.accArray[158], step.acc.accArray[159],
							step.acc.accArray[160], step.acc.accArray[161], step.acc.accArray[162], step.acc.accArray[163], step.acc.accArray[164],	
							step.acc.accArray[165], step.acc.accArray[166], step.acc.accArray[167], step.acc.accArray[168], step.acc.accArray[169],
							step.acc.accArray[170], step.acc.accArray[171], step.acc.accArray[172], step.acc.accArray[173], step.acc.accArray[174],	
							step.acc.accArray[175], step.acc.accArray[176], step.acc.accArray[177], step.acc.accArray[178], step.acc.accArray[179],
							step.acc.accArray[180], step.acc.accArray[181], step.acc.accArray[182], step.acc.accArray[183], step.acc.accArray[184],	
							step.acc.accArray[185], step.acc.accArray[186], step.acc.accArray[187], step.acc.accArray[188], step.acc.accArray[189],
							step.acc.accArray[190], step.acc.accArray[191]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc3info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_3_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID,  
              step.acc.accArray[192], step.acc.accArray[193], step.acc.accArray[194], 
							step.acc.accArray[195], step.acc.accArray[196], step.acc.accArray[197], step.acc.accArray[198], step.acc.accArray[199], 
							step.acc.accArray[200], step.acc.accArray[201], step.acc.accArray[202], step.acc.accArray[203], step.acc.accArray[204], 
							step.acc.accArray[205], step.acc.accArray[206], step.acc.accArray[207], step.acc.accArray[208], step.acc.accArray[209], 
							step.acc.accArray[210], step.acc.accArray[211], step.acc.accArray[212], step.acc.accArray[213], step.acc.accArray[214], 
							step.acc.accArray[215], step.acc.accArray[216], step.acc.accArray[217], step.acc.accArray[218], step.acc.accArray[219], 
							step.acc.accArray[220], step.acc.accArray[221], step.acc.accArray[222], step.acc.accArray[223], step.acc.accArray[224], 
							step.acc.accArray[225], step.acc.accArray[226], step.acc.accArray[227], step.acc.accArray[228], step.acc.accArray[229], 
							step.acc.accArray[230], step.acc.accArray[231], step.acc.accArray[232], step.acc.accArray[233], step.acc.accArray[234], 
							step.acc.accArray[235], step.acc.accArray[236], step.acc.accArray[237], step.acc.accArray[238], step.acc.accArray[239],			  
			        step.acc.accArray[240], step.acc.accArray[241], step.acc.accArray[242], step.acc.accArray[243], step.acc.accArray[244],
							step.acc.accArray[245], step.acc.accArray[246], step.acc.accArray[247], step.acc.accArray[248], step.acc.accArray[249],
							step.acc.accArray[250], step.acc.accArray[251], step.acc.accArray[252], step.acc.accArray[253], step.acc.accArray[254],
							step.acc.accArray[255], step.acc.accArray[256], step.acc.accArray[257], step.acc.accArray[258], step.acc.accArray[259],
							step.acc.accArray[260], step.acc.accArray[261], step.acc.accArray[262], step.acc.accArray[263], step.acc.accArray[264],
							step.acc.accArray[265], step.acc.accArray[266], step.acc.accArray[267], step.acc.accArray[268], step.acc.accArray[269],
							step.acc.accArray[270], step.acc.accArray[271], step.acc.accArray[272], step.acc.accArray[273], step.acc.accArray[274],	
							step.acc.accArray[275], step.acc.accArray[276], step.acc.accArray[277], step.acc.accArray[278], step.acc.accArray[279],
							step.acc.accArray[280], step.acc.accArray[281], step.acc.accArray[282], step.acc.accArray[283], step.acc.accArray[284],	
							step.acc.accArray[285], step.acc.accArray[286], step.acc.accArray[287]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc4info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_4_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID,  
              step.acc.accArray[288], step.acc.accArray[289], 
							step.acc.accArray[290], step.acc.accArray[291], step.acc.accArray[292], step.acc.accArray[293], step.acc.accArray[294], 
							step.acc.accArray[295], step.acc.accArray[296], step.acc.accArray[297], step.acc.accArray[298], step.acc.accArray[299], 
							step.acc.accArray[300], step.acc.accArray[301], step.acc.accArray[302], step.acc.accArray[303], step.acc.accArray[304], 
							step.acc.accArray[305], step.acc.accArray[306], step.acc.accArray[307], step.acc.accArray[308], step.acc.accArray[309], 
							step.acc.accArray[310], step.acc.accArray[311], step.acc.accArray[312], step.acc.accArray[313], step.acc.accArray[314], 
							step.acc.accArray[315], step.acc.accArray[316], step.acc.accArray[317], step.acc.accArray[318], step.acc.accArray[319], 
							step.acc.accArray[320], step.acc.accArray[321], step.acc.accArray[322], step.acc.accArray[323], step.acc.accArray[324], 
							step.acc.accArray[325], step.acc.accArray[326], step.acc.accArray[327], step.acc.accArray[328], step.acc.accArray[329], 
							step.acc.accArray[330], step.acc.accArray[331], step.acc.accArray[332], step.acc.accArray[333], step.acc.accArray[334], 
							step.acc.accArray[335], step.acc.accArray[336], step.acc.accArray[337], step.acc.accArray[338], step.acc.accArray[339],			  
			        step.acc.accArray[340], step.acc.accArray[341], step.acc.accArray[342], step.acc.accArray[343], step.acc.accArray[344],
							step.acc.accArray[345], step.acc.accArray[346], step.acc.accArray[347], step.acc.accArray[348], step.acc.accArray[349],
							step.acc.accArray[350], step.acc.accArray[351], step.acc.accArray[352], step.acc.accArray[353], step.acc.accArray[354],
							step.acc.accArray[355], step.acc.accArray[356], step.acc.accArray[357], step.acc.accArray[358], step.acc.accArray[359],
							step.acc.accArray[360], step.acc.accArray[361], step.acc.accArray[362], step.acc.accArray[363], step.acc.accArray[364],
							step.acc.accArray[365], step.acc.accArray[366], step.acc.accArray[367], step.acc.accArray[368], step.acc.accArray[369],
							step.acc.accArray[370], step.acc.accArray[371], step.acc.accArray[372], step.acc.accArray[373], step.acc.accArray[374],	
							step.acc.accArray[375], step.acc.accArray[376], step.acc.accArray[377], step.acc.accArray[378], step.acc.accArray[379],
							step.acc.accArray[380], step.acc.accArray[381], step.acc.accArray[382], step.acc.accArray[383]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc5info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_5_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID,  
              step.acc.accArray[384], 
              step.acc.accArray[385], step.acc.accArray[386], step.acc.accArray[387], step.acc.accArray[388], step.acc.accArray[389], 
							step.acc.accArray[390], step.acc.accArray[391], step.acc.accArray[392], step.acc.accArray[393], step.acc.accArray[394], 
							step.acc.accArray[395], step.acc.accArray[396], step.acc.accArray[397], step.acc.accArray[398], step.acc.accArray[399], 
							step.acc.accArray[400], step.acc.accArray[401], step.acc.accArray[402], step.acc.accArray[403], step.acc.accArray[404], 
							step.acc.accArray[405], step.acc.accArray[406], step.acc.accArray[407], step.acc.accArray[408], step.acc.accArray[409], 
							step.acc.accArray[410], step.acc.accArray[411], step.acc.accArray[412], step.acc.accArray[413], step.acc.accArray[414], 
							step.acc.accArray[415], step.acc.accArray[416], step.acc.accArray[417], step.acc.accArray[418], step.acc.accArray[419], 
							step.acc.accArray[420], step.acc.accArray[421], step.acc.accArray[422], step.acc.accArray[423], step.acc.accArray[424], 
							step.acc.accArray[425], step.acc.accArray[426], step.acc.accArray[427], step.acc.accArray[428], step.acc.accArray[429], 
							step.acc.accArray[430], step.acc.accArray[431], step.acc.accArray[432], step.acc.accArray[433], step.acc.accArray[434], 
							step.acc.accArray[435], step.acc.accArray[436], step.acc.accArray[437], step.acc.accArray[438], step.acc.accArray[439],			  
			        step.acc.accArray[440], step.acc.accArray[441], step.acc.accArray[442], step.acc.accArray[443], step.acc.accArray[444],
							step.acc.accArray[445], step.acc.accArray[446], step.acc.accArray[447], step.acc.accArray[448], step.acc.accArray[449],
							step.acc.accArray[450], step.acc.accArray[451], step.acc.accArray[452], step.acc.accArray[453], step.acc.accArray[454],
							step.acc.accArray[455], step.acc.accArray[456], step.acc.accArray[457], step.acc.accArray[458], step.acc.accArray[459],
							step.acc.accArray[460], step.acc.accArray[461], step.acc.accArray[462], step.acc.accArray[463], step.acc.accArray[464],
							step.acc.accArray[465], step.acc.accArray[466], step.acc.accArray[467], step.acc.accArray[468], step.acc.accArray[469],
							step.acc.accArray[470], step.acc.accArray[471], step.acc.accArray[472], step.acc.accArray[473], step.acc.accArray[474],	
							step.acc.accArray[475], step.acc.accArray[476], step.acc.accArray[477], step.acc.accArray[478], step.acc.accArray[479]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc6info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_6_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
              step.acc.accArray[480], step.acc.accArray[481], step.acc.accArray[482], step.acc.accArray[483], step.acc.accArray[484],
              step.acc.accArray[485], step.acc.accArray[486], step.acc.accArray[487], step.acc.accArray[488], step.acc.accArray[489], 
							step.acc.accArray[490], step.acc.accArray[491], step.acc.accArray[492], step.acc.accArray[493], step.acc.accArray[494], 
							step.acc.accArray[495], step.acc.accArray[496], step.acc.accArray[497], step.acc.accArray[498], step.acc.accArray[499], 
							step.acc.accArray[500], step.acc.accArray[501], step.acc.accArray[502], step.acc.accArray[503], step.acc.accArray[504], 
							step.acc.accArray[505], step.acc.accArray[506], step.acc.accArray[507], step.acc.accArray[508], step.acc.accArray[509], 
							step.acc.accArray[510], step.acc.accArray[511], step.acc.accArray[512], step.acc.accArray[513], step.acc.accArray[514], 
							step.acc.accArray[515], step.acc.accArray[516], step.acc.accArray[517], step.acc.accArray[518], step.acc.accArray[519], 
							step.acc.accArray[520], step.acc.accArray[521], step.acc.accArray[522], step.acc.accArray[523], step.acc.accArray[524], 
							step.acc.accArray[525], step.acc.accArray[526], step.acc.accArray[527], step.acc.accArray[528], step.acc.accArray[529], 
							step.acc.accArray[530], step.acc.accArray[531], step.acc.accArray[532], step.acc.accArray[533], step.acc.accArray[534], 
							step.acc.accArray[535], step.acc.accArray[536], step.acc.accArray[537], step.acc.accArray[538], step.acc.accArray[539],			  
			        step.acc.accArray[540], step.acc.accArray[541], step.acc.accArray[542], step.acc.accArray[543], step.acc.accArray[544],
							step.acc.accArray[545], step.acc.accArray[546], step.acc.accArray[547], step.acc.accArray[548], step.acc.accArray[549],
							step.acc.accArray[550], step.acc.accArray[551], step.acc.accArray[552], step.acc.accArray[553], step.acc.accArray[554],
							step.acc.accArray[555], step.acc.accArray[556], step.acc.accArray[557], step.acc.accArray[558], step.acc.accArray[559],
							step.acc.accArray[560], step.acc.accArray[561], step.acc.accArray[562], step.acc.accArray[563], step.acc.accArray[564],
							step.acc.accArray[565], step.acc.accArray[566], step.acc.accArray[567], step.acc.accArray[568], step.acc.accArray[569],
							step.acc.accArray[570], step.acc.accArray[571], step.acc.accArray[572], step.acc.accArray[573], step.acc.accArray[574],	
							step.acc.accArray[575]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc7info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_7_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[576], step.acc.accArray[577], step.acc.accArray[578], step.acc.accArray[579], 
              step.acc.accArray[580], step.acc.accArray[581], step.acc.accArray[582], step.acc.accArray[583], step.acc.accArray[584],
              step.acc.accArray[585], step.acc.accArray[586], step.acc.accArray[587], step.acc.accArray[588], step.acc.accArray[589], 
							step.acc.accArray[590], step.acc.accArray[591], step.acc.accArray[592], step.acc.accArray[593], step.acc.accArray[594], 
							step.acc.accArray[595], step.acc.accArray[596], step.acc.accArray[597], step.acc.accArray[598], step.acc.accArray[599], 
							step.acc.accArray[600], step.acc.accArray[601], step.acc.accArray[602], step.acc.accArray[603], step.acc.accArray[604], 
							step.acc.accArray[605], step.acc.accArray[606], step.acc.accArray[607], step.acc.accArray[608], step.acc.accArray[609], 
							step.acc.accArray[610], step.acc.accArray[611], step.acc.accArray[612], step.acc.accArray[613], step.acc.accArray[614], 
							step.acc.accArray[615], step.acc.accArray[616], step.acc.accArray[617], step.acc.accArray[618], step.acc.accArray[619], 
							step.acc.accArray[620], step.acc.accArray[621], step.acc.accArray[622], step.acc.accArray[623], step.acc.accArray[624], 
							step.acc.accArray[625], step.acc.accArray[626], step.acc.accArray[627], step.acc.accArray[628], step.acc.accArray[629], 
							step.acc.accArray[630], step.acc.accArray[631], step.acc.accArray[632], step.acc.accArray[633], step.acc.accArray[634], 
							step.acc.accArray[635], step.acc.accArray[636], step.acc.accArray[637], step.acc.accArray[638], step.acc.accArray[639],			  
			        step.acc.accArray[640], step.acc.accArray[641], step.acc.accArray[642], step.acc.accArray[643], step.acc.accArray[644],
							step.acc.accArray[645], step.acc.accArray[646], step.acc.accArray[647], step.acc.accArray[648], step.acc.accArray[649],
							step.acc.accArray[650], step.acc.accArray[651], step.acc.accArray[652], step.acc.accArray[653], step.acc.accArray[654],
							step.acc.accArray[655], step.acc.accArray[656], step.acc.accArray[657], step.acc.accArray[658], step.acc.accArray[659],
							step.acc.accArray[660], step.acc.accArray[661], step.acc.accArray[662], step.acc.accArray[663], step.acc.accArray[664],
							step.acc.accArray[665], step.acc.accArray[666], step.acc.accArray[667], step.acc.accArray[668], step.acc.accArray[669],
							step.acc.accArray[670], step.acc.accArray[671]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc8info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_8_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[672], step.acc.accArray[673], step.acc.accArray[674], 
			        step.acc.accArray[675], step.acc.accArray[676], step.acc.accArray[677], step.acc.accArray[678], step.acc.accArray[679], 
              step.acc.accArray[680], step.acc.accArray[681], step.acc.accArray[682], step.acc.accArray[683], step.acc.accArray[684],
              step.acc.accArray[685], step.acc.accArray[686], step.acc.accArray[687], step.acc.accArray[688], step.acc.accArray[689], 
							step.acc.accArray[690], step.acc.accArray[691], step.acc.accArray[692], step.acc.accArray[693], step.acc.accArray[694], 
							step.acc.accArray[695], step.acc.accArray[696], step.acc.accArray[697], step.acc.accArray[698], step.acc.accArray[699], 
							step.acc.accArray[700], step.acc.accArray[701], step.acc.accArray[702], step.acc.accArray[703], step.acc.accArray[704], 
							step.acc.accArray[705], step.acc.accArray[706], step.acc.accArray[707], step.acc.accArray[708], step.acc.accArray[709], 
							step.acc.accArray[710], step.acc.accArray[711], step.acc.accArray[712], step.acc.accArray[713], step.acc.accArray[714], 
							step.acc.accArray[715], step.acc.accArray[716], step.acc.accArray[717], step.acc.accArray[718], step.acc.accArray[719], 
							step.acc.accArray[720], step.acc.accArray[721], step.acc.accArray[722], step.acc.accArray[723], step.acc.accArray[724], 
							step.acc.accArray[725], step.acc.accArray[726], step.acc.accArray[727], step.acc.accArray[728], step.acc.accArray[729], 
							step.acc.accArray[730], step.acc.accArray[731], step.acc.accArray[732], step.acc.accArray[733], step.acc.accArray[734], 
							step.acc.accArray[735], step.acc.accArray[736], step.acc.accArray[737], step.acc.accArray[738], step.acc.accArray[739],			  
			        step.acc.accArray[740], step.acc.accArray[741], step.acc.accArray[742], step.acc.accArray[743], step.acc.accArray[744],
							step.acc.accArray[745], step.acc.accArray[746], step.acc.accArray[747], step.acc.accArray[748], step.acc.accArray[749],
							step.acc.accArray[750], step.acc.accArray[751], step.acc.accArray[752], step.acc.accArray[753], step.acc.accArray[754],
							step.acc.accArray[755], step.acc.accArray[756], step.acc.accArray[757], step.acc.accArray[758], step.acc.accArray[759],
							step.acc.accArray[760], step.acc.accArray[761], step.acc.accArray[762], step.acc.accArray[763], step.acc.accArray[764],
							step.acc.accArray[765], step.acc.accArray[766], step.acc.accArray[767]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc9info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_9_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[768], step.acc.accArray[769], 
              step.acc.accArray[770], step.acc.accArray[771], step.acc.accArray[772], step.acc.accArray[773], step.acc.accArray[774], 
			        step.acc.accArray[775], step.acc.accArray[776], step.acc.accArray[777], step.acc.accArray[778], step.acc.accArray[779], 
              step.acc.accArray[780], step.acc.accArray[781], step.acc.accArray[782], step.acc.accArray[783], step.acc.accArray[784],
              step.acc.accArray[785], step.acc.accArray[786], step.acc.accArray[787], step.acc.accArray[788], step.acc.accArray[789], 
							step.acc.accArray[790], step.acc.accArray[791], step.acc.accArray[792], step.acc.accArray[793], step.acc.accArray[794], 
							step.acc.accArray[795], step.acc.accArray[796], step.acc.accArray[797], step.acc.accArray[798], step.acc.accArray[799], 
							step.acc.accArray[800], step.acc.accArray[801], step.acc.accArray[802], step.acc.accArray[803], step.acc.accArray[804], 
							step.acc.accArray[805], step.acc.accArray[806], step.acc.accArray[807], step.acc.accArray[808], step.acc.accArray[809], 
							step.acc.accArray[810], step.acc.accArray[811], step.acc.accArray[812], step.acc.accArray[813], step.acc.accArray[814], 
							step.acc.accArray[815], step.acc.accArray[816], step.acc.accArray[817], step.acc.accArray[818], step.acc.accArray[819], 
							step.acc.accArray[820], step.acc.accArray[821], step.acc.accArray[822], step.acc.accArray[823], step.acc.accArray[824], 
							step.acc.accArray[825], step.acc.accArray[826], step.acc.accArray[827], step.acc.accArray[828], step.acc.accArray[829], 
							step.acc.accArray[830], step.acc.accArray[831], step.acc.accArray[832], step.acc.accArray[833], step.acc.accArray[834], 
							step.acc.accArray[835], step.acc.accArray[836], step.acc.accArray[837], step.acc.accArray[838], step.acc.accArray[839],			  
			        step.acc.accArray[840], step.acc.accArray[841], step.acc.accArray[842], step.acc.accArray[843], step.acc.accArray[844],
							step.acc.accArray[845], step.acc.accArray[846], step.acc.accArray[847], step.acc.accArray[848], step.acc.accArray[849],
							step.acc.accArray[850], step.acc.accArray[851], step.acc.accArray[852], step.acc.accArray[853], step.acc.accArray[854],
							step.acc.accArray[855], step.acc.accArray[856], step.acc.accArray[857], step.acc.accArray[858], step.acc.accArray[859],
							step.acc.accArray[860], step.acc.accArray[861], step.acc.accArray[862], step.acc.accArray[863]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc10info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_10_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[864], 
			        step.acc.accArray[865], step.acc.accArray[866], step.acc.accArray[867], step.acc.accArray[868], step.acc.accArray[869], 
              step.acc.accArray[870], step.acc.accArray[871], step.acc.accArray[872], step.acc.accArray[873], step.acc.accArray[874], 
			        step.acc.accArray[875], step.acc.accArray[876], step.acc.accArray[877], step.acc.accArray[878], step.acc.accArray[879], 
              step.acc.accArray[880], step.acc.accArray[881], step.acc.accArray[882], step.acc.accArray[883], step.acc.accArray[884],
              step.acc.accArray[885], step.acc.accArray[886], step.acc.accArray[887], step.acc.accArray[888], step.acc.accArray[889], 
							step.acc.accArray[890], step.acc.accArray[891], step.acc.accArray[892], step.acc.accArray[893], step.acc.accArray[894], 
							step.acc.accArray[895], step.acc.accArray[896], step.acc.accArray[897], step.acc.accArray[898], step.acc.accArray[899], 
							step.acc.accArray[900], step.acc.accArray[901], step.acc.accArray[902], step.acc.accArray[903], step.acc.accArray[904], 
							step.acc.accArray[905], step.acc.accArray[906], step.acc.accArray[907], step.acc.accArray[908], step.acc.accArray[909], 
							step.acc.accArray[910], step.acc.accArray[911], step.acc.accArray[912], step.acc.accArray[913], step.acc.accArray[914], 
							step.acc.accArray[915], step.acc.accArray[916], step.acc.accArray[917], step.acc.accArray[918], step.acc.accArray[919], 
							step.acc.accArray[920], step.acc.accArray[921], step.acc.accArray[922], step.acc.accArray[923], step.acc.accArray[924], 
							step.acc.accArray[925], step.acc.accArray[926], step.acc.accArray[927], step.acc.accArray[928], step.acc.accArray[929], 
							step.acc.accArray[930], step.acc.accArray[931], step.acc.accArray[932], step.acc.accArray[933], step.acc.accArray[934], 
							step.acc.accArray[935], step.acc.accArray[936], step.acc.accArray[937], step.acc.accArray[938], step.acc.accArray[939],			  
			        step.acc.accArray[940], step.acc.accArray[941], step.acc.accArray[942], step.acc.accArray[943], step.acc.accArray[944],
							step.acc.accArray[945], step.acc.accArray[946], step.acc.accArray[947], step.acc.accArray[948], step.acc.accArray[949],
							step.acc.accArray[950], step.acc.accArray[951], step.acc.accArray[952], step.acc.accArray[953], step.acc.accArray[954],
							step.acc.accArray[955], step.acc.accArray[956], step.acc.accArray[957], step.acc.accArray[958], step.acc.accArray[959]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc11info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_11_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[960], step.acc.accArray[961], step.acc.accArray[962], step.acc.accArray[963], step.acc.accArray[964], 
			        step.acc.accArray[965], step.acc.accArray[966], step.acc.accArray[967], step.acc.accArray[968], step.acc.accArray[969], 
              step.acc.accArray[970], step.acc.accArray[971], step.acc.accArray[972], step.acc.accArray[973], step.acc.accArray[974], 
			        step.acc.accArray[975], step.acc.accArray[976], step.acc.accArray[977], step.acc.accArray[978], step.acc.accArray[979], 
              step.acc.accArray[980], step.acc.accArray[981], step.acc.accArray[982], step.acc.accArray[983], step.acc.accArray[984],
              step.acc.accArray[985], step.acc.accArray[986], step.acc.accArray[987], step.acc.accArray[988], step.acc.accArray[989], 
							step.acc.accArray[990], step.acc.accArray[991], step.acc.accArray[992], step.acc.accArray[993], step.acc.accArray[994], 
							step.acc.accArray[995], step.acc.accArray[996], step.acc.accArray[997], step.acc.accArray[998], step.acc.accArray[999], 
							step.acc.accArray[1000], step.acc.accArray[1001], step.acc.accArray[1002], step.acc.accArray[1003], step.acc.accArray[1004], 
							step.acc.accArray[1005], step.acc.accArray[1006], step.acc.accArray[1007], step.acc.accArray[1008], step.acc.accArray[1009], 
							step.acc.accArray[1010], step.acc.accArray[1011], step.acc.accArray[1012], step.acc.accArray[1013], step.acc.accArray[1014], 
							step.acc.accArray[1015], step.acc.accArray[1016], step.acc.accArray[1017], step.acc.accArray[1018], step.acc.accArray[1019], 
							step.acc.accArray[1020], step.acc.accArray[1021], step.acc.accArray[1022], step.acc.accArray[1023], step.acc.accArray[1024], 
							step.acc.accArray[1025], step.acc.accArray[1026], step.acc.accArray[1027], step.acc.accArray[1028], step.acc.accArray[1029], 
							step.acc.accArray[1030], step.acc.accArray[1031], step.acc.accArray[1032], step.acc.accArray[1033], step.acc.accArray[1034], 
							step.acc.accArray[1035], step.acc.accArray[1036], step.acc.accArray[1037], step.acc.accArray[1038], step.acc.accArray[1039],			  
			        step.acc.accArray[1040], step.acc.accArray[1041], step.acc.accArray[1042], step.acc.accArray[1043], step.acc.accArray[1044],
							step.acc.accArray[1045], step.acc.accArray[1046], step.acc.accArray[1047], step.acc.accArray[1048], step.acc.accArray[1049],
							step.acc.accArray[1050], step.acc.accArray[1051], step.acc.accArray[1052], step.acc.accArray[1053], step.acc.accArray[1054],
							step.acc.accArray[1055]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc12info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_12_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[1056], step.acc.accArray[1057], step.acc.accArray[1058], step.acc.accArray[1059],  
			        step.acc.accArray[1060], step.acc.accArray[1061], step.acc.accArray[1062], step.acc.accArray[1063], step.acc.accArray[1064], 
			        step.acc.accArray[1065], step.acc.accArray[1066], step.acc.accArray[1067], step.acc.accArray[1068], step.acc.accArray[1069], 
              step.acc.accArray[1070], step.acc.accArray[1071], step.acc.accArray[1072], step.acc.accArray[1073], step.acc.accArray[1074], 
			        step.acc.accArray[1075], step.acc.accArray[1076], step.acc.accArray[1077], step.acc.accArray[1078], step.acc.accArray[1079], 
              step.acc.accArray[1080], step.acc.accArray[1081], step.acc.accArray[1082], step.acc.accArray[1083], step.acc.accArray[1084],
              step.acc.accArray[1085], step.acc.accArray[1086], step.acc.accArray[1087], step.acc.accArray[1088], step.acc.accArray[1089], 
							step.acc.accArray[1090], step.acc.accArray[1091], step.acc.accArray[1092], step.acc.accArray[1093], step.acc.accArray[1094], 
							step.acc.accArray[1095], step.acc.accArray[1096], step.acc.accArray[1097], step.acc.accArray[1098], step.acc.accArray[1099], 
							step.acc.accArray[1100], step.acc.accArray[1101], step.acc.accArray[1102], step.acc.accArray[1103], step.acc.accArray[1104], 
							step.acc.accArray[1105], step.acc.accArray[1106], step.acc.accArray[1107], step.acc.accArray[1108], step.acc.accArray[1109], 
							step.acc.accArray[1110], step.acc.accArray[1111], step.acc.accArray[1112], step.acc.accArray[1113], step.acc.accArray[1114], 
							step.acc.accArray[1115], step.acc.accArray[1116], step.acc.accArray[1117], step.acc.accArray[1118], step.acc.accArray[1119], 
							step.acc.accArray[1120], step.acc.accArray[1121], step.acc.accArray[1122], step.acc.accArray[1123], step.acc.accArray[1124], 
							step.acc.accArray[1125], step.acc.accArray[1126], step.acc.accArray[1127], step.acc.accArray[1128], step.acc.accArray[1129], 
							step.acc.accArray[1130], step.acc.accArray[1131], step.acc.accArray[1132], step.acc.accArray[1133], step.acc.accArray[1134], 
							step.acc.accArray[1135], step.acc.accArray[1136], step.acc.accArray[1137], step.acc.accArray[1138], step.acc.accArray[1139],			  
			        step.acc.accArray[1140], step.acc.accArray[1141], step.acc.accArray[1142], step.acc.accArray[1143], step.acc.accArray[1144],
							step.acc.accArray[1145], step.acc.accArray[1146], step.acc.accArray[1147], step.acc.accArray[1148], step.acc.accArray[1149],
							step.acc.accArray[1150], step.acc.accArray[1151]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc13info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_13_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[1152], step.acc.accArray[1153], step.acc.accArray[1154], 
			        step.acc.accArray[1155], step.acc.accArray[1156], step.acc.accArray[1157], step.acc.accArray[1158], step.acc.accArray[1159],   
			        step.acc.accArray[1160], step.acc.accArray[1161], step.acc.accArray[1162], step.acc.accArray[1163], step.acc.accArray[1164], 
			        step.acc.accArray[1165], step.acc.accArray[1166], step.acc.accArray[1167], step.acc.accArray[1168], step.acc.accArray[1169], 
              step.acc.accArray[1170], step.acc.accArray[1171], step.acc.accArray[1172], step.acc.accArray[1173], step.acc.accArray[1174], 
			        step.acc.accArray[1175], step.acc.accArray[1176], step.acc.accArray[1177], step.acc.accArray[1178], step.acc.accArray[1179], 
              step.acc.accArray[1180], step.acc.accArray[1181], step.acc.accArray[1182], step.acc.accArray[1183], step.acc.accArray[1184],
              step.acc.accArray[1185], step.acc.accArray[1186], step.acc.accArray[1187], step.acc.accArray[1188], step.acc.accArray[1189], 
							step.acc.accArray[1190], step.acc.accArray[1191], step.acc.accArray[1192], step.acc.accArray[1193], step.acc.accArray[1194], 
							step.acc.accArray[1195], step.acc.accArray[1196], step.acc.accArray[1197], step.acc.accArray[1198], step.acc.accArray[1199], 
							step.acc.accArray[1200], step.acc.accArray[1201], step.acc.accArray[1202], step.acc.accArray[1203], step.acc.accArray[1204], 
							step.acc.accArray[1205], step.acc.accArray[1206], step.acc.accArray[1207], step.acc.accArray[1208], step.acc.accArray[1209], 
							step.acc.accArray[1210], step.acc.accArray[1211], step.acc.accArray[1212], step.acc.accArray[1213], step.acc.accArray[1214], 
							step.acc.accArray[1215], step.acc.accArray[1216], step.acc.accArray[1217], step.acc.accArray[1218], step.acc.accArray[1219], 
							step.acc.accArray[1220], step.acc.accArray[1221], step.acc.accArray[1222], step.acc.accArray[1223], step.acc.accArray[1224], 
							step.acc.accArray[1225], step.acc.accArray[1226], step.acc.accArray[1227], step.acc.accArray[1228], step.acc.accArray[1229], 
							step.acc.accArray[1230], step.acc.accArray[1231], step.acc.accArray[1232], step.acc.accArray[1233], step.acc.accArray[1234], 
							step.acc.accArray[1235], step.acc.accArray[1236], step.acc.accArray[1237], step.acc.accArray[1238], step.acc.accArray[1239],			  
			        step.acc.accArray[1240], step.acc.accArray[1241], step.acc.accArray[1242], step.acc.accArray[1243], step.acc.accArray[1244],
							step.acc.accArray[1245], step.acc.accArray[1246], step.acc.accArray[1247]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc14info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_14_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[1248], step.acc.accArray[1249], 
			        step.acc.accArray[1250], step.acc.accArray[1251], step.acc.accArray[1252], step.acc.accArray[1253], step.acc.accArray[1254],
			        step.acc.accArray[1255], step.acc.accArray[1256], step.acc.accArray[1257], step.acc.accArray[1258], step.acc.accArray[1259],   
			        step.acc.accArray[1260], step.acc.accArray[1261], step.acc.accArray[1262], step.acc.accArray[1263], step.acc.accArray[1264], 
			        step.acc.accArray[1265], step.acc.accArray[1266], step.acc.accArray[1267], step.acc.accArray[1268], step.acc.accArray[1269], 
              step.acc.accArray[1270], step.acc.accArray[1271], step.acc.accArray[1272], step.acc.accArray[1273], step.acc.accArray[1274], 
			        step.acc.accArray[1275], step.acc.accArray[1276], step.acc.accArray[1277], step.acc.accArray[1278], step.acc.accArray[1279], 
              step.acc.accArray[1280], step.acc.accArray[1281], step.acc.accArray[1282], step.acc.accArray[1283], step.acc.accArray[1284],
              step.acc.accArray[1285], step.acc.accArray[1286], step.acc.accArray[1287], step.acc.accArray[1288], step.acc.accArray[1289], 
							step.acc.accArray[1290], step.acc.accArray[1291], step.acc.accArray[1292], step.acc.accArray[1293], step.acc.accArray[1294], 
							step.acc.accArray[1295], step.acc.accArray[1296], step.acc.accArray[1297], step.acc.accArray[1298], step.acc.accArray[1299], 
							step.acc.accArray[1300], step.acc.accArray[1301], step.acc.accArray[1302], step.acc.accArray[1303], step.acc.accArray[1304], 
							step.acc.accArray[1305], step.acc.accArray[1306], step.acc.accArray[1307], step.acc.accArray[1308], step.acc.accArray[1309], 
							step.acc.accArray[1310], step.acc.accArray[1311], step.acc.accArray[1312], step.acc.accArray[1313], step.acc.accArray[1314], 
							step.acc.accArray[1315], step.acc.accArray[1316], step.acc.accArray[1317], step.acc.accArray[1318], step.acc.accArray[1319], 
							step.acc.accArray[1320], step.acc.accArray[1321], step.acc.accArray[1322], step.acc.accArray[1323], step.acc.accArray[1324], 
							step.acc.accArray[1325], step.acc.accArray[1326], step.acc.accArray[1327], step.acc.accArray[1328], step.acc.accArray[1329], 
							step.acc.accArray[1330], step.acc.accArray[1331], step.acc.accArray[1332], step.acc.accArray[1333], step.acc.accArray[1334], 
							step.acc.accArray[1335], step.acc.accArray[1336], step.acc.accArray[1337], step.acc.accArray[1338], step.acc.accArray[1339],			  
			        step.acc.accArray[1340], step.acc.accArray[1341], step.acc.accArray[1342], step.acc.accArray[1343]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc15info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_15_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[1344], 
			        step.acc.accArray[1345], step.acc.accArray[1346], step.acc.accArray[1347], step.acc.accArray[1348], step.acc.accArray[1349],
			        step.acc.accArray[1350], step.acc.accArray[1351], step.acc.accArray[1352], step.acc.accArray[1353], step.acc.accArray[1354],
			        step.acc.accArray[1355], step.acc.accArray[1356], step.acc.accArray[1357], step.acc.accArray[1358], step.acc.accArray[1359],   
			        step.acc.accArray[1360], step.acc.accArray[1361], step.acc.accArray[1362], step.acc.accArray[1363], step.acc.accArray[1364], 
			        step.acc.accArray[1365], step.acc.accArray[1366], step.acc.accArray[1367], step.acc.accArray[1368], step.acc.accArray[1369], 
              step.acc.accArray[1370], step.acc.accArray[1371], step.acc.accArray[1372], step.acc.accArray[1373], step.acc.accArray[1374], 
			        step.acc.accArray[1375], step.acc.accArray[1376], step.acc.accArray[1377], step.acc.accArray[1378], step.acc.accArray[1379], 
              step.acc.accArray[1380], step.acc.accArray[1381], step.acc.accArray[1382], step.acc.accArray[1383], step.acc.accArray[1384],
              step.acc.accArray[1385], step.acc.accArray[1386], step.acc.accArray[1387], step.acc.accArray[1388], step.acc.accArray[1389], 
							step.acc.accArray[1390], step.acc.accArray[1391], step.acc.accArray[1392], step.acc.accArray[1393], step.acc.accArray[1394], 
							step.acc.accArray[1395], step.acc.accArray[1396], step.acc.accArray[1397], step.acc.accArray[1398], step.acc.accArray[1399], 
							step.acc.accArray[1400], step.acc.accArray[1401], step.acc.accArray[1402], step.acc.accArray[1403], step.acc.accArray[1404], 
							step.acc.accArray[1405], step.acc.accArray[1406], step.acc.accArray[1407], step.acc.accArray[1408], step.acc.accArray[1409], 
							step.acc.accArray[1410], step.acc.accArray[1411], step.acc.accArray[1412], step.acc.accArray[1413], step.acc.accArray[1414], 
							step.acc.accArray[1415], step.acc.accArray[1416], step.acc.accArray[1417], step.acc.accArray[1418], step.acc.accArray[1419], 
							step.acc.accArray[1420], step.acc.accArray[1421], step.acc.accArray[1422], step.acc.accArray[1423], step.acc.accArray[1424], 
							step.acc.accArray[1425], step.acc.accArray[1426], step.acc.accArray[1427], step.acc.accArray[1428], step.acc.accArray[1429], 
							step.acc.accArray[1430], step.acc.accArray[1431], step.acc.accArray[1432], step.acc.accArray[1433], step.acc.accArray[1434], 
							step.acc.accArray[1435], step.acc.accArray[1436], step.acc.accArray[1437], step.acc.accArray[1438], step.acc.accArray[1439]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
		if (pregant_status.acc16info_trans_success == false)
		{
			sprintf(nbiot.mqtt.mqttMessage, 
							"{\"messageId\": \"%d\",\"params\": {\"data\": [{\"key\":\"accelerometer_16_array\",\"value\":[%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d]}]}}", 
							nbiot.mqtt.mqttMsgID, 
			        step.acc.accArray[1440], step.acc.accArray[1441], step.acc.accArray[1442], step.acc.accArray[1443], step.acc.accArray[1444],
			        step.acc.accArray[1445], step.acc.accArray[1446], step.acc.accArray[1447], step.acc.accArray[1448], step.acc.accArray[1449],
			        step.acc.accArray[1450], step.acc.accArray[1451], step.acc.accArray[1452], step.acc.accArray[1453], step.acc.accArray[1454],
			        step.acc.accArray[1455], step.acc.accArray[1456], step.acc.accArray[1457], step.acc.accArray[1458], step.acc.accArray[1459],   
			        step.acc.accArray[1460], step.acc.accArray[1461], step.acc.accArray[1462], step.acc.accArray[1463], step.acc.accArray[1464], 
			        step.acc.accArray[1465], step.acc.accArray[1466], step.acc.accArray[1467], step.acc.accArray[1468], step.acc.accArray[1469], 
              step.acc.accArray[1470], step.acc.accArray[1471], step.acc.accArray[1472], step.acc.accArray[1473], step.acc.accArray[1474], 
			        step.acc.accArray[1475], step.acc.accArray[1476], step.acc.accArray[1477], step.acc.accArray[1478], step.acc.accArray[1479], 
              step.acc.accArray[1480], step.acc.accArray[1481], step.acc.accArray[1482], step.acc.accArray[1483], step.acc.accArray[1484],
              step.acc.accArray[1485], step.acc.accArray[1486], step.acc.accArray[1487], step.acc.accArray[1488], step.acc.accArray[1489], 
							step.acc.accArray[1490], step.acc.accArray[1491], step.acc.accArray[1492], step.acc.accArray[1493], step.acc.accArray[1494], 
							step.acc.accArray[1495], step.acc.accArray[1496], step.acc.accArray[1497], step.acc.accArray[1498], step.acc.accArray[1499], 
							step.acc.accArray[1500], step.acc.accArray[1501], step.acc.accArray[1502], step.acc.accArray[1503], step.acc.accArray[1504], 
							step.acc.accArray[1505], step.acc.accArray[1506], step.acc.accArray[1507], step.acc.accArray[1508], step.acc.accArray[1509], 
							step.acc.accArray[1510], step.acc.accArray[1511], step.acc.accArray[1512], step.acc.accArray[1513], step.acc.accArray[1514], 
							step.acc.accArray[1515], step.acc.accArray[1516], step.acc.accArray[1517], step.acc.accArray[1518], step.acc.accArray[1519], 
							step.acc.accArray[1520], step.acc.accArray[1521], step.acc.accArray[1522], step.acc.accArray[1523], step.acc.accArray[1524], 
							step.acc.accArray[1525], step.acc.accArray[1526], step.acc.accArray[1527], step.acc.accArray[1528], step.acc.accArray[1529], 
							step.acc.accArray[1530], step.acc.accArray[1531], step.acc.accArray[1532], step.acc.accArray[1533], step.acc.accArray[1534], 
							step.acc.accArray[1535]);
			nbiot.dmp_topictype = PROPERTY_Batch;
			nbiot_printf("Message: %s\n", nbiot.mqtt.mqttMessage);
			return true;
		}
	}
	return false;
}
//###############################################################################################################
bool nbiot_mqttTransmit(void)
{
	if (adc.alarmBattery == true)
	{
		if (nbiot_mqttPublish() == true)
		{
			adc.alarmBattery = false;
			return true;
		}
		if (nbiot.mqtt.mqttConnectedLast == false)
		{
			if(nbiot_mqttDisConnect() == false)
				return false;
			else
				return true;
		}
	}
	
	if (freefall_status.freefall_data_ready == true)
	{
		if (freefall_status.gnss_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				freefall_status.gnss_trans_success = true;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (freefall_status.freefall_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				freefall_status.freefall_trans_success = true;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
  }
	
  if (gnss_status.gnss_data_ready == true)
	{
		if (gnss_status.gnss_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				gnss_status.gnss_trans_success = true;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
	}
			
  if (pregant_status.pregnant_data_ready == true)
	{
		if (pregant_status.pregnant_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.pregnant_trans_success = true;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
				{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
				
		if (pregant_status.acc1info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc1info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY1_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc2info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc2info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY2_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc3info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc3info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY3_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc4info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc4info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY4_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
	
		if (pregant_status.acc5info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc5info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY5_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc6info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc6info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY6_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}

		if (pregant_status.acc7info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc7info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY7_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc8info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc8info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY8_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc9info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc9info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY9_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc10info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc10info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY10_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc11info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc11info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY11_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc12info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc12info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY12_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc13info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc13info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY13_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc14info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc14info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY14_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc15info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc15info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY15_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
		
		if (pregant_status.acc16info_trans_success == false)
		{
			if (nbiot_mqttPublish() == true)
			{
				pregant_status.acc16info_trans_success = true;
				pregant_status.acc_trans = ACC_ARRAY16_TRANS;
				return true;
			}
			if (nbiot.mqtt.mqttConnectedLast == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				else
					return true;
			}
		}
	}
	
	return true;
}
//###############################################################################################################
#endif
