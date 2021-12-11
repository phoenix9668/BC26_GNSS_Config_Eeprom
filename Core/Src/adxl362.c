/********************************************************************************
 Author : CAC (China Applications Support Team) 

 Date :   May, 2014

 File name :  ADXL362.c 

 Description :	 ADXL362 SPI communication driver                

 Hardware plateform : 	EVAL-ADuCM360MKZ and EVAL-ADXL362Z 
 
 Connection:
                 EVAL-ADuCM360MKZ       EVAL-ADXL362Z 
                               
                 P1.4:MISO,             MISO             
                 P1.5:SCLK,             SCLK                                                             
                 P1.6:MOSI,             MOSI
                 P1.7:GPIO as CS,       CS  
********************************************************************************/

#include "adxl362.h"
#include "spi.h"
#include "gpio.h"
#include "nbiot.h"
#include "rtc.h"

uint8_t fifo[1024];
step_t step;
__IO FlagStatus freeFallDetection;
extern uint8_t ErrorIndex;

/*******************************************************************
  @brief unsigned char ADXL362RegisterRead(unsigned char Address)
         Read a register value from ADXL362         
  @param
         unsigned char Address:       Register address
  @return   
         unsigned int  ReceiveValue:  read register value from ADXL362
*******************************************************************/
unsigned char ADXL362RegisterRead(unsigned char Address)
{   
    unsigned char SendTemp[2];
    unsigned char ReceiveTemp[2];
    unsigned char ReceiveValue;

    adxl362_csn_low();              		//CS down
    SendTemp[0] = 0x0B;                 //0x0B: read register command
    SendTemp[1] = Address;              //address byte
    SpiFunction(SPI1, SendTemp, ReceiveTemp, 2, 1);
    ReceiveValue = ReceiveTemp[0];
    adxl362_csn_high();
    return(ReceiveValue);               //CS up
}

/*******************************************************************
  @brief void ADXL362RegisterWrite(unsigned char Address, unsigned char SendValue)
         send SPI command to ADXL362
  @param
         unsigned char Address:       Register address
         unsigned char SendValue:     Value written to ADXL362 register
  @return   
         none
*******************************************************************/
void ADXL362RegisterWrite(unsigned char Address, unsigned char SendValue)
{
    unsigned char SendTemp[3];
    unsigned char ReceiveTemp[3];

    adxl362_csn_low();              			//CS down
    SendTemp[0] = 0x0A;                 //0x0A: write register
    SendTemp[1] = Address;              //address byte
    SendTemp[2] = SendValue;

    SpiFunction(SPI1, SendTemp, ReceiveTemp, 3, 0);
    adxl362_csn_high();              		//CS up
}

/*******************************************************************
  @brief void ADXL362BurstRead(unsigned char Address, unsigned char NumberofRegisters, unsigned char *RegisterData)
         Multibyte read from ADXL362
  @param
         unsigned char Address:           Register address
         unsigned char NumberofRegisters: Register numbers to be read
         unsigned char *RegisterData:     Buffer save the read value
  @return   
         none  
*******************************************************************/
void ADXL362BurstRead(unsigned char Address, unsigned char NumberofRegisters, unsigned char *RegisterData)
{
    unsigned char SendTemp[2];

    adxl362_csn_low();         			//CS down
    SendTemp[0] = 0x0B;            	//0x0B: read register  
    SendTemp[1] = Address;         	//address byte
    SpiFunction(SPI1, SendTemp, RegisterData, 2, NumberofRegisters);
    adxl362_csn_high();        			//CS up
}

/*******************************************************************
  @brief void ADXL362BurstWrite(unsigned char Address, unsigned char NumberofRegisters, unsigned char *RegisterData)
         Multibyte write to ADXL362
  @param
         unsigned char Address:           Register address
         unsigned char NumberofRegisters: Register numbers to be written
         unsigned char *RegisterData:     Buffer save the written value
  @return   
         none 
*******************************************************************/
void ADXL362BurstWrite(unsigned char Address, unsigned char NumberofRegisters, unsigned char *RegisterData)
{
    unsigned char SendTemp[256];
    unsigned char ReceiveTemp[2];
    unsigned char RegisterIndex;

    adxl362_csn_low();										//CS down
    SendTemp[0] = 0x0A;                 //0x0A: write register
    SendTemp[1] = Address;              //address byte
    for (RegisterIndex=0; RegisterIndex<NumberofRegisters; RegisterIndex++)
    {
        SendTemp[2+RegisterIndex] = *(RegisterData + RegisterIndex);
    }
    SpiFunction(SPI1, SendTemp, ReceiveTemp, (2+NumberofRegisters), 0);
    adxl362_csn_high();              		//CS up
}

/*******************************************************************
  @brief void ADXL362FifoRead(unsigned char NumberofRegisters, unsigned char *RegisterData)
         Multibyte read from ADXL362 FIFO
  @param
         unsigned char NumberofRegisters: Register numbers to be read
         unsigned char *RegisterData:     Buffer save the read value
  @return
         none
*******************************************************************/
void ADXL362FifoRead(unsigned int NumberofRegisters, unsigned char *RegisterData)
{
		unsigned char SendTemp[1];
    adxl362_csn_low();         			//CS down
		SendTemp[0] = 0x0D;            	//0x0D: read register
    SpiFunction(SPI1, SendTemp, RegisterData, 1, NumberofRegisters);
    adxl362_csn_high();        			//CS up
}

/*******************************************************************
  @brief void ADXL362FifoEntries(void)
         read ADXL362 FIFO ENTRIES REGISTERS 
  @param
         none
  @return
         none
*******************************************************************/
uint16_t ADXL362FifoEntries(void)
{
		uint16_t ReadValueTemp;
		ReadValueTemp = (0x00ff & ADXL362RegisterRead(XL362_FIFO_ENTRIES_L)) + (0x0300 & ADXL362RegisterRead(XL362_FIFO_ENTRIES_H));
		return(ReadValueTemp);
}

/*******************************************************************
  @brief void ADXL362FifoProcess(void)
         Multibyte read from ADXL362 FIFO
  @param
				 none
  @return
				 none
*******************************************************************/
void ADXL362FifoProcess(void)
{
		uint8_t maxIndex = 0;
    float max = 0;
		
		memset(sq, 0, sizeof(sq));
		memset(xAxis, 0, sizeof(xAxis));
		memset(yAxis, 0, sizeof(yAxis));
		memset(zAxis, 0, sizeof(zAxis));
	
	  // If the fifo data is not aligned. Organize the data
		if (((fifo[3]>>6 & 0x03) == 0x0) && ((fifo[5]>>6 & 0x03) == 0x1) && ((fifo[7]>>6 & 0x03) == 0x2))
		{
			for(uint16_t i=0; i < 1024; i++)
			{
				if (i >= 1016)
					fifo[i] = 0;
				else
					fifo[i] = fifo[i+2];
			}
		}

		if (((fifo[5]>>6 & 0x03) == 0x0) && ((fifo[7]>>6 & 0x03) == 0x1) && ((fifo[9]>>6 & 0x03) == 0x2))
		{
			for(uint16_t i=0; i < 1024; i++)
			{
				if (i >= 1016)
					fifo[i] = 0;
				else
				  fifo[i] = fifo[i+4];
			}
		}

		if (((fifo[7]>>6 & 0x03) == 0x0) && ((fifo[9]>>6 & 0x03) == 0x1) && ((fifo[11]>>6 & 0x03) == 0x2))
		{
			for(uint16_t i=0; i < 1024; i++)
			{
				if (i >= 1016)
					fifo[i] = 0;
				else
				  fifo[i] = fifo[i+6];
			}
		}

		for(uint16_t i=0; i < 512; i++)
		{
//			nbiot_printf("F[%d] = %x ", 2*i, fifo[2*i]);
//			nbiot_printf("F[%d] = %x ", 2*i+1, fifo[2*i+1]);
      // 1.To 16-bit complement
			if ((fifo[2*i+1]>>6 & 0x03) == 0x0)
			{
				if ((fifo[2*i+1] & 0x08))
					xAxis[i/4] = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)) + 0xf000);
				else
					xAxis[i/4] = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)));
//				nbiot_printf("X[%d] = %hd, %hx ", i/4, xAxis[i/4], xAxis[i/4]);
			}
			else if ((fifo[2*i+1]>>6 & 0x03) == 0x1)
			{
				if ((fifo[2*i+1] & 0x08))
					yAxis[i/4] = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)) + 0xf000);
				else
					yAxis[i/4] = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)));
//				nbiot_printf("Y[%d] = %hd, %hx ", i/4, yAxis[i/4], yAxis[i/4]);
			}
			else if ((fifo[2*i+1]>>6 & 0x03) == 0x2)
			{
				if ((fifo[2*i+1] & 0x08))
					zAxis[i/4] = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)) + 0xf000);
				else
					zAxis[i/4] = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)));
//				nbiot_printf("Z[%d] = %hd, %hx\n", i/4, zAxis[i/4], zAxis[i/4]);
			}
			else if ((fifo[2*i+1]>>6 & 0x03) == 0x3)
			{
				if ((fifo[2*i+1] & 0x08))
					temp = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)) + 0xf000);
				else
					temp = (short int)(fifo[2*i] + (0x0f00 & (fifo[2*i+1]<<8)));
//				nbiot_printf("temp[%d] = %hd, %hx\n", i/4, temp, temp);
			}
			
			// 2.Calculate the sample root mean square
      if ((fifo[2*i+1]>>6 & 0x03) == 0x3)
      {
			  sq[i/4] = sqrt(pow(xAxis[i/4] - yAxis[i/4], 2) + pow(xAxis[i/4] - zAxis[i/4], 2) + pow(yAxis[i/4] - zAxis[i/4], 2));
      }
		}

		// 3.Find the subscript of the maximum root mean square
    for (uint8_t i=0; i<(sizeof(fifo)/8); i++)
    {
      if (sq[i] >= max)
      {
        max = sq[i];
        maxIndex = i;
      }
    }

    // 4.Find the sample with the largest root mean square
//		if (step.acc.accIndex <= 307)
//		{
//			for (uint8_t i=0; i<3; i++)
//			{
//				step.acc.accArray[step.acc.accIndex*5 + i] = (uint16_t)(0x00ff & fifo[maxIndex*8 + 2*i]) + (uint16_t)(0xff00 & fifo[maxIndex*8 + 2*i+1]<<8);
//				nbiot_printf("accelerometerArray[%d] = %d, %x\n", step.acc.accIndex*5 + i, step.acc.accArray[step.acc.accIndex*5 + i], step.acc.accArray[step.acc.accIndex*5 + i]);
//			}
//			GetRTC(&UTC_Time, &UTC_Date);
//			step.acc.accArray[step.acc.accIndex*5+3] = (uint16_t)UTC_Time.Hours;
//			nbiot_printf("accelerometerArray[%d] = %d, %x\n", step.acc.accIndex*5+3, step.acc.accArray[step.acc.accIndex*5+3], step.acc.accArray[step.acc.accIndex*5+3]);
//			step.acc.accArray[step.acc.accIndex*5+4] = (uint16_t)(0x00ff & UTC_Time.Seconds) + (uint16_t)(0xff00 & UTC_Time.Minutes<<8);
//			nbiot_printf("accelerometerArray[%d] = %d, %x\n", step.acc.accIndex*5+4, step.acc.accArray[step.acc.accIndex*5+4], step.acc.accArray[step.acc.accIndex*5+4]);
//	  }

		if (xAxis[maxIndex] <= -400 || xAxis[maxIndex] >= 400 || yAxis[maxIndex] <= -400 || yAxis[maxIndex] >= 400)
		{
		  step.stepNum++;
//			nbiot_printf("stepNum = %d\n", step.stepNum);
		}

		if ((step.acc.accIndex <= 511) && (fifo[maxIndex*8+5] != 0))
		{
			step.acc.accArray[step.acc.accIndex*3] = (uint16_t)(0x00ff & fifo[maxIndex*8+2]) + (uint16_t)(0xff00 & fifo[maxIndex*8+3]<<8);
//			nbiot_printf("accelerometerArray[%d] = %d, %x\n", step.acc.accIndex*3, step.acc.accArray[step.acc.accIndex*3], step.acc.accArray[step.acc.accIndex*3]);
			step.acc.accArray[step.acc.accIndex*3+1] = (uint16_t)(0x00ff & fifo[maxIndex*8+4]) + (uint16_t)(0xff00 & fifo[maxIndex*8+5]<<8);
//			nbiot_printf("accelerometerArray[%d] = %d, %x\n", step.acc.accIndex*3+1, step.acc.accArray[step.acc.accIndex*3+1], step.acc.accArray[step.acc.accIndex*3+1]);
			GetRTC(&UTC_Time, &UTC_Date);
			step.acc.accArray[step.acc.accIndex*3+2] = (uint16_t)(0x00ff & UTC_Time.Minutes) + (uint16_t)(0xff00 & UTC_Time.Hours<<8);
//			nbiot_printf("accelerometerArray[%d] = %d, %x\n", step.acc.accIndex*3+2, step.acc.accArray[step.acc.accIndex*3+2], step.acc.accArray[step.acc.accIndex*3+2]);
		}
		
    step.acc.accIndex++;
		ADXL362RegisterRead(XL362_STATUS);
//		nbiot_printf("XL362_STATUS: %x\n",ADXL362RegisterRead(XL362_STATUS));
}

/*******************************************************************
  @brief void ADXL362_Init(void)
         initial and configure ADXL362
  @param
				 none
  @return
         none
*******************************************************************/
void ADXL362_Init(void)
{
		unsigned int ReadValueTemp;
		memset(&step, 0, sizeof(step));
		ADXL362RegisterWrite(XL362_SOFT_RESET,0x52);   						// software reset
		nbiot_delay(1000);

		ADXL362RegisterWrite(XL362_THRESH_ACT_L,0x2C);						//set active threshold equip 300mg
		ReadValueTemp = ADXL362RegisterRead(XL362_THRESH_ACT_L);
//		nbiot_printf("THRESH_ACT_L: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x2C){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_THRESH_ACT_H,0x01);
		ReadValueTemp = ADXL362RegisterRead(XL362_THRESH_ACT_H);
//		nbiot_printf("THRESH_ACT_H: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x01){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_TIME_ACT,0x2D);						//set active time equip 45/200s
		ReadValueTemp = ADXL362RegisterRead(XL362_TIME_ACT);
//		nbiot_printf("TIME_ACT: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x2D){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_THRESH_INACT_L,0x2C);					//set inactive threshold equip 300mg
		ReadValueTemp = ADXL362RegisterRead(XL362_THRESH_INACT_L);
//		nbiot_printf("THRESH_INACT_L: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x2C){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_THRESH_INACT_H,0x01);
		ReadValueTemp = ADXL362RegisterRead(XL362_THRESH_INACT_H);
//		nbiot_printf("THRESH_INACT_H: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x01){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_TIME_INACT_L,0x18);						//set inactive time equip 24/200s
		ReadValueTemp = ADXL362RegisterRead(XL362_TIME_INACT_L);
//		nbiot_printf("TIME_INACT_L: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x18){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_TIME_INACT_H,0x00);
		ReadValueTemp = ADXL362RegisterRead(XL362_TIME_INACT_H);
//		nbiot_printf("TIME_INACT_H: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x00){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_ACT_INACT_CTL,0x07);						//configure default mode,enable active and inactive
		ReadValueTemp = ADXL362RegisterRead(XL362_ACT_INACT_CTL);
//		nbiot_printf("ACT_INACT_CTL: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x07){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_INTMAP1,0x10);									//configure act map INT1
		ReadValueTemp = ADXL362RegisterRead(XL362_INTMAP1);
//		nbiot_printf("INTMAP1: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x10){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_INTMAP2,0x20);									//configure inact map INT2
		ReadValueTemp = ADXL362RegisterRead(XL362_INTMAP2);
//		nbiot_printf("INTMAP2: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x20){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_FIFO_CONTROL,0x0E);						//select fifo Stream mode,Store Temperature Data to FIFO
		ReadValueTemp = ADXL362RegisterRead(XL362_FIFO_CONTROL);
//		nbiot_printf("FIFO_CONTROL: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x0E){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_FIFO_SAMPLES,0xFF);						//select fifo sample number//0xFF
		ReadValueTemp = ADXL362RegisterRead(XL362_FIFO_SAMPLES);
//		nbiot_printf("FIFO_SAMPLES: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0xFF){
			ErrorIndex = 0x03;
			Error_Handler();
		}

		ADXL362RegisterWrite(XL362_FILTER_CTL,0x14);             	//select 2g range,ODR:200Hz
		ReadValueTemp = ADXL362RegisterRead(XL362_FILTER_CTL);
//		nbiot_printf("FILTER_CTL: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x14){
			ErrorIndex = 0x03;
			Error_Handler();
		}
    //any changes to the registers before the POWER_CTL register (Register 0x00 to Register 0x2C) should be made with the device in standby
    ADXL362RegisterWrite(XL362_POWER_CTL,0x02);              	//select measurement mode
		ReadValueTemp = ADXL362RegisterRead(XL362_POWER_CTL);
//		nbiot_printf("POWER_CTL: %x\n",ReadValueTemp);
		if(ReadValueTemp != 0x02){
			ErrorIndex = 0x03;
			Error_Handler();
		}
		
		nbiot_delay(200);
}

/*******************************************************************
  @brief void ADXL362_ReInit(void)
         reinitial and configure ADXL362
  @param
				 none
  @return
         none
*******************************************************************/
void ADXL362_ReInit(uint8_t thresh_act_h, uint8_t thresh_act_l, uint8_t thresh_inact_h, uint8_t thresh_inact_l, uint8_t time_inact_h, uint8_t time_inact_l, uint8_t filter_ctl)
{
		ADXL362RegisterWrite(XL362_SOFT_RESET,0x52);   						// software reset
		nbiot_delay(1000);
		ADXL362RegisterWrite(XL362_THRESH_ACT_L,thresh_act_l);						//set active threshold equip 350mg
		ADXL362RegisterWrite(XL362_THRESH_ACT_H,thresh_act_h);
		ADXL362RegisterWrite(XL362_THRESH_INACT_L,thresh_inact_l);					//set inactive threshold equip 150mg
		ADXL362RegisterWrite(XL362_THRESH_INACT_H,thresh_inact_h);
		ADXL362RegisterWrite(XL362_TIME_INACT_L,time_inact_l);						//set inactive time equip 1/12.5s
		ADXL362RegisterWrite(XL362_TIME_INACT_H,time_inact_h);
		ADXL362RegisterWrite(XL362_ACT_INACT_CTL,0x3F);						//configure loop mode,enable active and inactive
		ADXL362RegisterWrite(XL362_INTMAP1,0x40);									//configure awake map INT1
		ADXL362RegisterWrite(XL362_INTMAP2,0x00);									//configure awake map INT2
		ADXL362RegisterWrite(XL362_FIFO_CONTROL,0x00);						//select fifo stream mode//0x0E
		ADXL362RegisterWrite(XL362_FIFO_SAMPLES,0x00);						//select fifo sample number//0xFF
		ADXL362RegisterWrite(XL362_FILTER_CTL,filter_ctl);             	//select 2g range,ODR:12.5Hz
    //any changes to the registers before the POWER_CTL register (Register 0x00 to Register 0x2C) should be made with the device in standby
    ADXL362RegisterWrite(XL362_POWER_CTL,0x0A);              	//select measurement mode
		nbiot_delay(200);
}
