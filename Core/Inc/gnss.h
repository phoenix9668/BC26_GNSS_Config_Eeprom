#ifndef _GPS_H_
#define _GPS_H_

#include <stdint.h>
#include "main.h"
#include <math.h>

//##################################################################################################################
#define	_GPS_RXSIZE        512    //  GPS-command rx buffer size
#define _SET_BUFFERSIZE    64			// Size of Reception buffer
#define _GPS_USART         USART1
#define gps_delay(x)       HAL_Delay(x)

//##################################################################################################################
/**
 * \brief           Buffer for USART DMA
 * \note            Contains RAW unprocessed data received by UART and transfered by DMA
 */
static uint8_t gps_usart_rx_dma_buffer[_GPS_RXSIZE];

typedef struct 
{
	bool      UARTIdleIndex;
	bool      DMAHTIndex;
	bool      DMATCIndex;
	
}gps_usart_rx_dma_index_t;

extern gps_usart_rx_dma_index_t gps_usart_rx_dma_index;
//##################################################################################################################
//NMEA_data is a structure caring all useful received data.
typedef struct{
	/*!
	 * <pre>	
	 * time of fix
	 * format: 
	 * hhmmss		for fix rate = 1Hz or less
	 * hhmmss.ss	for fix rate > 1Hz 
	 * </pre>
	 */
	double		UTC_time; 
	/*!
	 * <pre>
	 * date of fix
	 * format:	DDMMYY 
	 * </pre>
	 */
	int			UT_date;
	/*!
	 * <pre>
	 * latitude of position
	 * format:	DDDMM.MMMM 
	 * </pre>
	 */
	double 		latitude;
	/*!
	 * <pre>
	 * latitude direction
	 * N or S
	 * </pre>
	 */
	uint8_t 		latitude_direction;
	/*!
	 * <pre>
	 * longitude of position
	 * format:	DDDMM.MMMM 
	 * </pre>
	 */
	double 	 longitude;
	/*!	 
	 * <pre>
	 * longitude direction
	 * E or W
	 * </pre>
	 */
	uint8_t 		longitude_direction;
	/*!
	 * <pre>
	 * Antenna altitude above mean-sea-level
	 * units of antenna altitude:	meters
	 * </pre>
	 */
	double 	 altitude;
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
	double		speed_knots;
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
	uint8_t		fix_status;
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

}gps_valid_data_t;

extern gps_valid_data_t gps_valid_data;

typedef struct
{
	double			utc_time;
	
	double			latitude;
	char		    latitude_direction;
	double			longitude;
	char			  longitude_direction;

	uint8_t			fix_status;
	uint8_t			sat_in_use;
	float			  HDOP;
	double			altitude;
	char				altitude_units;
	float			  geoid_separation;
	char				geoid_units;
	
	uint16_t		dgps_age;
	char				dgps_stationID[4];
	char				check_sum[2];

}GGA_t;

typedef struct
{
	double			utc_time;
	
	char        data_valid;
	double		  latitude;
	char			  latitude_direction;
	double		  longitude;
	char			  longitude_direction;
	
	double			speed_knots;
	float			  COG;
	int					ut_date;
	float       magnetic_variation;
	char				magnetic_variation_direction;
	char				positioning_mode;
	char				navigational_status;

	char				check_sum[2];

}RMC_t;

typedef struct 
{
  uint8_t   rxBuffer[_GPS_RXSIZE];
	uint16_t	rxCounter;
	bool      rxIndex;
	bool      power;
	
	GGA_t GGA;
	RMC_t RMC;
	
}gps_t;

extern gps_t gps;
//##################################################################################################################
/* USART related functions */
void gps_usart_init(void);
void gps_usart_deinit(void);
void gps_usart_rxCallBack(void);
void gps_usart_send_data(const void* data, size_t len);
void gps_usart_send_string(const char* str);

bool l86_init(void);
void gps_init(gps_t *gps);
void gps_deinit(gps_t *gps);
void gps_power(bool on_off);
void gps_process(void);
bool set_deviceInfo(void);
//##################################################################################################################

#endif
