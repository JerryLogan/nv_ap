#include "MS51_16K.H"
#include "isp_uart1.h"

/**
 ******************************************************************************
 *@Pin Definition
 * stm8s003							//novuton
 * PA1 PWR_ON_INT				//p3.0
 * PA2 MCU_UPDATE				//p1.7
 * PB4 GAUGE_SCL 				//p1.3
 * PB5 GAUGE_SDA				//p1.4
 * PC4 BAT_DET_LED_R		//p1.1
 * PC5 BAT_DET_LED_G		//p1.0
 * PC7 BAT_DET_LED_O		//p0.1
 * PC6 interrupt				//p0.0
 *
 ******************************************************************************
 */
 
 

#define Debug_EVB								//EVB MS51FB
#define RED_LED_ON      0x52
#define GREEN_LED_ON    0x47
#define ORANGE_LED_ON   0x4F
#define RED_LED_OFF     0x72
#define GREEN_LED_OFF   0x67
#define ORANGE_LED_OFF  0x6F
//#define CMD_GET_FWVER   0xF0  //reserve 0xF0~0xFE
#define POWER_OFF_MODE  0x00
#define POWER_ON_MODE   0x01
#define FW_VER          0xA5 //A=DPB30, B=DPB60, C=DPB70; Ver=5
#define CMD_NOT_SUPPORT 0xFF

static uint8_t power_mode = POWER_OFF_MODE;
uint8_t ReceivedData = 0;

void myDelay(unsigned int value);
void UART1_Config(void); 
void InitLEDPins(void);
void ResetLEDPins(void);
void GPIO_Config(void);

void GPIO_Config(void)
{
#ifdef Debug_EVB
	P12_QUASI_MODE;//evb LED
#else
	P01_QUASI_MODE;//Orange
	P10_QUASI_MODE;//Green
	P11_QUASI_MODE;//Red
	P30_QUASI_MODE;//PWR_ON_INT
	P17_QUASI_MODE;//MCU_UPDATE
#endif
}

void InitLEDPins(void)
{
#ifdef Debug_EVB
	P12_QUASI_MODE;//evb LED
#else
	P01_QUASI_MODE;//Orange
	P10_QUASI_MODE;//Green
	P11_QUASI_MODE;//Red
#endif
	ResetLEDPins();
}

void ResetLEDPins(void)
{
	//Turn off LED
#ifdef Debug_EVB
	P12 = 0x00;//EVB
#else
	P01 = 0x00;//Orange
	P10 = 0x00;//Green
	P11 = 0x00;//Red
#endif
}
void myDelay(unsigned int value)
{
	//16MHz, 5 clock cycles
	unsigned int i;
	unsigned int j;
	for(i = 0; i < value; i++)
		for(j = 0; j < 1600; j++); 
}

void UART1_Config(void)
{
	P06_QUASI_MODE;//UART0_TX
	P07_INPUT_MODE;//UART0_RX
	UART_Open(16000000,UART0,9600);
}

void main() {

	/*Clock configuration */
	MODIFY_HIRC(HIRC_16);//16Mhz
	
	/*GPIO configuration */
	GPIO_Config();

	
	/*I2C configuration */
	//todo I2C_GasGauge_Init();
	
	/*LED GPIO configuration */
	InitLEDPins();

	if(0)//todo(!(GPIOA->IDR&(1<<1)))
	{
		power_mode = POWER_OFF_MODE;
		//todo EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOC, EXTI_SENSITIVITY_FALL_ONLY);
	}
	else
	{
		power_mode = POWER_ON_MODE;
		/*UART1 configuration */
		UART1_Config();
	}

	while(1)
	{
		if(power_mode == POWER_ON_MODE) {
			ReceivedData = 0;
			ReceivedData = Receive_Data(UART0);
			ENABLE_UART0_PRINTF;
/*
P01 = 0x00;//Orange
P10 = 0x00;//Green
P11 = 0x00;//Red
*/
			switch(ReceivedData)
			{
				case RED_LED_ON:
					P11 = 0xff;//Red
#ifdef Debug_EVB
					P12 = 0xff;//Red
#endif
					break;
				case GREEN_LED_ON:
					P10 = 0xff;//Green
					break;
				case ORANGE_LED_ON:
					P01 = 0xff;//Orange
					break;
				case RED_LED_OFF:
					P11 = 0x00;//Red
#ifdef Debug_EVB
					P12 = 0x00;//Red
#endif
					break;
				case GREEN_LED_OFF:
					P10 = 0x00;//Green
					break;
				case ORANGE_LED_OFF:
					P01 = 0x00;//Orange
					break;
				case CMD_GET_FWVER:
					//todo Transmit(FW_VER);
					break;
				
				default:
					//todo Transmit(CMD_NOT_SUPPORT);
					break;
			}
		}	else {
			ResetLEDPins();
			//todo halt();
		}
		
		/*
		P12 = 0xff;
		myDelay(100);
		P12 = 0x00;
		myDelay(100);
		*/
	}
}