#include "gnss.h"
#include "nbiot.h"
#include "gpio.h"
#include "usart.h"
#include "lptim.h"
#include "rtc.h"

gps_usart_rx_dma_index_t gps_usart_rx_dma_index;
gps_valid_data_t gps_valid_data;
gps_t gps;

char strfound[_SET_BUFFERSIZE];
char dmp_eeprom[_SET_BUFFERSIZE];
uint8_t strfoundsize;

//##################################################################################################################
/**
 * \brief           Calculate length of statically allocated array
 */
#define ARRAY_LEN(x)            (sizeof(x) / sizeof((x)[0]))
//##################################################################################################################
/**
  * @brief  This function Turn ON L86 PWRKEY
  * @param  None
  * @retval None
  */
void l86_power_on(void)
{
  /* Turn ON L86 */
  gnss_en_on();
}
//##################################################################################################################
/**
  * @brief  This function Turn OFF L86 PWRKEY
  * @param  None
  * @retval None
  */
void l86_power_off(void)
{
  /* Turn OFF L86 */
  gnss_en_off();
}
//##################################################################################################################
/**
  * @brief  This function Control L86 RESET
  * @param  None
  * @retval >2ms
  */
void l86_reset(void)
{
  /* Reset L86 */
  reset_n_on();
	gps_delay(10);
  reset_n_off();
}
//##################################################################################################################
bool l86_init(void)
{
	l86_power_on();
	
	/* BaudRate 9600 */
	gps_delay(2000);
	gps_usart_send_string("$PMTK001,314,3*36\r\n");
	gps_delay(20);
	gps_usart_send_string("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");
	gps_delay(150);
	gps_usart_send_string("$PMTK251,115200*1F\r\n");
	MX_USART1_UART_DeInit();
	SET_MX_USART1_BaudRate_115200();
	gps_usart_init();
	gps_delay(200);
	/* BaudRate 115200 */
	gps_usart_send_string("$PMTK001,314,3*36\r\n");
	gps_delay(20);
	gps_usart_send_string("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n");
	memset(&gps,0,sizeof(gps));
	while (!gps.rxIndex){gps_delay(1);}
	char *str = strstr((char*)gps.rxBuffer,"$PMTK001,314,3*36");
	if (str != NULL)
	{
		gps_usart_send_string("$PMTK251,115200*1F\r\n");
		gps_delay(1000);
		l86_power_off();
		return true;
	}
	gps_delay(1000);
	l86_power_off();
	return false;
}
//##################################################################################################################
void gps_init(gps_t *gps)
{
	memset(gps,0,sizeof(*gps));
	gps->rxIndex = false;
//	nbiot_printf("[GPS] init done\r\n");
}
//##################################################################################################################
void gps_deinit(gps_t *gps)
{
	memset(gps,0,sizeof(*gps));
	gps->rxIndex = false;
	nbiot_printf("[GPS] deinit done\r\n");
}
//##################################################################################################################
void gps_power(bool on_off)
{
  nbiot_printf("[GPS] power(%d) begin\r\n", on_off);
	if (on_off == true)
  {
    gps.power = true;
		l86_power_on();
    nbiot_printf("[GPS] power(%d) done\r\n", on_off);
  }
	if (on_off == false)
  {
    gps.power = false;
		l86_power_off();
    nbiot_printf("[GPS] power(%d) done\r\n", on_off);
  }
}
//##################################################################################################################
void gps_usart_init(void)
{
	LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)&_GPS_USART->RDR);
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)gps_usart_rx_dma_buffer);
	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, ARRAY_LEN(gps_usart_rx_dma_buffer));

	/* Enable HT & TC interrupts */
	LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_3);
	LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_3);

	LL_USART_EnableDMAReq_RX(_GPS_USART);
	LL_USART_EnableIT_IDLE(_GPS_USART);
	
	/* Enable DMA */
	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
}
//##################################################################################################################
void gps_usart_deinit(void)
{
	/* Disable HT & TC interrupts */
	LL_DMA_DisableIT_HT(DMA1, LL_DMA_CHANNEL_3);
	LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_3);

	LL_USART_DisableDMAReq_RX(_GPS_USART);
	LL_USART_DisableIT_IDLE(_GPS_USART);
	
	/* Disable DMA */
	LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);
}
//##################################################################################################################
/**
 * \brief           Process received data over UART
 * Data are written to RX ringbuffer for application processing at latter stage
 * \param[in]       data: Data to process
 * \param[in]       len: Length in units of bytes
 */
bool gps_usart_process_data(const void* data, size_t len, uint8_t mode) {
	const uint8_t* d = data;
	if ((mode == 1) && (gps_usart_rx_dma_index.DMAHTIndex == true)){
		for (uint8_t i=0; i<len; i++, ++d)
				gps.rxBuffer[i] = *d;
		gps.rxCounter += len;
		return true;
	}
	if ((mode == 2) && (gps_usart_rx_dma_index.DMATCIndex == true)){
		for (uint8_t i=0; i<len; i++, ++d)
				gps.rxBuffer[i] = *d;
		gps.rxCounter += len;
		return true;
	}
	if (gps_usart_rx_dma_index.UARTIdleIndex == true){
		if (mode == 1){
			if (gps.rxCounter == 0){
				for (uint8_t i=0; i<len; i++, ++d)
					gps.rxBuffer[i] = *d;
				gps.rxCounter += len;
			}
			else{
				for (uint8_t i=0; i<len; i++, ++d)
					gps.rxBuffer[gps.rxCounter+i] = *d;
				gps.rxCounter += len;
			}
			return true;
		}
		if (mode == 3){
			for (uint8_t i=0; i<len; i++, ++d)
				gps.rxBuffer[gps.rxCounter+i] = *d;
			gps.rxCounter += len;
			return true;
		}
	}
	return false;
}
//##################################################################################################################
/**
 * \brief           Check for new data received with DMA
 */
void gps_usart_rxCallBack(void) {
    static size_t old_pos;
    size_t pos;

    /* Calculate current position in buffer */
    pos = ARRAY_LEN(gps_usart_rx_dma_buffer) - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3);
    if (pos != old_pos) {                       /* Check change in received data */
        if (pos > old_pos) {                    /* Current position is over previous one */
            /* We are in "linear" mode */
            /* Process data directly by subtracting "pointers" */
            gps_usart_process_data(&gps_usart_rx_dma_buffer[old_pos], pos - old_pos, 1);
        } else {
            /* We are in "overflow" mode */
            /* First process data to the end of buffer */
            gps_usart_process_data(&gps_usart_rx_dma_buffer[old_pos], ARRAY_LEN(gps_usart_rx_dma_buffer) - old_pos, 2);
            /* Check and continue with beginning of buffer */
            if (pos > 0) {
              gps_usart_process_data(&gps_usart_rx_dma_buffer[0], pos, 3);
            }
        }
        old_pos = pos;                          /* Save current position as old */
				memset(&gps_usart_rx_dma_index,0,sizeof(gps_usart_rx_dma_index));
    }
}

//##################################################################################################################
/**
 * \brief           Process received data over UART
 * \note            Either process them directly or copy to other bigger buffer
 * \param[in]       data: Data to process
 * \param[in]       len: Length in units of bytes
 */
void gps_usart_send_data(const void* data, size_t len) {
	const uint8_t* d = data;
	/*
	 * This function is called on DMA TC and HT events, aswell as on UART IDLE (if enabled) line event.
	 * 
	 * For the sake of this example, function does a loop-back data over UART in polling mode.
	 * Check ringbuff RX-based example for implementation with TX & RX DMA transfer.
	 */

	for (; len > 0; --len, ++d) {
			LL_USART_TransmitData8(_GPS_USART, *d);
			while (!LL_USART_IsActiveFlag_TXE(_GPS_USART)) {}
	}
	while (!LL_USART_IsActiveFlag_TC(_GPS_USART)) {}
}
//##################################################################################################################
/**
 * \brief           Send string to USART
 * \param[in]       str: String to send
 */
void gps_usart_send_string(const char* str) {
    gps_usart_send_data((uint8_t*)str, strlen(str));
}
//##################################################################################################################
void gga_parser(void){
	char *str = strstr((char*)gps.rxBuffer,"$GPGGA,");
	if (str != NULL)
	{
		memset(&gps.GGA,0,sizeof(gps.GGA));
		sscanf(str,"$GPGGA,%lf,%lf,%c,%lf,%c,%hhd,%hhd,%f,%lf,%c,%f,%c,%hd,%s,*%2s\r\n",&gps.GGA.utc_time,
					&gps.GGA.latitude,&gps.GGA.latitude_direction,&gps.GGA.longitude,&gps.GGA.longitude_direction,
					&gps.GGA.fix_status,&gps.GGA.sat_in_use,&gps.GGA.HDOP,&gps.GGA.altitude,&gps.GGA.altitude_units,
					&gps.GGA.geoid_separation,&gps.GGA.geoid_units,&gps.GGA.dgps_age,gps.GGA.dgps_stationID,gps.GGA.check_sum);
	
		if (gps.GGA.fix_status == 1 || gps.GGA.fix_status == 2)
		{
			gps_valid_data.longitude = gps.GGA.longitude;
			if (gps.GGA.longitude_direction == 'E')
				gps_valid_data.longitude_direction = 1;
			else if (gps.GGA.longitude_direction == 'W')
				gps_valid_data.longitude_direction = 2;
			else
				gps_valid_data.longitude_direction = 0;
			gps_valid_data.latitude = gps.GGA.latitude;
			if (gps.GGA.latitude_direction == 'N')
				gps_valid_data.latitude_direction = 1;
			else if (gps.GGA.latitude_direction == 'S')
				gps_valid_data.latitude_direction = 2;
			else
				gps_valid_data.latitude_direction = 0;
			gps_valid_data.fix_status = gps.GGA.fix_status;
			gps_valid_data.sat_in_use = gps.GGA.sat_in_use;
			gps_valid_data.altitude = gps.GGA.altitude;
			gps_valid_data.HDOP = gps.GGA.HDOP;

			nbiot_printf("latitude = %f\n",gps_valid_data.latitude);
			nbiot_printf("latitude_direction = %d\n",gps_valid_data.latitude_direction);
			nbiot_printf("longitude = %f\n",gps_valid_data.longitude);
			nbiot_printf("longitude_direction = %d\n",gps_valid_data.longitude_direction);
		}
	}
}
//##################################################################################################################
void rmc_parser(void){
	char *str = strstr((char*)gps.rxBuffer,"$GNRMC,");
	if (str != NULL)
	{
		memset(&gps.RMC,0,sizeof(gps.RMC));
		sscanf(str,"$GNRMC,%lf,%c,%lf,%c,%lf,%c,%lf,%f,%d,%f,%c,%c,%c,*%2s\r\n",&gps.RMC.utc_time,&gps.RMC.data_valid,
					&gps.RMC.latitude,&gps.RMC.latitude_direction,&gps.RMC.longitude,&gps.RMC.longitude_direction,
					&gps.RMC.speed_knots,&gps.RMC.COG,&gps.RMC.ut_date,&gps.RMC.magnetic_variation,&gps.RMC.magnetic_variation_direction,
					&gps.RMC.positioning_mode,&gps.RMC.navigational_status,gps.RMC.check_sum);
	
		if (gps.RMC.data_valid == 'A')
		{
			sscanf(str,"$GNRMC,%2hhd%2hhd%2hhd.%3hd,%c,%lf,%c,%lf,%c,%lf,%f,%2hhd%2hhd%2hhd,%f,%c,%c,%c,*%2s\r\n",&UTC_Calendar.Hours,
					&UTC_Calendar.Minutes,&UTC_Calendar.Seconds,&UTC_Calendar.MicroSec,&gps.RMC.data_valid,
					&gps.RMC.latitude,&gps.RMC.latitude_direction,&gps.RMC.longitude,&gps.RMC.longitude_direction,
					&gps.RMC.speed_knots,&gps.RMC.COG,&UTC_Calendar.Date,&UTC_Calendar.Month,&UTC_Calendar.Year,&gps.RMC.magnetic_variation,&gps.RMC.magnetic_variation_direction,
					&gps.RMC.positioning_mode,&gps.RMC.navigational_status,gps.RMC.check_sum);
			SetRTC(&UTC_Calendar);
			
			gps_valid_data.UTC_time = gps.RMC.utc_time;
			gps_valid_data.UT_date = gps.RMC.ut_date;
			gps_valid_data.longitude = gps.RMC.longitude;
			if (gps.RMC.longitude_direction == 'E')
				gps_valid_data.longitude_direction = 1;
			else if (gps.RMC.longitude_direction == 'W')
				gps_valid_data.longitude_direction = 2;
			else
				gps_valid_data.longitude_direction = 0;
			gps_valid_data.latitude = gps.RMC.latitude;
			if (gps.RMC.latitude_direction == 'N')
				gps_valid_data.latitude_direction = 1;
			else if (gps.RMC.latitude_direction == 'S')
				gps_valid_data.latitude_direction = 2;
			else
				gps_valid_data.latitude_direction = 0;
			gps_valid_data.speed_knots = gps.RMC.speed_knots;
			
			nbiot_printf("latitude = %f\n",gps_valid_data.latitude);
			nbiot_printf("latitude_direction = %d\n",gps_valid_data.latitude_direction);
			nbiot_printf("longitude = %f\n",gps_valid_data.longitude);
			nbiot_printf("longitude_direction = %d\n",gps_valid_data.longitude_direction);
			nbiot_printf("speed_knots = %f\n",gps_valid_data.speed_knots);
		}
	}
	
	str = strstr((char*)gps.rxBuffer,"$GPRMC,");
	if (str != NULL)
	{
		memset(&gps.RMC,0,sizeof(gps.RMC));
		sscanf(str,"$GPRMC,%lf,%c,%lf,%c,%lf,%c,%lf,%f,%d,%f,%c,%c,%c,*%2s\r\n",&gps.RMC.utc_time,&gps.RMC.data_valid,
					&gps.RMC.latitude,&gps.RMC.latitude_direction,&gps.RMC.longitude,&gps.RMC.longitude_direction,
					&gps.RMC.speed_knots,&gps.RMC.COG,&gps.RMC.ut_date,&gps.RMC.magnetic_variation,&gps.RMC.magnetic_variation_direction,
					&gps.RMC.positioning_mode,&gps.RMC.navigational_status,gps.RMC.check_sum);

		if (gps.RMC.data_valid == 'A')
		{
			sscanf(str,"$GNRMC,%2hhd%2hhd%2hhd.%3hd,%c,%lf,%c,%lf,%c,%lf,%f,%2hhd%2hhd%2hhd,%f,%c,%c,%c,*%2s\r\n",&UTC_Calendar.Hours,
					&UTC_Calendar.Minutes,&UTC_Calendar.Seconds,&UTC_Calendar.MicroSec,&gps.RMC.data_valid,
					&gps.RMC.latitude,&gps.RMC.latitude_direction,&gps.RMC.longitude,&gps.RMC.longitude_direction,
					&gps.RMC.speed_knots,&gps.RMC.COG,&UTC_Calendar.Date,&UTC_Calendar.Month,&UTC_Calendar.Year,&gps.RMC.magnetic_variation,&gps.RMC.magnetic_variation_direction,
					&gps.RMC.positioning_mode,&gps.RMC.navigational_status,gps.RMC.check_sum);
			SetRTC(&UTC_Calendar);
			
			gps_valid_data.UTC_time = gps.RMC.utc_time;
			gps_valid_data.UT_date = gps.RMC.ut_date;
			gps_valid_data.longitude = gps.RMC.longitude;
			if (gps.RMC.longitude_direction == 'E')
				gps_valid_data.longitude_direction = 1;
			else if (gps.RMC.longitude_direction == 'W')
				gps_valid_data.longitude_direction = 2;
			else
				gps_valid_data.longitude_direction = 0;
			gps_valid_data.latitude = gps.RMC.latitude;
			if (gps.RMC.latitude_direction == 'N')
				gps_valid_data.latitude_direction = 1;
			else if (gps.RMC.latitude_direction == 'S')
				gps_valid_data.latitude_direction = 2;
			else
				gps_valid_data.latitude_direction = 0;
			gps_valid_data.speed_knots = gps.RMC.speed_knots;
		}
	}
}
//##################################################################################################################
void gps_loop(void)
{
	if (gps.rxIndex == true)
	{
		nbiot_printf("rxBuffer = %s\n", gps.rxBuffer);
		nbiot_printf("rxCounter = %d\n", gps.rxCounter);
		rmc_parser();
		gga_parser();
		memset(gps.rxBuffer,0,sizeof(gps.rxBuffer));
		gps.rxCounter = 0x00;
		gps.rxIndex = false;
	}
}
//##################################################################################################################
bool gps_waitForLocate(uint8_t waitSecond)
{
  nbiot_printf("[GPS] waitForLocate(%d second) begin\r\n", waitSecond);
  uint32_t startTime = HAL_GetTick();
  while (HAL_GetTick() - startTime < (waitSecond * 1000))
  {
		LL_IWDG_ReloadCounter(IWDG);
    gps_delay(100);
    gps_loop();
    if ((gps.RMC.data_valid == 'A') && (gps.GGA.fix_status == 1 || gps.GGA.fix_status == 2))
    {
      nbiot_printf("[GPS] waitForLocate() done\r\n");
      return true;
    }
  }
  nbiot_printf("[GPS] waitForLocate() failed!\r\n");
  return false;
}
//##################################################################################################################
void gps_process(void)
{
	if (gps.power == false)
	{
		memset(&gps_valid_data,0,sizeof(gps_valid_data));
		gps_init(&gps);
		#if (_NBIOT_SET_DEVINFO_USART == 0)
		gps_power(true);
		#endif
	}
	gps_waitForLocate(60);
	gps_deinit(&gps);
	#if (_NBIOT_SET_DEVINFO_USART == 0)
	gps_power(false);
	#endif
}
//##################################################################################################################
bool set_deviceInfo(void)
{
	char *str = strstr((char*)gps.rxBuffer,"5A5A5");
	if (str != NULL)
	{
		memset(strfound, 0, sizeof(strfound));
		strfoundsize = gps.rxCounter-2;
		strncpy(strfound, str, strfoundsize);
//		nbiot_printf("strfoundsize: %d\n", strfoundsize);
//		nbiot_printf("found str: %s\n", strfound);
	}
	else
		return false;

	if (strfound[5] == 'P')
	{
		memset(dmp_eeprom, 0, sizeof(dmp_eeprom));
		if ((strfoundsize - 6) > sizeof(nbiot.dmp_mqtt_triads.productKey))
		{
			nbiot_printf("[NBIOT] strfoundsize exceed productKey length!\r\n");
			return false;
		}
		for (uint8_t i=0; i<(sizeof(nbiot.dmp_mqtt_triads.productKey)/4); i++)
		{
			DATAEEPROM_Program(EEPROM_START_ADDR+4*i, (uint32_t)(0x000000FF & strfound[6+4*i])+(uint32_t)(0x0000FF00 & strfound[7+4*i]<<8)+(uint32_t)(0x00FF0000 & strfound[8+4*i]<<16)+(uint32_t)(0xFF000000 & strfound[9+4*i]<<24));
			dmp_eeprom[4*i] = (uint8_t)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i));
			dmp_eeprom[4*i+1] = (uint8_t)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i)>>8);
			dmp_eeprom[4*i+2] = (uint8_t)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i)>>16);
			dmp_eeprom[4*i+3] = (uint8_t)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+4*i)>>24);
		}
		for (uint8_t i=0; i<sizeof(nbiot.dmp_mqtt_triads.productKey); i++)
			nbiot.dmp_mqtt_triads.productKey[i] = dmp_eeprom[i];
		nbiot_printf("productKey program end\n");
		nbiot_printf("productKey = %s\n",nbiot.dmp_mqtt_triads.productKey);
		return true;
	}
	if (strfound[5] == 'D')
	{
		memset(dmp_eeprom, 0, sizeof(dmp_eeprom));
		if ((strfoundsize - 6) > sizeof(nbiot.dmp_mqtt_triads.deviceKey))
		{
			nbiot_printf("[NBIOT] strfoundsize exceed deviceKey length!\r\n");
			return false;
		}
		for (uint8_t i=0; i<((sizeof(nbiot.dmp_mqtt_triads.deviceKey)/4) - 1); i++)
		{
			DATAEEPROM_Program(EEPROM_START_ADDR+20+4*i, (uint32_t)(0x000000FF & strfound[6+4*i])+(uint32_t)(0x0000FF00 & strfound[7+4*i]<<8)+(uint32_t)(0x00FF0000 & strfound[8+4*i]<<16)+(uint32_t)(0xFF000000 & strfound[9+4*i]<<24));
			dmp_eeprom[4*i] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+20+4*i));
			dmp_eeprom[4*i+1] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+20+4*i)>>8);
			dmp_eeprom[4*i+2] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+20+4*i)>>16);
			dmp_eeprom[4*i+3] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+20+4*i)>>24);
		}
		for (uint8_t i=0; i<sizeof(nbiot.dmp_mqtt_triads.deviceKey); i++)
			nbiot.dmp_mqtt_triads.deviceKey[i] = dmp_eeprom[i];
		nbiot_printf("deviceKey program end\n");
		nbiot_printf("deviceKey = %s\n",nbiot.dmp_mqtt_triads.deviceKey);
		return true;
	}
	if (strfound[5] == 'S')
	{
		memset(dmp_eeprom, 0, sizeof(dmp_eeprom));
		if ((strfoundsize - 6) > sizeof(nbiot.dmp_mqtt_triads.deviceSecret))
		{
			nbiot_printf("[NBIOT] strfoundsize exceed deviceSecret length!\r\n");
			return false;
		}
		for (uint8_t i=0; i<((sizeof(nbiot.dmp_mqtt_triads.deviceSecret)/4) - 1); i++)
		{
			DATAEEPROM_Program(EEPROM_START_ADDR+56+4*i, (uint32_t)(0x000000FF & strfound[6+4*i])+(uint32_t)(0x0000FF00 & strfound[7+4*i]<<8)+(uint32_t)(0x00FF0000 & strfound[8+4*i]<<16)+(uint32_t)(0xFF000000 & strfound[9+4*i]<<24));
			dmp_eeprom[4*i] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+56+4*i));
			dmp_eeprom[4*i+1] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+56+4*i)>>8);
			dmp_eeprom[4*i+2] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+56+4*i)>>16);
			dmp_eeprom[4*i+3] = (char)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+56+4*i)>>24);
		}
		for (uint8_t i=0; i<sizeof(nbiot.dmp_mqtt_triads.deviceSecret); i++)
			nbiot.dmp_mqtt_triads.deviceSecret[i] = dmp_eeprom[i];
		nbiot_printf("deviceSecret program end\n");
		nbiot_printf("deviceSecret = %s\n",nbiot.dmp_mqtt_triads.deviceSecret);
		return true;
	}
	if (strfound[5] == 'W')
	{
		DATAEEPROM_Program(EEPROM_START_ADDR+92, (uint32_t)(0x000000FF & Ascii2Hex(strfound[6])));
		lptim.wakeupSel = (uint8_t)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+92));
		nbiot_printf("wakeupSel program end\n");
		nbiot_printf("wakeupSel = %d\n",lptim.wakeupSel);
		WakeUp_Time_Init();
		memset(&gps,0,sizeof(gps));
		return true;
	}
	
	return false;
}
//##################################################################################################################
