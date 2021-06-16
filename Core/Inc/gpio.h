/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define led_off()							HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET)
#define led_on()							HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET)
#define led_tog()							HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin)

#define psm_eint_off()				HAL_GPIO_WritePin(PSM_EINT_GPIO_Port, PSM_EINT_Pin, GPIO_PIN_RESET)
#define psm_eint_on()					HAL_GPIO_WritePin(PSM_EINT_GPIO_Port, PSM_EINT_Pin, GPIO_PIN_SET)

#define vbet_en_off()					HAL_GPIO_WritePin(VBAT_EN_GPIO_Port, VBAT_EN_Pin, GPIO_PIN_RESET)
#define vbet_en_on()					HAL_GPIO_WritePin(VBAT_EN_GPIO_Port, VBAT_EN_Pin, GPIO_PIN_SET)

#define ldo_en_off()					HAL_GPIO_WritePin(LDO_EN_GPIO_Port, LDO_EN_Pin, GPIO_PIN_RESET)
#define ldo_en_on()						HAL_GPIO_WritePin(LDO_EN_GPIO_Port, LDO_EN_Pin, GPIO_PIN_SET)

#define gnss_en_off()					HAL_GPIO_WritePin(GNSS_EN_GPIO_Port, GNSS_EN_Pin, GPIO_PIN_RESET)
#define gnss_en_on()					HAL_GPIO_WritePin(GNSS_EN_GPIO_Port, GNSS_EN_Pin, GPIO_PIN_SET)

#define reset_n_off()					HAL_GPIO_WritePin(RESET_N_GPIO_Port, RESET_N_Pin, GPIO_PIN_RESET)
#define reset_n_on()					HAL_GPIO_WritePin(RESET_N_GPIO_Port, RESET_N_Pin, GPIO_PIN_SET)

#define force_on_off()				HAL_GPIO_WritePin(FORCE_ON_GPIO_Port, FORCE_ON_Pin, GPIO_PIN_RESET)
#define force_on_on()					HAL_GPIO_WritePin(FORCE_ON_GPIO_Port, FORCE_ON_Pin, GPIO_PIN_SET)

#define adxl362_csn_low()			HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)
#define adxl362_csn_high()		HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)
/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
