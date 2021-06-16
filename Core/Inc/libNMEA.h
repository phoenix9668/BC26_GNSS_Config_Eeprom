/*!
 * @file libNMEA.h
 * @author	Michał Pałka
 *
 *  Created on: Nov 23, 2020
 *
 * 	This is the header file of the library created to handling communication with GPS devices by NMEA protocol.
 * 	It is created for STM32 MCUs family.
 * 	It should be included in "main.h" file
 */

#include "stm32l0xx_hal.h"
#include <stdlib.h>
#include <string.h>
/**
 * UART DMA buffer size definition
 */
#define NMEA_UART_DMA_BUFFER_SIZE	256
/**
 * Circular buffer size definition
 */
#define NMEA_UART_BUFFER_SIZE 512
/**
 * Working buffer size definition
 */
#define NMEA_WORKING_BUFFER_SIZE 128
/**
 * Pointer to UART handler structure using to communication with GPS module.
 */
extern UART_HandleTypeDef *NMEA_huart;
/**
 * Pointer to DMA handler structure using to receive messages from GPS module.
 */
extern DMA_HandleTypeDef *NMEA_DMA;

extern uint16_t data_length;

/*!
 * Definition of ERROR codes (NMEA_status).
 */
typedef enum {
	NMEA_OK=0,		/**< operation success*/
	NMEA_BUFFER_OVERFLOWED,	/**< circular buffer overflowed*/
	NMEA_CHECKSUM_ERROR,	/**< invalid data received*/
	NMEA_WRONG_DATA,	/**< wrong data passed by user (of the library)*/
	NMEA_WRONG_CB_ID,	/**< invalid CB ID*/
	NMEA_ERROR		/**< something went wrong*/

}NMEA_status;

/*!
 * NMEA_data is a structure caring all useful received data.
 */
typedef struct{
	/*!
	 * <pre>	
	 * time of fix
	 * format: 
	 * hhmmss		for fix rate = 1Hz or less
	 * hhmmss.ss	for fix rate > 1Hz 
	 * </pre>
	 */
	float		UTC_time; 
	/*!	 
	 * <pre>
	 * date of fix
	 * format:	MMDDRR 
	 * </pre>
	 */
	int			UT_date;
	/*!	 
	 * <pre>
	 * latitude of position
	 * format:	DDDMM.MMMM 
	 * </pre>
	 */
	float 		latitude;
	/*!	 
	 * <pre>
	 * latitude direction
	 * N or S
	 * </pre>
	 */
	char 		latitude_direction;
	/*!	 
	 * <pre>
	 * longitude of position
	 * format:	DDDMM.MMMM 
	 * </pre>
	 */
	float 		longitude;
	/*!	 
	 * <pre>
	 * longitude direction
	 * E or W
	 * </pre>
	 */
	char 		longitude_direction;
	/*!	 
	 * <pre>
	 * Antenna altitude above mean-sea-level
	 * units of antenna altitude:	meters
	 * </pre>
	 */
	float 		altitude;
	/*!	 
	 * <pre>
	 * Geoidal separation
	 * units of geoidal separation:	meters
	 * </pre>
	 */
	float		geoidal_separation;
	/*!	 
	 * <pre>
	 * Speed over ground
	 * units of speed:	kilometers/hour
	 * </pre>
	 */
	float		speed_kmph;
	/*!	 
	 * <pre>
	 * Speed over ground
	 * units of speed:	knots
	 * </pre>
	 */
	float		speed_knots;
	/*!	 
	 * <pre>
	 * Number of satellites in view
	 * </pre>
	 */
	uint8_t		sat_in_view;
	/*!	 
	 * <pre>
	 * Number of satellites in use
	 * </pre>
	 */
	uint8_t		sat_in_use;
	/*!	 
	 * <pre>
	 * Fix flag 
	 * 0 - Invalid
	 * 1 - GPS fix
	 * </pre>
	 */
	uint8_t		fix;
	/*!	 
	 * <pre>
	 * Fix mode 
	 * 1 - Fix not available
	 * 2 - 2D mode
	 * 3 - 3D mode
	 * </pre>
	 */
	uint8_t		fix_mode;
	/*!	 
	 * <pre>
	 * Position dilution of precision (3D)
	 * units:	meters
	 * </pre>
	 */
	float		PDOP;
	/*!	 
	 * <pre>
	 * Horizontal dilution of precision
	 * units:	meters
	 * </pre>
	 */
	float		HDOP;
	/*!	 
	 * <pre>
	 * Vertical dilution of precision 
	 * units:	meters
	 * </pre>
	 */
	float		VDOP;

}NMEA_data;
/**
 * NMEA_data instance declaration.
 */
extern NMEA_data nmea_data;

/**
 * NMEA_init is a function to initialize library.\n
 * You should place it before while() loop. 
 * @param[in] 	huart 	is a pointer to UART_HandleTypeDef used to comunicate with GPS module.
 * @param[in]	DMA		is a pointer to DMA_HandleTypeDef used to receive data from module.
 */
void NMEA_init(UART_HandleTypeDef *huart,DMA_HandleTypeDef	*DMA);

/**
 * NMEA_process_task is a function that handles the processing data from module.\n
 * You should place it inside while() loop. 
 * @param[out]	NMEA_status 	is a status code. It should be NMEA_OK. For more information check out NMEA_status documentation.
 */
NMEA_status NMEA_process_task(void);

/**
 * user_IDLE_IT_handler is a function that detects UART idle IT and handles it.\n
 * It should be used in void "UARTX_IRQHandler(void)" from "stm32XXxx_it.c" file like this:
 *@code
void UART4_IRQHandler(void)
{
	//USER CODE BEGIN UART4_IRQn 0
	
	user_UART_IDLE_IT_handler();
	
	//USER CODE END UART4_IRQn 0

  	...
 *@endcode
 */
NMEA_status user_UART_IDLE_IT_handler(void);













