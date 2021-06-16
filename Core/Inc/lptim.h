/**
  ******************************************************************************
  * @file    lptim.h
  * @brief   This file contains all the function prototypes for
  *          the lptim.c file
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
#ifndef __LPTIM_H__
#define __LPTIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
typedef enum
{
  WAKEUP_TIME_5MIN            		= 0x02,
  WAKEUP_TIME_10MIN             	= 0x05,
	WAKEUP_TIME_15MIN             	= 0x07,
  WAKEUP_TIME_30MIN              	= 0x0E,
	WAKEUP_TIME_45MIN              	= 0x15,
  WAKEUP_TIME_1HOUR           		= 0x1C,
  WAKEUP_TIME_2HOUR           		= 0x38,
  WAKEUP_TIME_3HOUR            		= 0x54,
	WAKEUP_TIME_4HOUR            		= 0x70

} WAKEUP_TIMETypeDef;

typedef struct
{
	__IO ITStatus twentyMinuteIndex;
	uint8_t twentyMinuteTimeBase;    //one step == 128s
	__IO ITStatus twoHourIndex;
	uint8_t twoHourTimeBase;    //one step == 128s
	
	__IO ITStatus wakeupIndex;
	WAKEUP_TIMETypeDef wakeupTime;
	uint8_t wakeupTimeBase;			//one step == 128s
	uint8_t wakeupSel;
} lptim_t;

extern lptim_t    lptim;
/* USER CODE END Private defines */

void MX_LPTIM1_Init(void);

/* USER CODE BEGIN Prototypes */
void WakeUp_Time_Init(void);
void Pregnant_Time_Init(void);
void LPTIM1_Counter_Start_IT(void);
void LPTimerAutoreloadMatch_Callback(void);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __LPTIM_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
