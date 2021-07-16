/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "iwdg.h"
#include "lptim.h"
#include "usart.h"
#include "rtc.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adxl362.h"
#include "nbiot.h"
#include "gnss.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t ErrorIndex;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void SystemPower_Config(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_LPTIM1_Init();
  MX_LPUART1_UART_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  MX_IWDG_Init();
  MX_ADC_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */
	System_Initial();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		LL_IWDG_ReloadCounter(IWDG);
		#if (_NBIOT_SET_DEVINFO_USART == 1)
		gps_init(&gps);
    gps_delay(100);
		set_deviceInfo();
		memset(gps.rxBuffer,0,sizeof(gps.rxBuffer));
		gps.rxCounter = 0x00;
		gps.rxIndex = false;
		#endif
		if (lptim.wakeupIndex == SET)
		{
			if (transProcessNum == 0){
				adc_process();
				gps_process();
			}
			gnss_status.gnss_data_ready = true;
			transProcessNum ++;
			lptim.wakeupIndex = RESET;
		}
		if (freeFallDetection == SET)
		{
			if (transProcessNum == 0){
				adc_process();
				gps_process();
			}
			freefall_status.freefall_data_ready = true;
			transProcessNum ++;
			freeFallDetection = RESET;
		}
		if (lptim.twoHourIndex == SET)
		{
			if (transProcessNum == 0)
				adc_process();
			pregant_status.pregnant_data_ready = true;
			transProcessNum ++;
			lptim.twoHourIndex = RESET;
		}
		LL_IWDG_ReloadCounter(IWDG);
		if (transProcessNum != 0)
			if (nbiot_process() == false)
			{
				nbiot.restart++;
				nbiot_clean();
				nbiot_power(false);
			}
		if (nbiot.restart == 6)
		{
			memset(&gnss_status, 0, sizeof(gnss_status));
			memset(&pregant_status, 0, sizeof(pregant_status));
			memset(&freefall_status, 0, sizeof(freefall_status));
			transProcessNum = 0;
			nbiot_erase();
		}
		if (freefall_status.freefall_trans_success == true)
		{
			memset(&freefall_status, 0, sizeof(freefall_status));
			transProcessNum --;
			if (transProcessNum == 0)
				nbiot_erase();
		}
		if (gnss_status.gnss_trans_success == true)
		{
			memset(&gnss_status, 0, sizeof(gnss_status));
			transProcessNum --;
			if (transProcessNum == 0)
				nbiot_erase();
		}
		if (pregant_status.acc_trans == ((step.acc.accIndex >> 5) + 1))
		{
			memset(&pregant_status, 0, sizeof(pregant_status));
			memset(&step.acc, 0, sizeof(step.acc));
			transProcessNum --;
			if (transProcessNum == 0)
				nbiot_erase();
		}
		if (lptim.twentyMinuteIndex == SET)
		{
			step.stepArray[step.stepStage] = step.stepNum;
/**
	*	for(uint8_t i=0; i < STEP_LOOPNUM; i++)
	*		nbiot_printf("%d ",step.stepArray[i]);
	*	nbiot_printf("\n");
	*	nbiot_printf("step.stepStage = %d\n",step.stepStage);
	*/
			DATAEEPROM_Program((EEPROM_START_ADDR+100+4*step.stepStage), (uint32_t)step.stepArray[step.stepStage]);
			step.stepNum = 0;
			if(step.stepStage == (STEP_LOOPNUM - 1))
				step.stepStage = 0;
			else
				step.stepStage++;
			DATAEEPROM_Program((EEPROM_START_ADDR+244), step.stepStage);
			lptim.twentyMinuteIndex = RESET;
		}
		if (step.stepState == SET)
		{
			ADXL362FifoProcess();
			step.stepState = RESET;
		}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_8;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_LPUART1
                              |RCC_PERIPHCLK_RTC|RCC_PERIPHCLK_LPTIM1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  PeriphClkInit.LptimClockSelection = RCC_LPTIM1CLKSOURCE_LSE;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief  System Power Configuration
  *         The system Power is configured as follow :
  *            + Regulator in LP mode
  *            + VREFINT OFF, with fast wakeup enabled
  *            + HSI as SysClk after Wake Up
  *            + No IWDG
  *            + Automatic Wakeup using RTC clocked by LSI (after ~4s)
  * @param  None
  * @retval None
  */
static void SystemPower_Config(void)
{
	/* Disable Wakeup Counter */
	HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
	/* ## Setting the Wake up time ############################################*/
	/*  RTC Wakeup Interrupt Generation:
			Wakeup Time Base = (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSE or LSI))
			Wakeup Time = Wakeup Time Base * WakeUpCounter 
			= (RTC_WAKEUPCLOCK_RTCCLK_DIV /(LSE or LSI)) * WakeUpCounter
				==> WakeUpCounter = Wakeup Time / Wakeup Time Base
    
			To configure the wake up timer to 10s the WakeUpCounter is set to 0x5000 for LSE:
			RTC_WAKEUPCLOCK_RTCCLK_DIV = RTCCLK_Div16 = 16 
			Wakeup Time Base = 16 /(32.768KHz) = 0.48828125 ms
			Wakeup Time = 10s = 0.48828125ms  * WakeUpCounter
				==> WakeUpCounter = 10s/0.48828125ms = 20480 = 0x5000 */
	HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 0x8000, RTC_WAKEUPCLOCK_RTCCLK_DIV16);
	
  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* Enable Ultra low power mode */
  HAL_PWREx_EnableUltraLowPower();

  /* Enable the fast wake up from Ultra low power mode */
  HAL_PWREx_EnableFastWakeUp();
}

/**
  * @brief System Initial
  * @retval None
*/
void System_Initial(void)
{
	vcc_bc26_power_ctrl(false);
	gps_power(false);
	/*##-1- close ADC peripheral ##*/
	if (HAL_ADC_DeInit(&hadc) != HAL_OK)
  {
    Error_Handler();
  }
	/*##-2- open SPI, initial ADXL362 LPTIM1 peripheral ##*/
	Pregnant_Time_Init();
	WakeUp_Time_Init();
	LPTIM1_Counter_Start_IT();
	Enable_SPI(SPI1);
	ADXL362_Init();
	/*##-3- open LPUART1,enable USART1 and DMA1 */
	gps_usart_init();
	Enable_LPUART1();
	/*##-4- Enable CRC clock */
  __CRC_CLK_ENABLE();
	/*##-5- printf open message,judge ADXL362 stutes ##*/
	Show_Message();
	dmp_mqttTriadsInit();
	
	/*##-6- initial struct nbiot ##*/
	memset(&gnss_status, 0, sizeof(gnss_status));
	memset(&pregant_status, 0, sizeof(pregant_status));
	memset(&freefall_status, 0, sizeof(freefall_status));
	mqttMessageID = 0;
	if (nbiot_init() == false){
			ErrorIndex = 0x01;
		Error_Handler();
	}
	
	/*##-7- configure l86 ##*/
	gps_init(&gps);
	memset(&gps_usart_rx_dma_index,0,sizeof(gps_usart_rx_dma_index));
	memset(&gps_valid_data,0,sizeof(gps_valid_data));
	#if (_NBIOT_SET_DEVINFO_USART == 0)
	if (l86_init() == false){
		ErrorIndex = 0x02;
		Error_Handler();
	}
	#endif

	/*##-8- init step struct ##*/
	for(uint8_t i=0; i < STEP_LOOPNUM; i++){
		step.stepArray[i] = (uint16_t)(0x0000FFFF & DATAEEPROM_Read(EEPROM_START_ADDR+100+4*i));
	}
	step.stepStage = (uint8_t)(0x000000FF & DATAEEPROM_Read(EEPROM_START_ADDR+244));
	
	transProcessNum = 0;
}

/**
  * @brief Show Message
  * @retval None
*/
void Show_Message(void)
{
	unsigned int  ReadValueTemp;
	nbiot_printf("BC26&L86 transfer program \n");
	nbiot_printf("using USART2,configuration:%d 8-N-1 \n",115200);
	nbiot_printf("using 'atc' library\r\n");

//	nbiot_printf("Device_Serial0 : %x\n",(unsigned int)Device_Serial0);
//	nbiot_printf("Device_Serial1 : %x\n",(unsigned int)Device_Serial1);
//	nbiot_printf("Device_Serial2 : %x\n",(unsigned int)Device_Serial2);

	ReadValueTemp = ADXL362RegisterRead(XL362_DEVID_AD);     	//Analog Devices device ID, 0xAD
	if(ReadValueTemp == 0xAD)
	{
		led_on();
		nbiot_delay(500);
	}
	nbiot_printf("Analog Devices device ID: %x\n",ReadValueTemp);	 	//send via UART
	ReadValueTemp = ADXL362RegisterRead(XL362_DEVID_MST);    	//Analog Devices MEMS device ID, 0x1D
	if(ReadValueTemp == 0x1D)
	{
		led_off();
		nbiot_delay(500);
	}
	nbiot_printf("Analog Devices MEMS device ID: %x\n",ReadValueTemp);	//send via UART
	ReadValueTemp = ADXL362RegisterRead(XL362_PARTID);       	//part ID, 0xF2
	if(ReadValueTemp == 0xF2)
	{
		led_on();
		nbiot_delay(500);
	}
	nbiot_printf("Part ID: %x\n",ReadValueTemp);										//send via UART
	ReadValueTemp = ADXL362RegisterRead(XL362_REVID);       	//version ID, 0x02/0x03
	if(ReadValueTemp == 0x02 || ReadValueTemp == 0x03)
	{
		led_off();
	}
	nbiot_printf("Version ID: %x\n",ReadValueTemp);									//send via UART
}

/**
  * @brief DATAEEPROM WRITE
  * @retval None
*/
void DATAEEPROM_Program(uint32_t Address, uint32_t Data)
{
	/* Unlocks the data memory and FLASH_PECR register access *************/
	if(HAL_FLASHEx_DATAEEPROM_Unlock() != HAL_OK)
	{
    Error_Handler();
	}
	/* Clear FLASH error pending bits */
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_SIZERR |
							FLASH_FLAG_OPTVERR | FLASH_FLAG_RDERR |
								FLASH_FLAG_FWWERR | FLASH_FLAG_NOTZEROERR);
	/*Erase a word in data memory *************/
	if (HAL_FLASHEx_DATAEEPROM_Erase(Address) != HAL_OK)
	{
		Error_Handler();
	}
	/*Enable DATA EEPROM fixed Time programming (2*Tprog) *************/
	HAL_FLASHEx_DATAEEPROM_EnableFixedTimeProgram();
	/* Program word at a specified address *************/
	if (HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, Address, Data) != HAL_OK)
	{
		Error_Handler();
	}
	/*Disables DATA EEPROM fixed Time programming (2*Tprog) *************/
	HAL_FLASHEx_DATAEEPROM_DisableFixedTimeProgram();

	/* Locks the Data memory and FLASH_PECR register access. (recommended
     to protect the DATA_EEPROM against possible unwanted operation) *********/
	HAL_FLASHEx_DATAEEPROM_Lock();

}

/**
  * @brief DATAEEPROM READ
  * @retval DATA
*/
uint32_t DATAEEPROM_Read(uint32_t Address)
{
	return *(__IO uint32_t*)Address;
}

/**
  * @brief  SHA256 HMAC compute example.
  * @param  InputMessage: pointer to input message to be hashed.
  * @param  InputMessageLength: input data message length in byte.
  * @param  HMAC_key: pointer to key used in the HMAC computation
  * @param  HMAC_keyLength: HMAC key length in byte.
  * @param  MessageDigest: pointer to output parameter that will handle message digest
  * @param  MessageDigestLength: pointer to output digest length.
  * @retval error status: can be HASH_SUCCESS if success or one of
  *         HASH_ERR_BAD_PARAMETER, HASH_ERR_BAD_CONTEXT,
  *         HASH_ERR_BAD_OPERATION if error occured.
  */
int32_t STM32_SHA256_HMAC_Compute(uint8_t* InputMessage,
                                uint32_t InputMessageLength,
                                uint8_t *HMAC_key,
                                uint32_t HMAC_keyLength,
                                uint8_t *MessageDigest,
                                int32_t* MessageDigestLength)
{
  HMAC_SHA256ctx_stt HMAC_SHA256ctx;
  uint32_t error_status = HASH_SUCCESS;

  /* Set the size of the desired MAC*/
  HMAC_SHA256ctx.mTagSize = CRL_SHA256_SIZE;

  /* Set flag field to default value */
  HMAC_SHA256ctx.mFlags = E_HASH_DEFAULT;

  /* Set the key pointer in the context*/
  HMAC_SHA256ctx.pmKey = HMAC_key;

  /* Set the size of the key */
  HMAC_SHA256ctx.mKeySize = HMAC_keyLength;

  /* Initialize the context */
  error_status = HMAC_SHA256_Init(&HMAC_SHA256ctx);

  /* check for initialization errors */
  if (error_status == HASH_SUCCESS)
  {
    /* Add data to be hashed */
    error_status = HMAC_SHA256_Append(&HMAC_SHA256ctx,
                                    InputMessage,
                                    InputMessageLength);

    if (error_status == HASH_SUCCESS)
    {
      /* retrieve */
      error_status = HMAC_SHA256_Finish(&HMAC_SHA256ctx, MessageDigest, MessageDigestLength);
    }
  }

  return error_status;
}

void double2Bytes(double val,uint8_t* bytes_array){
    // Create union of shared memory space
    union {
        double double_variable;
        uint8_t temp_array[8];
    } u;
    // Overite bytes of union with float variable
    u.double_variable = val;
    // Assign bytes to input array
    memcpy(bytes_array, u.temp_array, 8);
}

void LED_Blinking(uint32_t Period)
{
  /* Toggle LED2 in an infinite loop */
  while (1)
  {
    led_tog();
    nbiot_delay(Period);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	if (ErrorIndex == 0x01 || ErrorIndex == 0x02)
		LED_Blinking(200);
	else
		LED_Blinking(2000);
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
