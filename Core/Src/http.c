#include "nbiot.h"

#if (_NBIOT_HTTP == 1)
//###############################################################################################################
bool nbiot_httpInit(void)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpInit() failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpInit() failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+HTTPINIT\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_httpInit() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] gprs_httpInit() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_httpSetContent(const char *content)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpSetContent() failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpSetContent() failed!\r\n");
    return false;
  }
  sprintf((char*)nbiot.buffer, "AT+HTTPPARA=\"CONTENT\",\"%s\"\r\n", content);
  if (nbiot_command((char*)nbiot.buffer, 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_httpSetContent() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] gprs_httpSetContent() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_httpSetUserData(const char *data)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpSetUserData() failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpSetUserData() failed!\r\n");
    return false;
  }
  nbiot_transmit((uint8_t* ) "AT+HTTPPARA=\"USERDATA\",\"", strlen("AT+HTTPPARA=\"USERDATA\",\""));
  nbiot_transmit((uint8_t* ) data, strlen(data));
  if (nbiot_command("\"\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_httpSetUserData() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] gprs_httpSetUserData() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_httpSendData(const char *data, uint16_t timeout_ms)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpSendData() failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpSendData() failed!\r\n");
    return false;
  }
  sprintf((char*)nbiot.buffer, "AT+HTTPDATA=%d,%d\r\n", strlen(data), timeout_ms);
  do
  {
    if (nbiot_command((char*)nbiot.buffer, timeout_ms, NULL, 0, 2, "\r\nDOWNLOAD\r\n", "\r\nERROR\r\n") != 1)
      break;
    if (nbiot_command(data, timeout_ms, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;
    nbiot_delay(timeout_ms);
    nbiot_printf("[NBIOT] gprs_httpSendData() done\r\n");
    nbiot_unlock();
    return true;
  } while (0);
  nbiot_printf("[NBIOT] gprs_httpSendData() failed!\r\n");
  nbiot_unlock();
  return false;
}
//###############################################################################################################
int16_t nbiot_httpGet(const char *url, bool ssl, uint16_t timeout_ms)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpGet(%s) failed!\r\n", url);
    return -1;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpGet(%s) failed!\r\n", url);
    return false;
  }
  nbiot.http.code = -1;
  nbiot.http.dataLen = 0;
  do
  {
    if (nbiot_command("AT+HTTPPARA=\"CID\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;
    sprintf((char*)nbiot.buffer, "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
    if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;
    if (nbiot_command("AT+HTTPPARA=\"REDIR\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;
    if (ssl)
    {
      if (nbiot_command("AT+HTTPSSL=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
        break;
    }
    else
    {
      if (nbiot_command("AT+HTTPSSL=0\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
        break;
    }
    if (nbiot_command("AT+HTTPACTION=0\r\n", timeout_ms , (char*)nbiot.buffer, sizeof(nbiot.buffer), 2, "\r\n+HTTPACTION:", "\r\nERROR\r\n") != 1)
      break;
    sscanf((char*)nbiot.buffer, "\r\n+HTTPACTION: 0,%hd,%d\r\n", &nbiot.http.code, &nbiot.http.dataLen);
  } while (0);
  nbiot_printf("[NBIOT] gprs_httpGet(%s) done. answer: %d\r\n", url, nbiot.http.code);
  nbiot_unlock();
  return nbiot.http.code;
}
//###############################################################################################################
int16_t nbiot_httpPost(const char *url, bool ssl, uint16_t timeout_ms)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpPost(%s) failed!\r\n", url);
    return -1;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpPost(%s) failed!\r\n", url);
    return false;
  }
  nbiot.http.code = -1;
  nbiot.http.dataLen = 0;
  do
  {
    if (nbiot_command("AT+HTTPPARA=\"CID\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;
    sprintf((char*)nbiot.buffer, "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
    if (nbiot_command((char*)nbiot.buffer, 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;
    if (nbiot_command("AT+HTTPPARA=\"REDIR\",1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
      break;
    if (ssl)
    {
      if (nbiot_command("AT+HTTPSSL=1\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
        break;
    }
    else
    {
      if (nbiot_command("AT+HTTPSSL=0\r\n", 1000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
        break;
    }
    if (nbiot_command("AT+HTTPACTION=1\r\n", timeout_ms , (char*)nbiot.buffer, sizeof(nbiot.buffer), 2, "\r\n+HTTPACTION:", "\r\nERROR\r\n") != 1)
      break;
    sscanf((char*)nbiot.buffer, "\r\n+HTTPACTION: 1,%hd,%d\r\n", &nbiot.http.code, &nbiot.http.dataLen);
  } while (0);
  nbiot_printf("[NBIOT] gprs_httpPost(%s) done. answer: %d\r\n", url, nbiot.http.code);
  nbiot_unlock();
  return nbiot.http.code;
}
//###############################################################################################################
uint32_t nbiot_httpDataLen(void)
{
  return nbiot.http.dataLen;
}
//###############################################################################################################
uint16_t nbiot_httpRead(uint8_t *data, uint16_t len)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpRead() failed!\r\n");
    return 0;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpRead() failed!\r\n");
    return false;
  }
  memset(nbiot.buffer, 0, sizeof(nbiot.buffer));
  if (len >= sizeof(nbiot.buffer) - 32)
    len = sizeof(nbiot.buffer) - 32;
  char buf[32];
  sprintf(buf, "AT+HTTPREAD=%d,%d\r\n", nbiot.http.dataCurrent, len);
  if (nbiot_command(buf, 1000 , (char*)nbiot.buffer, sizeof(nbiot.buffer), 2, "\r\n+HTTPREAD: ", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_httpRead() failed!\r\n");
    nbiot_unlock();
    return 0;
  }
  if (sscanf((char*)nbiot.buffer, "\r\n+HTTPREAD: %hd\r\n", &len) != 1)
  {
    nbiot_printf("[NBIOT] gprs_httpRead() failed!\r\n");
    nbiot_unlock();
    return 0;
  }
  nbiot.http.dataCurrent += len;
  if (nbiot.http.dataCurrent >= nbiot.http.dataLen)
    nbiot.http.dataCurrent = nbiot.http.dataLen;
  
  uint8_t *s = (uint8_t*)strchr((char*)nbiot.buffer, '\n');  
  if (s == NULL)
  {
    nbiot_printf("[NBIOT] gprs_httpRead() failed!\r\n");
    nbiot_unlock();
    return 0;
  }
  s++;
  for (uint16_t i = 0 ; i < len; i++)
    nbiot.buffer[i] = *s++;
  if (data != NULL)
    memcpy(data, nbiot.buffer, len);
  nbiot_printf("[NBIOT] gprs_httpRead() done. length: %d\r\n", len);
  nbiot_unlock();
  return len;      
}
//###############################################################################################################
bool nbiot_httpTerminate(void)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_httpTerminate() failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_httpTerminate() failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+HTTPTERM\r\n", 1000, NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") == 1)
  {
    nbiot_printf("[NBIOT] gprs_httpTerminate() done\r\n");
    nbiot_unlock();
    return true;
  }
  nbiot_printf("[NBIOT] gprs_httpTerminate() failed!\r\n");
  nbiot_unlock();
  return false;
}
//###############################################################################################################
bool nbiot_ntpServer(char *server, int8_t time_zone_in_quarter)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_ntpServer(%s, %d) failed!\r\n", server, time_zone_in_quarter);
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_ntpServer(%s, %d) failed!\r\n", server, time_zone_in_quarter);
    return false;
  }
  sprintf((char*)nbiot.buffer, "AT+CNTP=\"%s\",%d\r\n", server, time_zone_in_quarter);
  if (nbiot_command((char*)nbiot.buffer, 10000 , NULL, 0, 2, "\r\nOK\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_ntpServer(%s, %d) failed!\r\n", server, time_zone_in_quarter);
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] gprs_ntpServer(%s, %d) done\r\n", server, time_zone_in_quarter);
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_ntpSyncTime(void)
{
  if (nbiot.http.connected == false)
  {
    nbiot_printf("[NBIOT] gprs_ntpSyncTime() failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_ntpSyncTime() failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+CNTP\r\n", 10000, NULL, 0, 2, "\r\n+CNTP: 1\r\n", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_ntpSyncTime() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  nbiot_printf("[NBIOT] gprs_ntpSyncTime() done\r\n");
  nbiot_unlock();
  return true;
}
//###############################################################################################################
bool nbiot_ntpGetTime(char *string)
{
  if (string == NULL)
  {
    nbiot_printf("[NBIOT] gprs_ntpGetTime() failed!\r\n");
    return false;
  }
  if (nbiot_lock(10000) == false)
  {
    nbiot_printf("[NBIOT] gprs_ntpGetTime() failed!\r\n");
    return false;
  }
  if (nbiot_command("AT+CCLK?\r\n", 10000, (char*)nbiot.buffer, sizeof(nbiot.buffer), 2, "\r\n+CCLK:", "\r\nERROR\r\n") != 1)
  {
    nbiot_printf("[NBIOT] gprs_ntpGetTime() failed!\r\n");
    nbiot_unlock();
    return false;
  }
  sscanf((char*)nbiot.buffer, "\r\n+CCLK: \"%[^\"\r\n]", string);
  nbiot_printf("[NBIOT] gprs_ntpGetTime() done. %s\r\n", string);
  nbiot_unlock();
  return true;
}
#endif
