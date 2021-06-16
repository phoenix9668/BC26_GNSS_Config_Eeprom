/**
  ******************************************************************************
  * @file    lptim.c
  * @brief   This file provides code for the configuration
  *          of the LPTIM instances.
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

/* Includes ------------------------------------------------------------------*/
#include "lptim.h"

/* USER CODE BEGIN 0 */
#include "nbiot.h"
lptim_t lptim;
/* USER CODE END 0 */

/* LPTIM1 init function */
void MX_LPTIM1_Init(void)
{

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);

  /* LPTIM1 interrupt Init */
  NVIC_SetPriority(LPTIM1_IRQn, 0);
  NVIC_EnableIRQ(LPTIM1_IRQn);

  LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
  LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV128);
  LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
  LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
  LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
  LL_LPTIM_TrigSw(LPTIM1);

}

/* USER CODE BEGIN 1 */
void WakeUp_Time_Init(void)
{
	lptim.wakeupIndex = RESET;
	lptim.wakeupTimeBase = 0x00;    //one step == 128s
	lptim.wakeupSel = (uint8_t)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+92));
	nbiot_printf("lptim.wakeupSel = %x\n",lptim.wakeupSel);
	
	switch(lptim.wakeupSel)
	{
		case 0x00 : lptim.wakeupTime = WAKEUP_TIME_5MIN;
								break;
		case 0x01 : lptim.wakeupTime = WAKEUP_TIME_10MIN;
								break;
		case 0x02 : lptim.wakeupTime = WAKEUP_TIME_15MIN;
								break;
		case 0x03 : lptim.wakeupTime = WAKEUP_TIME_30MIN;
								break;
		case 0x04 : lptim.wakeupTime = WAKEUP_TIME_45MIN;
								break;
		case 0x05 : lptim.wakeupTime = WAKEUP_TIME_1HOUR;
								break;
		case 0x06 : lptim.wakeupTime = WAKEUP_TIME_2HOUR;
								break;
		case 0x07 : lptim.wakeupTime = WAKEUP_TIME_3HOUR;
								break;
		case 0x08 : lptim.wakeupTime = WAKEUP_TIME_4HOUR;
								break;
		default : lptim.wakeupTime = WAKEUP_TIME_1HOUR;
							break;
	}
	nbiot_printf("lptim.wakeupTime = %x\n",lptim.wakeupTime);
}

void Pregnant_Time_Init(void)
{
	lptim.twentyMinuteIndex = RESET;
	lptim.twentyMinuteTimeBase = 0x00;
	lptim.twoHourIndex = RESET;
	lptim.twoHourTimeBase = 0x00;
}

void LPTIM1_Counter_Start_IT(void)
{
  /* Enable the Autoreload match Interrupt */
  LL_LPTIM_EnableIT_ARRM(LPTIM1);
  /* Enable the LPTIM1 counter */
  LL_LPTIM_Enable(LPTIM1);
  /* Set the Autoreload value */
  LL_LPTIM_SetAutoReload(LPTIM1, 0x7FFF);
  /* Start the LPTIM counter in continuous mode */
  LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);
}

/**
  * @brief  LPTimer Autoreload match interrupt processing
  * @param  None
  * @retval None
  */
void LPTimerAutoreloadMatch_Callback(void)
{
	lptim.twentyMinuteTimeBase ++;
	lptim.twoHourTimeBase ++;
	lptim.wakeupTimeBase ++;
	nbiot_printf("lptim.wakeupTimeBase = %d\n",lptim.wakeupTimeBase);
	if(lptim.twentyMinuteTimeBase == 0x0A)
	{
		lptim.twentyMinuteIndex = SET;
		lptim.twentyMinuteTimeBase = 0x00;
	}
	if(lptim.twoHourTimeBase == 0x38)
	{
		lptim.twoHourIndex = SET;
		lptim.twoHourTimeBase = 0x00;
	}
	if(lptim.wakeupTimeBase == lptim.wakeupTime)
	{
		lptim.wakeupIndex = SET;
		lptim.wakeupTimeBase = 0x00;

	}
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
