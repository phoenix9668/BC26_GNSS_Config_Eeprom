#include "nbiot.h"
#include "gpio.h"
#include "adc.h"

nbiot_t nbiot;
gnss_status_t gnss_status;
pregant_status_t pregant_status;
freefall_status_t freefall_status;
__IO uint16_t mqttMessageID;
__IO uint8_t transProcessNum;

//###############################################################################################################
/**
  * @brief  This function Control VCC_BC26
  * @param  None
  * @retval None
  */
void vcc_bc26_power_ctrl(bool on_off)
{
	LL_IWDG_ReloadCounter(IWDG);
	if (on_off == true)
	{
		ldo_en_off();				/* Turn OFF U5 */
		vbet_en_off();				/* Turn OFF VBAT */
		nbiot_delay(2000);
		if (adc.vbatUp3v6 == true)
			ldo_en_on();				/* Turn ON LDO */
		else
			vbet_en_on();				/* Turn ON VBAT */
		nbiot_delay(2000);
	}
	if (on_off == false)
	{
		ldo_en_off();					/* Turn OFF U5 */
		vbet_en_off();					/* Turn OFF VBAT */
	}
}
//###############################################################################################################
/**
  * @brief  This function Control BC26 PWRKEY
  * @param  None
  * @retval >500ms
  */
void open_bc26(void)
{
	LL_IWDG_ReloadCounter(IWDG);
	HAL_GPIO_WritePin(_NBIOT_KEY_GPIO, _NBIOT_KEY_PIN, GPIO_PIN_SET);
	nbiot_delay(1000);
	HAL_GPIO_WritePin(_NBIOT_KEY_GPIO, _NBIOT_KEY_PIN, GPIO_PIN_RESET);
	nbiot_delay(2000);
}
//###############################################################################################################
/**
  * @brief  This function Control BC26 RESET
  * @param  None
  * @retval >50ms
  */
void reset_bc26(void)
{
	HAL_GPIO_WritePin(_NBIOT_RST_GPIO, _NBIOT_RST_PIN, GPIO_PIN_SET);
	nbiot_delay(100);
  HAL_GPIO_WritePin(_NBIOT_RST_GPIO, _NBIOT_RST_PIN, GPIO_PIN_RESET);
	nbiot_delay(200);
}
//###############################################################################################################
void nbiot_found(char *found_str)
{
  char *str;

  str = strstr(found_str, "\r\n+CEREG: ");
  if (str != NULL)
  {
    int16_t p1 = -1, p2 = -1;
    sscanf(str, "\r\n+CEREG: %hd,%hd", &p1, &p2);
    if (p2 == 1)
    {
      nbiot.status.netReg = 1;
      nbiot.status.netChange = 1;
      return;
    }
    else
    {
      nbiot.status.netReg = 0;
      nbiot.status.netChange = 1;
      return;
    }
  }
  str = strstr(found_str, "\r\n+CGATT: ");
  if (str != NULL)
  {
    int16_t p1 = -1;
    sscanf(str, "\r\n+CGATT: %hd", &p1);
    if (p1 == 1)
    {
      nbiot.status.netReg = 1;
      nbiot.status.netChange = 1;
      return;
    }
    else
    {
      nbiot.status.netReg = 0;
      nbiot.status.netChange = 1;
      return;
    }
  }
  str = strstr(found_str, "\r\n+CSCON: ");
  if (str != NULL)
  {
    int16_t p1 = -1, p2 = -1;
    sscanf(str, "\r\n+CSCON: %hd,%hd", &p1, &p2);
    if (p2 == 1)
    {
      nbiot.status.sigCon = 1;
      nbiot.status.netChange = 1;
      return;
    }
    else
    {
      nbiot.status.sigCon = 0;
      nbiot.status.netChange = 1;
      return;
    }
  }
#if (_NBIOT_MQTT == 1)
	sprintf(str, "\r\n+QMTSTAT: %d\r\n", nbiot.dmp_mqtt_con_parm.TCP_connectID);
  str = strstr(found_str, str);
  if (str != NULL)
  {
    nbiot.mqtt.mqttConnected = false;
    return;
  }
  str = strstr(found_str, "\r\n+QMTRECV: ");
  if (str != NULL)
  {
    memset(nbiot.mqtt.mqttMessage, 0, sizeof(nbiot.mqtt.mqttMessage));
    memset(nbiot.mqtt.mqttTopic, 0, sizeof(nbiot.mqtt.mqttTopic));
    str = strtok(str, "\"");
    do
    {
      str = strtok(NULL, "\"");
      if (str == NULL)
        break;
      char *endStr;
      uint16_t len;
      endStr = strtok(NULL, "\"");
      if (endStr == NULL)
        break;
      len = endStr - str;
      if (len > sizeof(nbiot.mqtt.mqttTopic))
        len = sizeof(nbiot.mqtt.mqttTopic);
      if (len > 2)
        len --;
      strncpy(nbiot.mqtt.mqttTopic, str, len);
      str = strtok(NULL, "\"");
      if (str == NULL)
        break;
      endStr = strtok(NULL, "\"");
      if (endStr == NULL)
        break;
      len = endStr - str;
      if (len > sizeof(nbiot.mqtt.mqttMessage))
        len = sizeof(nbiot.mqtt.mqttMessage);
      if (len > 2)
        len --;
      strncpy(nbiot.mqtt.mqttMessage, str, len);
      nbiot.mqtt.mqttData = 1;      
      
    }while (0);
    return;
  }
#endif
#if (_NBIOT_HTTP == 1)
  str = strstr(found_str, "");
#endif
}
//###############################################################################################################
void nbiot_init_commands(void)
{
  nbiot_command("AT&W0\r\n", 5000, NULL, 0, 1, "\r\nOK\r\n");
  nbiot_command("ATE1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
  nbiot_command("AT+CEREG=3\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
	nbiot_command("AT+QNBIOTEVENT=1,1\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
	nbiot_command("AT+CPSMS=0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
	nbiot_command("AT+CEDRXS=0\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
#if (_NBIOT_MQTT == 1)

#endif
#if (_NBIOT_HTTP == 1)

#endif
#if (_NBIOT_LP == 1)
	nbiot_command("AT+CEREG=5\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n");
	nbiot_command("AT+CPSMS=1,,,\"00100011\",\"00100010\"\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
	nbiot_command("AT+CEDRXS=2,5,\"0101\"\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n");
#endif
	
	
}
//###############################################################################################################
bool nbiot_lock(uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();
  while (HAL_GetTick() - start < timeout_ms)
  {
    if (nbiot.lock == 0)
    {
      nbiot.lock = 1;
      return true;
    }
    nbiot_delay(1);
  }
  return false;
}
//###############################################################################################################
void nbiot_unlock()
{
  nbiot.lock = 0;
}
//###############################################################################################################
bool nbiot_init(void)
{
  if (nbiot.inited == true)
    return true;
  nbiot_printf("[NBIOT] init begin\r\n");
  HAL_GPIO_WritePin(_NBIOT_KEY_GPIO, _NBIOT_KEY_PIN, GPIO_PIN_RESET);
  memset(&nbiot, 0, sizeof(nbiot));
  atc_init(&nbiot.atc, "NBIOT ATC", _NBIOT_USART, nbiot_found);
  if (atc_addSearch(&nbiot.atc, "\r\n+CEREG:") == false)
	{
		nbiot_printf("[NBIOT] init CEREG failed!\r\n");
    return false;
	}
  if (atc_addSearch(&nbiot.atc, "\r\n+CGATT:") == false)
	{
		nbiot_printf("[NBIOT] init CGATT failed!\r\n");
    return false;
	}
  if (atc_addSearch(&nbiot.atc, "\r\n+CSCON:") == false)
	{
		nbiot_printf("[NBIOT] init CSCON failed!\r\n");
    return false;
	}
#if (_NBIOT_MQTT == 1)
  if (atc_addSearch(&nbiot.atc, "\r\n+QMTSTAT:") == false)
	{
		nbiot_printf("[NBIOT] init QMTSTAT failed!\r\n");
    return false;
	}
  if (atc_addSearch(&nbiot.atc, "\r\n+QMTRECV:") == false)
	{
		nbiot_printf("[NBIOT] init QMTRECV failed!\r\n");
    return false;
	}
#endif
#if (_NBIOT_HTTP == 1)

#endif
  nbiot_printf("[NBIOT] init done\r\n");
  nbiot.inited = true;
	nbiot.restart = 0;
  return true;
}
//###############################################################################################################
void nbiot_clean(void)
{
  nbiot_printf("[NBIOT] clean begin\r\n");
  HAL_GPIO_WritePin(_NBIOT_KEY_GPIO, _NBIOT_KEY_PIN, GPIO_PIN_RESET);
  memset(&nbiot.mqtt, 0, sizeof(nbiot.mqtt));
  memset(&nbiot.buffer, 0, sizeof(nbiot.buffer));
	memset(&nbiot.signal, 0, sizeof(nbiot.signal));
	memset(&nbiot.nbiot_dev_info, 0, sizeof(nbiot.nbiot_dev_info));
	memset(&nbiot.error, 0, sizeof(nbiot.error));
	memset(&nbiot.times, 0, sizeof(nbiot.times));
	memset(&nbiot.lock, 0, sizeof(nbiot.lock));
  nbiot_printf("[NBIOT] clean done\r\n");
}
//###############################################################################################################
void nbiot_erase(void)
{
  nbiot.restart = 0;
  mqttMessageID = 0;
  nbiot_clean();
  nbiot_power(false);
}
//###############################################################################################################
void nbiot_loop(void)
{
  static uint32_t nbiot_time_1s = 0;
  static uint32_t nbiot_time_10s = 0;
  atc_loop(&nbiot.atc);
  char str1[64];
  char str2[16];
  //  +++ 1s timer  ######################
  if (HAL_GetTick() - nbiot_time_1s > 1000)
  {
    nbiot_time_1s = HAL_GetTick();
		
		if ((nbiot.status.turnOn == 1) && (nbiot.lock == 0))
    {
      if (nbiot_command("AT\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
        nbiot.error++;
      else
        nbiot.error = 0;
      if (nbiot.error >= 10)
				nbiot_power(false);
    }
    if (nbiot.status.turnOff == 1)
		{
      nbiot.error = 0;
      if (nbiot_command("AT\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
      {
        nbiot_power(false);
      }
    }

#if (_NBIOT_HTTP == 1 || _NBIOT_MQTT == 1)
    //  +++ simcard check
    if ((nbiot.status.power == 1) && (nbiot.status.simcardChecked == 0))
    {
      if (nbiot_command("AT+CPIN?\r\n", 1000, str1, sizeof(str1), 2, "\r\n+CPIN:", "\r\nERROR\r\n") == 1)
      {
        if (sscanf(str1, "\r\n+CPIN: %[^\r\n]", str2) == 1)
        {
          if (strcmp(str2, "READY") == 0)
          {
            nbiot_callback_simcardReady();
            nbiot.status.simcardChecked = 1;
          }
          if (strcmp(str2, "SIM PIN") == 0)
          {
            nbiot_callback_simcardPinRequest();
            nbiot.status.simcardChecked = 1;
          }
          if (strcmp(str2, "SIM PUK") == 0)
          {
            nbiot_callback_simcardPukRequest();
            nbiot.status.simcardChecked = 1;
          }
        }
      }
      else
      {
        nbiot_callback_simcardNotInserted();
      }
    }
    //  --- simcard check

    //  +++ network check
    if ((nbiot.status.power == 1) && (nbiot.status.netChange == 1))
    {
      nbiot.status.netChange = 0;
      if (nbiot.status.netReg == 1)
      {
        nbiot.status.registerd = 1;
        nbiot_callback_networkRegister();
      }
      else
      {
        nbiot.status.registerd = 0;
        nbiot_callback_networkUnregister();
      }
    }
    //  --- network check
#endif
    
    //  +++ network check
#if (_NBIOT_MQTT == 1)
    if (nbiot.status.power == 1)
    {
			if (nbiot.mqtt.mqttData == 1)
			{
				nbiot.mqtt.mqttData = 0;
				nbiot_callback_mqttMessage(nbiot.mqtt.mqttTopic, nbiot.mqtt.mqttMessage);
			}
			if ((nbiot.mqtt.mqttConnected == 1) && (nbiot.mqtt.mqttConnectedLast == 0))
			{
				nbiot.mqtt.mqttConnectedLast = 1;      
			}
			else if ((nbiot.mqtt.mqttConnected == 0) && (nbiot.mqtt.mqttConnectedLast == 1))
			{
				nbiot.mqtt.mqttConnectedLast = 0;
				nbiot_callback_mqttDisconnect();
			}
		}
#endif
    //  --- network check
  }
  //  --- 1s timer  ######################

  //  +++ 10s timer ######################
  if ((HAL_GetTick() - nbiot_time_10s > 10000) && (nbiot.status.power == 1))
  {
    nbiot_time_10s = HAL_GetTick();

#if (_NBIOT_HTTP == 1 || _NBIOT_MQTT == 1)
    //  +++ check network
    if ((nbiot.status.power == 1) && (nbiot.lock == 0))
    {
			nbiot_getVOLTAGE();
      nbiot_getSignalQuality_0_to_100();
      if (nbiot.status.netReg == 0)
      {
        nbiot_command("AT+CEREG?\r\n", 1000, NULL, 0, 0);
      }
      if (nbiot.status.netReg == 0)
      {
        nbiot_command("AT+CGATT?\r\n", 1000, NULL, 0, 0);
      }
    }
    //  --- check network

    //  +++ network check
//#if (_NBIOT_HTTP == 1 || _NBIOT_MQTT == 1)
//    if ((nbiot.status.power == 1) && (nbiot.lock == 0))
//    {
//      if (nbiot.mqtt.connect)
//      {
//        if (nbiot_command("AT+CGPADDR=1\r\n", 1000, str1, sizeof(str1), 1, "\r\n+CGPADDR: 1,") == 1)
//        {
//          if (sscanf(str1, "\r\n+CGPADDR: 1,\"%[^\"\r\n]", nbiot.mqtt.ip) == 1)
//          {
//            if (nbiot.mqtt.connectedLast == false)
//            {
//              nbiot.mqtt.connected = true;
//              nbiot.mqtt.connectedLast = true;
//              nbiot_callback_connected();
//            }
//          }
//          else
//          {
//            if (nbiot.mqtt.connectedLast == true)
//            {
//              nbiot.mqtt.connected = false;
//              nbiot.mqtt.connectedLast = false;
//              nbiot_callback_disconnected();
//            }
//          }
//        }
//        else
//        {
//          if (nbiot.mqtt.connectedLast == true)
//          {
//            nbiot.mqtt.connected = false;
//            nbiot.mqtt.connectedLast = false;
//            nbiot_callback_disconnected();
//          }
//        }
//      }
//    }
//#endif
    //  --- network check

#endif
  }
  //  --- 10s timer ######################

}
//###############################################################################################################
bool nbiot_process(void)
{
	if (nbiot.inited == true && nbiot.status.power == 0)
	{
		if (nbiot_power(true) == false)
			return false;
		if (nbiot_waitForRegister(40) == false)
			return false;
		dmp_mqttConParmGenerate();
	}
	if(nbiot_connect() == false)
		nbiot.times++;
	if((nbiot.mqtt.connected == true) && (nbiot.mqtt.mqttOpened == true))
		nbiot.times = 0;
	if (nbiot.times >= 3)
		return false;
	nbiot_loop();
	if ((nbiot.status.registerd == 1) && (nbiot.mqtt.connected == true))
	{
		if (nbiot.mqtt.mqttConnected == false)
		{
			nbiot_mqttClientOpen(120, 10, 3, 3, 1);
			if (nbiot.mqtt.mqttOpened == false)
			{
				if(nbiot_mqttDisConnect() == false)
					return false;
				nbiot.times++;
				return true;
			}
			nbiot_mqttConnect();
		}
		if (nbiot.mqtt.mqttConnected == true)
		{
			nbiot_mqttMessage();
			nbiot_mqttTopic();
			return nbiot_mqttTransmit();
		}
		nbiot.times++;
	}
	return true;
}
//###############################################################################################################
bool nbiot_power(bool on_off)
{
  nbiot_printf("[NBIOT] power(%d) begin\r\n", on_off);
  uint8_t state = 0;
  if (on_off)
  {
    nbiot.status.turnOn = 1;
    nbiot.status.turnOff = 0;
  }
  else
  {
    nbiot.status.turnOn = 0;
    nbiot.status.turnOff = 1;
  }
  if (nbiot_command("AT\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n") == 1)
    state = 1;
	nbiot_printf("[NBIOT] state(%d)\r\n", state);
  if ((on_off == true) && (state == 1))
  {
    memset(&nbiot.status, 0, sizeof(nbiot.status));
    nbiot.status.power = 1;
    nbiot.status.turnOn = 1;
    nbiot_init_commands();
    nbiot_printf("[NBIOT] power(%d) done\r\n", on_off);
    return true;
  }
  if ((on_off == true) && (state == 0))
  {
    memset(&nbiot.status, 0, sizeof(nbiot.status));
    nbiot.status.turnOn = 1;
		vcc_bc26_power_ctrl(true);
		open_bc26();
    for (uint8_t i = 0; i < 5; i++)
    {
      if (nbiot_command("AT\r\n", 1000, NULL, 0, 1, "\r\nOK\r\n") == 1)
      {
        state = 1;
        break;
      }
    }
    if (state == 1)
    {
      nbiot_delay(5000);
      nbiot_init_commands();
      nbiot.status.power = 1;
      nbiot_printf("[NBIOT] power(%d) done\r\n", on_off);
      return true;
    }
    else
    {
      nbiot_printf("[NBIOT] power(%d) failed!\r\n", on_off);
      return false;
    }
  }
  if (on_off == false)
  {
    nbiot.status.turnOff = 1;
		vcc_bc26_power_ctrl(false);
    nbiot_delay(1000);
    nbiot.status.power = 0;
    nbiot_printf("[NBIOT] power(%d) done\r\n", on_off);
    return true;
  }
  nbiot_printf("[NBIOT] power(%d) failed!\r\n", on_off);
  return false;
}
//###############################################################################################################
bool nbiot_registered(void)
{
  return nbiot.status.registerd;
}
//###############################################################################################################
bool nbiot_getIMEI(char *string, uint8_t sizeOfString)
{
  if ((string == NULL) || (sizeOfString < 15))
  {
    nbiot_printf("[NBIOT] getIMEI() failed!\r\n");
    return false;
  }
  char str[40];
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] getIMEI() lock failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+CGSN=1\r\n", 1000 , str, sizeof(str), 2, "\r\n+CGSN:", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] getIMEI() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  if (sscanf(str, "\r\n+CGSN: %[^\r\n]", string) != 1)
  {
    nbiot_printf("[NBIOT] getIMEI() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] getIMEI() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_getIMSI(char *string, uint8_t sizeOfString)
{
  if ((string == NULL) || (sizeOfString < 15))
  {
    nbiot_printf("[NBIOT] getIMSI() failed!\r\n");
    return false;
  }
  char str[32];
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] getIMSI() lock failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+CIMI\r\n", 1000 , str, sizeof(str), 2, "AT+CIMI", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] getIMSI() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  if (sscanf(str, "AT+CIMI\r\n%[^\r\n]", string) != 1)
  {
    nbiot_printf("[NBIOT] getIMSI() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] getIMSI() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_getICCID(char *string, uint8_t sizeOfString)
{
  if ((string == NULL) || (sizeOfString < 20))
  {
    nbiot_printf("[NBIOT] getICCID() failed!\r\n");
    return false;
  }
  char str[40];
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] getICCID() lock failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+QCCID\r\n", 1000 , str, sizeof(str), 2, "\r\n+QCCID:", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] getICCID() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  if (sscanf(str, "\r\n+QCCID: %[^\r\n]", string) != 1)
  {
    nbiot_printf("[NBIOT] getICCID() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] getICCID() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
uint8_t nbiot_getVOLTAGE(void)
{
  char str[40];
	int16_t p1;
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] getVOLTAGE() lock failed!\r\n");
    return 0;
  }
  if (nbiot_command("AT+CBC\r\n", 1000 , str, sizeof(str), 2, "\r\n+CBC:", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] getVOLTAGE() failed!\r\n");
    nbiot_unlock();
    return 0;
  }
  if (sscanf(str, "\r\n+CBC: 0,0,%hd\r\n", &p1) != 1)
  {
    nbiot_printf("[NBIOT] getVOLTAGE() failed!\r\n");
    nbiot_unlock();
    return 0;
  }
	nbiot.voltage = p1;
  nbiot_printf("[NBIOT] getVOLTAGE() done\r\n");
  nbiot_unlock();
  return nbiot.voltage;
}
//###############################################################################################################
uint8_t nbiot_getSignalQuality_0_to_100(void)
{
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] getSignalQuality_0_to_100() lock failed!\r\n");
    return false;
  }
  char str[32];
  int16_t p1, p2;
  if (nbiot_command("AT+CSQ\r\n", 1000, str, sizeof(str), 2, "\r\n+CSQ:", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] getSignalQuality_0_to_100() failed!\r\n");
    nbiot_unlock();
    return 0;
  }
  if (sscanf(str, "\r\n+CSQ: %hd,%hd\r\n", &p1, &p2) != 2)
  {
    nbiot_printf("[NBIOT] getSignalQuality_0_to_100() failed!\r\n");
    nbiot_unlock();
    return 0;
  }
	nbiot.signal = p1;
//  if (p1; == 99)
//    nbiot.signal = 0;
//  else
//    nbiot.signal = (p1 * 100) / 31;
  nbiot_printf("[NBIOT] getSignalQuality_0_to_100() done\r\n");
  nbiot_unlock();
  return nbiot.signal;
}
//###############################################################################################################
bool nbiot_waitForRegister(uint8_t waitSecond)
{
  nbiot_printf("[NBIOT] waitForRegister(%d second) begin\r\n", waitSecond);
  uint32_t startTime = HAL_GetTick();
  while (HAL_GetTick() - startTime < (waitSecond * 1000))
  {
		LL_IWDG_ReloadCounter(IWDG);
    nbiot_delay(100);
    nbiot_loop();
    if (nbiot.status.registerd == 1)
    {
      for (uint8_t i = 0; i < 10; i++)
      {
        nbiot_loop();
        nbiot_delay(500);
      }
      nbiot_printf("[NBIOT] waitForRegister() done\r\n");
      return true;
    }
//    if (nbiot.inited == false)
//      continue;
  }
  nbiot_printf("[NBIOT] waitForRegister() failed!\r\n");
  return false;
}
//###############################################################################################################
bool nbiot_connect(void)
{
  nbiot_printf("[NBIOT] nbiot_connect() begin\r\n");
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] nbiot_connect() lock failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+CGPADDR=1\r\n", 1000, (char*)nbiot.buffer, sizeof(nbiot.buffer), 1, "\r\n+CGPADDR: 1,") == 1)
  {
		memset(nbiot.mqtt.ip, 0, sizeof(nbiot.mqtt.ip));
		sscanf((char*)nbiot.buffer, "\r\n+CGPADDR: 1,\"%[^\"\r\n]", nbiot.mqtt.ip);
		nbiot.mqtt.connected = true;
		nbiot.mqtt.connectedLast = true;
		nbiot_printf("[NBIOT] nbiot_connect() done\r\n");
		nbiot.mqtt.connect = true;
		nbiot_unlock();
		return true;
  }
	else
	{
		nbiot_printf("[NBIOT] nbiot_connect() failed!\r\n");
    nbiot.mqtt.connect = false;
    nbiot.mqtt.connected = false;
    nbiot_unlock();
		return false;
	}
}
//###############################################################################################################

