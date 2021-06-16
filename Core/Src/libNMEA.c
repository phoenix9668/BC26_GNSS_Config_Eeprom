/**
 * @file libNMEA.c
 * @author	Michał Pałka
 *
 *  Created on: Nov 23, 2020
 *
 *  Implementation of NMEA library.
 */


#include "libNMEA.h"
#include "nbiot.h"
#include "usart.h"

UART_HandleTypeDef *NMEA_huart;
DMA_HandleTypeDef *NMEA_DMA;
NMEA_data nmea_data;
uint16_t data_length;

/**
 * NMEA_UART_DMA_buffer is buffer to which incoming UART data are copied by DMA.\n
 * Size of this buffer has to be bigger then length of all incoming data from module in one shot.
 */
static uint8_t NMEA_UART_DMA_buffer[NMEA_UART_DMA_BUFFER_SIZE]={0};
/**
 * NMEA_UART_buffer is a circular buffer to which data are copied from NMEA_UART_DMA_buffer.\n
 * Size of this buffer should be at least twice NMEA_UART_DMA_BUFFER_SIZE.
 */
static uint8_t NMEA_UART_buffer[NMEA_UART_BUFFER_SIZE]={0};
/**
 * NMEA_working_buffer is buffer to which data from circular buffer are copied to be parse.\n
 * Size of this buffer has to be bigger than longest NMEA message.
 */
static uint8_t NMEA_working_buffer[NMEA_WORKING_BUFFER_SIZE]={0};
static int UART_buffer_head=0;		/**< Index of circular buffer head*/
static int UART_buffer_tail=0;		/**< Index of circular buffer tail*/		
static int UART_buffer_lines=0;	/**< Number of lines in circular buffer to read.*/

/**
 * NMEA_parser is function which parses single correct NMEA message.\n
 * Inside this function all known types of NMEA message are recognized and nmea_data structure fields are set.\n
 * There is also implemented mechanism of recognizing specified events and calling corresponding to them callbacks.
 * @param[in]	message	pointer to buffer storing NMEA message.
 */
static void 	NMEA_parser(char *message){

	int num = 0;
	char *fields[32]={NULL};
	fields[num++]=message;
	while ((message = strchr(message, ','))) {
		*message++ = 0;
		fields[num++]=message;
	}

	if(strcmp(fields[0],"$GPGLL")==0){

		nmea_data.latitude = atof(fields[1]);
		nmea_data.latitude_direction = *fields[2];
		nmea_data.longitude = atof(fields[3]);
		nmea_data.longitude_direction = *fields[4];

	}else if(strcmp(fields[0],"$GNGLL")==0){

		nmea_data.latitude = atof(fields[1]);
		nmea_data.latitude_direction = *fields[2];
		nmea_data.longitude = atof(fields[3]);
		nmea_data.longitude_direction = *fields[4];

	}else if(strcmp(fields[0],"$GPRMC")==0){

		nmea_data.UTC_time = atof(fields[1]);
		nmea_data.UT_date = atoi(fields[9]);

		nmea_data.latitude = atof(fields[3]);
		nmea_data.latitude_direction = *fields[4];
		nmea_data.longitude = atof(fields[5]);
		nmea_data.longitude_direction = *fields[6];

	}else if(strcmp(fields[0],"$GNRMC")==0){

		nmea_data.UTC_time = atof(fields[1]);
		nmea_data.UT_date = atoi(fields[9]);

		nmea_data.latitude = atof(fields[3]);
		nmea_data.latitude_direction = *fields[4];
		nmea_data.longitude = atof(fields[5]);
		nmea_data.longitude_direction = *fields[6];
		
		nbiot_printf("latitude = %f\n",nmea_data.latitude);
		nbiot_printf("latitude_direction = %c\n",nmea_data.latitude_direction);
		nbiot_printf("longitude = %f\n",nmea_data.longitude);
		nbiot_printf("longitude_direction = %c\n",nmea_data.longitude_direction);
	
	}else if(strcmp(fields[0],"$GPVTG")==0){

		nmea_data.speed_knots = atoi(fields[5]);
		nmea_data.speed_kmph = atoi(fields[7]);

	}else if(strcmp(fields[0],"$GPGGA")==0){

		nmea_data.UTC_time = atof(fields[1]);

		nmea_data.latitude = atof(fields[2]);
		nmea_data.latitude_direction = *fields[3];
		nmea_data.longitude = atof(fields[4]);
		nmea_data.longitude_direction = *fields[5];

		nmea_data.fix = atoi(fields[6]);
		nmea_data.sat_in_use = atoi(fields[7]);
		nmea_data.HDOP = atof(fields[8]);

		nmea_data.altitude = atof(fields[9]);
		nmea_data.geoidal_separation = atof(fields[11]);
		
		nbiot_printf("UTC_time = %f\n",nmea_data.UTC_time);
		nbiot_printf("latitude = %f\n",nmea_data.latitude);
		nbiot_printf("latitude_direction = %c\n",nmea_data.latitude_direction);
		nbiot_printf("longitude = %f\n",nmea_data.longitude);
		nbiot_printf("longitude_direction = %c\n",nmea_data.longitude_direction);
		nbiot_printf("fix = %d\n",nmea_data.fix);
		nbiot_printf("sat_in_use = %d\n",nmea_data.sat_in_use);
		nbiot_printf("HDOP = %f\n",nmea_data.HDOP);
		nbiot_printf("altitude = %f\n",nmea_data.altitude);
		nbiot_printf("geoidal_separation = %f\n",nmea_data.geoidal_separation);

	}else if(strcmp(fields[0],"$GPGSA")==0){

		nmea_data.fix_mode = atoi(fields[2]);

		nmea_data.PDOP = atof(fields[15]);
		nmea_data.HDOP = atof(fields[16]);
		nmea_data.VDOP = atof(fields[17]);

	}else if(strcmp(fields[0],"$GPGSV")==0){
		nmea_data.sat_in_view = atoi(fields[3]);
	}
	
}
/**
 * hx2int is function which converts hex number written using characters to corresponding integer.
 * @param[in]	n2		is older position ix hex code
 * @param[in]	n1		is younger position ix hex code
 * @param[out]	uint8_t	is integer corresponding to input hex
 */
static uint8_t hx2int(uint8_t n2, uint8_t n1){
	if (n2 <= '9') n2-='0';
	else n2=n2-'A'+10;

	if (n1 <= '9') n1-='0';
	else n1=n1-'A'+10;

	return n2*16+n1;

}
/**
 * NMEA_checksum_clc is function which calculates checksum of the message and compares it to checksum value given in NMEA message.\n
 * To convert given checksum it uses hx2int function.
 * @param[in]	message	pointer to buffer storing NMEA message.
 * @param[out]	NMEA_status 	is a status code. It should be NMEA_OK. For more information check out NMEA_status documentation.
 */
static NMEA_status NMEA_checksum_clc(uint8_t *message){
	uint8_t index = 1;
	uint8_t checksum_clc =0;

	while (message[index]!='*' && index<NMEA_WORKING_BUFFER_SIZE-2){
		checksum_clc^=message[index++];
	}

	uint8_t checksum = hx2int(message[index+1],message[index+2]);
	if (checksum!=checksum_clc){
		return NMEA_CHECKSUM_ERROR;
	}
	return NMEA_OK;


}

/**
 * NMEA_read_line is function which reads one NMEA message line from NMEA_UART_buffer circular buffer to NMEA_working_buffer.
 */
static void NMEA_read_line(void){
	int index = 0;
	while (index < NMEA_WORKING_BUFFER_SIZE) NMEA_working_buffer[index++]=0;	// Clean up working buffer.

	index = 0;
	while(NMEA_UART_buffer[UART_buffer_tail]!= '\n' && index < NMEA_WORKING_BUFFER_SIZE-2){
		NMEA_working_buffer[index]=NMEA_UART_buffer[UART_buffer_tail];
//		nbiot_printf("NMEA_UART_buffer[%d]=%c\n",UART_buffer_tail,NMEA_UART_buffer[UART_buffer_tail]);
//		nbiot_printf("NMEA_working_buffer[%d]=%c\n",index,NMEA_working_buffer[index]);
		NMEA_UART_buffer[UART_buffer_tail] = 0;
		UART_buffer_tail = (UART_buffer_tail + 1)%NMEA_UART_BUFFER_SIZE;
//		nbiot_printf("UART_buffer_tail=%d\n",UART_buffer_tail);
		++index;
	}
	NMEA_working_buffer[index]=NMEA_UART_buffer[UART_buffer_tail];
	NMEA_UART_buffer[UART_buffer_tail] = 0;
	UART_buffer_tail = (UART_buffer_tail + 1)%NMEA_UART_BUFFER_SIZE;
	++index;
	--UART_buffer_lines;

}

void NMEA_init(UART_HandleTypeDef *huart, DMA_HandleTypeDef *DMA){
	HAL_Delay(10);
	NMEA_huart=huart;
	NMEA_DMA=DMA;
	__HAL_UART_ENABLE_IT(NMEA_huart,UART_IT_IDLE);
	HAL_UART_Receive_DMA(NMEA_huart, NMEA_UART_DMA_buffer, NMEA_UART_DMA_BUFFER_SIZE);
}

/**
 * NMEA_UART_DMA_get_char is a function which takes character from NMEA_UART_DMA_buffer and places it inside NMEA_UART_buffer circular buffer.\n
 * If buffer overflowes, the oldest NMEA message will be deleted to make space for incoming messages.\n
 * If new line character is detected ('\ n'), the line counter (UART_buffer_lines) increases.
 * @param[in]	DMA_char	character from DMA buffer
 * @param[out]	NMEA_status 	is a status code. It should be NMEA_OK. For more information check out NMEA_status documentation.
 */
static NMEA_status NMEA_UART_DMA_get_char(uint8_t DMA_char){
	int position = (UART_buffer_head + 1)%NMEA_UART_BUFFER_SIZE;
//	nbiot_printf("position=%d\n",position);
	NMEA_status stat=NMEA_OK;

	if (position == UART_buffer_tail){		//buffer overflowed! make space for new message
		while (NMEA_UART_buffer[UART_buffer_tail]!='\n' && NMEA_UART_buffer[UART_buffer_tail]!=0){
			NMEA_UART_buffer[UART_buffer_tail]=0;
			UART_buffer_tail=(UART_buffer_tail + 1)%NMEA_UART_BUFFER_SIZE;
		}
		NMEA_UART_buffer[UART_buffer_tail]=0;
		UART_buffer_tail=(UART_buffer_tail + 1)%NMEA_UART_BUFFER_SIZE;
		stat=NMEA_BUFFER_OVERFLOWED;
	}
//	nbiot_printf("DMA_char=%c\n",DMA_char);
	NMEA_UART_buffer[UART_buffer_head]=DMA_char;
//	nbiot_printf("NMEA_UART_buffer[UART_buffer_head]=%c\n",NMEA_UART_buffer[UART_buffer_head]);

	UART_buffer_head=position;
//	nbiot_printf("UART_buffer_head=%d\n",UART_buffer_head);

	if(DMA_char=='\n'){
		++UART_buffer_lines;	//increment lines counter
	}
//	nbiot_printf("UART_buffer_lines=%d\n",UART_buffer_lines);

	return stat;
}

/**
 * NMEA_UART_DMA_copy_buffer is a function which copies messages from DMA buffer to UART circular buffer.\n
 * To do so, it uses NMEA_UART_DMA_get_char function for every character in NMEA_UART_DMA_buffer from 0 to (NMEA_UART_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(NMEA_DMA)).
 * @param[out]	NMEA_status 	is a status code. It should be NMEA_OK. For more information check out NMEA_status documentation.
 */
static NMEA_status NMEA_UART_DMA_copy_buffer(void){

	NMEA_status stat=NMEA_OK;

	data_length = NMEA_UART_DMA_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(NMEA_DMA);
	nbiot_printf("data_length=[%d]\n",data_length);

	for (int i = 0; i < data_length; i++){
		if (NMEA_UART_DMA_get_char(NMEA_UART_DMA_buffer[i])==NMEA_BUFFER_OVERFLOWED){
			stat=NMEA_BUFFER_OVERFLOWED;
		}
		NMEA_UART_DMA_buffer[i]=0;
	}

	HAL_UART_Receive_DMA(NMEA_huart, NMEA_UART_DMA_buffer, NMEA_UART_DMA_BUFFER_SIZE);
	return stat;
}

NMEA_status NMEA_process_task(void){
	NMEA_status stat = NMEA_OK;
	while(UART_buffer_lines>0) {
		NMEA_read_line();
		if (Set_DeviceInfo(NMEA_working_buffer) == true)
			dmp_mqttConParmGenerate();
		for (uint8_t i=0; i<NMEA_WORKING_BUFFER_SIZE; i++)
			nbiot_printf("[%d]=%c ",i, NMEA_working_buffer[i]);
		nbiot_printf("\n");
		if (NMEA_checksum_clc(NMEA_working_buffer) == NMEA_OK){
			NMEA_parser((char *)NMEA_working_buffer);
		}else stat = NMEA_CHECKSUM_ERROR;
	}
	return stat;
}

NMEA_status user_UART_IDLE_IT_handler(void){
	NMEA_status stat = NMEA_OK;
	if (__HAL_UART_GET_FLAG(NMEA_huart, UART_FLAG_IDLE) == SET) {
		__HAL_UART_CLEAR_FLAG(NMEA_huart,UART_FLAG_IDLE);
		HAL_UART_DMAStop(NMEA_huart);
		stat = NMEA_UART_DMA_copy_buffer();
	}
	return stat;
}




