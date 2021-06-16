/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_dma.h"
#include "stm32l0xx_ll_iwdg.h"
#include "stm32l0xx_ll_lptim.h"
#include "stm32l0xx_ll_lpuart.h"
#include "stm32l0xx_ll_rcc.h"
#include "stm32l0xx_hal.h"
#include "stm32l0xx_ll_spi.h"
#include "stm32l0xx_ll_usart.h"
#include "stm32l0xx_ll_system.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_exti.h"
#include "stm32l0xx_ll_bus.h"
#include "stm32l0xx_ll_cortex.h"
#include "stm32l0xx_ll_utils.h"
#include "stm32l0xx_ll_pwr.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include <stdbool.h>
#include "crypto.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define _NBIOT_SET_DEVINFO_USART 0
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define EEPROM_START_ADDR   0x08080000   /* Start @ of user eeprom area */
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define Hex2Ascii(data)  (data < 10)? ('0' + data) : ('A' + data - 10)
#define Ascii2Hex(data)  (data >= '0' && data <= '9')? (data - '0') : ((data >= 'A' && data <= 'F')? (data - 'A' + 10) : ((data >= 'a' && data <= 'f')? (data - 'a' + 10) : 0))
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RESET_Pin LL_GPIO_PIN_0
#define RESET_GPIO_Port GPIOA
#define LED_Pin LL_GPIO_PIN_5
#define LED_GPIO_Port GPIOA
#define PWRKEY_Pin LL_GPIO_PIN_0
#define PWRKEY_GPIO_Port GPIOB
#define RI_Pin LL_GPIO_PIN_1
#define RI_GPIO_Port GPIOB
#define PSM_EINT_Pin LL_GPIO_PIN_12
#define PSM_EINT_GPIO_Port GPIOB
#define VBAT_EN_Pin LL_GPIO_PIN_13
#define VBAT_EN_GPIO_Port GPIOB
#define LDO_EN_Pin LL_GPIO_PIN_14
#define LDO_EN_GPIO_Port GPIOB
#define GNSS_EN_Pin LL_GPIO_PIN_15
#define GNSS_EN_GPIO_Port GPIOB
#define INT1_Pin LL_GPIO_PIN_11
#define INT1_GPIO_Port GPIOA
#define INT1_EXTI_IRQn EXTI4_15_IRQn
#define INT2_Pin LL_GPIO_PIN_12
#define INT2_GPIO_Port GPIOA
#define INT2_EXTI_IRQn EXTI4_15_IRQn
#define CS_Pin LL_GPIO_PIN_15
#define CS_GPIO_Port GPIOA
#define RESET_N_Pin LL_GPIO_PIN_9
#define RESET_N_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
void System_Initial(void);
void Show_Message(void);
void Init_All_UART(void);
void DATAEEPROM_Program(uint32_t Address, uint32_t Data);
uint32_t DATAEEPROM_Read(uint32_t Address);
int32_t STM32_SHA256_HMAC_Compute(uint8_t* InputMessage,
                                uint32_t InputMessageLength,
                                uint8_t *HMAC_key,
                                uint32_t HMAC_keyLength,
                                uint8_t *MessageDigest,
                                int32_t* MessageDigestLength);
void double2Bytes(double val,uint8_t* bytes_array);																
void LED_Blinking(uint32_t Period);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
