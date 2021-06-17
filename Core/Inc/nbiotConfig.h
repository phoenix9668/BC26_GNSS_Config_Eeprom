
#ifndef _NBIOTCONFIG_H_
#define _NBIOTCONFIG_H_

#define _NBIOT_USART                      LPUART1
#define _NBIOT_KEY_GPIO                   PWRKEY_GPIO_Port
#define _NBIOT_KEY_PIN                    PWRKEY_Pin
#define _NBIOT_RST_GPIO                   RESET_GPIO_Port
#define _NBIOT_RST_PIN                    RESET_Pin

#define _NBIOT_DEBUG                      1       //  use printf debug
/*
 *	0 : use LPUART1 LL to printf
 *	1 : use USART1 LL to printf
 *	2 : use USART2 LL to printf
 *	3 : use USART1 HAL to printf
 *	4 : use USART2 HAL to printf
 */
#define _NBIOT_DEBUG_USART              	1
#define _NBIOT_HTTP									      0				// 	enable HTTP
#define _NBIOT_MQTT                       1       //  enable MQTT
#define _NBIOT_LP                       	0       //  enable low power
#define _NBIOT_ALIYUN                     0       //  connect aliyun
#define _NBIOT_DMP                     		1       //  connect cuiot dmp

#endif /*_NBIOTCONFIG_H_ */
