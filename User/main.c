#include "MS51_16K.H"
#include "isp_uart1.h"

/**
 ******************************************************************************
 *@Pin Definition
 * stm8s003                         //novuton  
 * PA1 PWR_ON_INT                   //p3.0
 * PA2 MCU_UPDATE                   //p1.7
 * PB4 GAUGE_SCL                    //p1.3
 * PB5 GAUGE_SDA                    //p1.4
 * PC4 BAT_DET_LED_R                //p1.1
 * PC5 BAT_DET_LED_G                //p1.0
 * PC7 BAT_DET_LED_O                //p0.1
 * PC6 interrupt                    //p0.0
 ******************************************************************************
 */

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 
 
//#define Debug_EVB                 //EVB MS51FB
#define RED_LED_ON                0x52
#define GREEN_LED_ON              0x47
#define ORANGE_LED_ON             0x4F
#define RED_LED_OFF               0x72
#define GREEN_LED_OFF             0x67
#define ORANGE_LED_OFF            0x6F
//#define CMD_GET_FWVER   0xF0  //reserve 0xF0~0xFE
#define POWER_OFF_MODE            0x00
#define POWER_ON_MODE             0x01
#define FW_VER                    0xA5 //A=DPB30, B=DPB60, C=DPB70; Ver=5
#define CMD_NOT_SUPPORT           0xFF

#define I2C_CLOCK                 13
#define I2C_SLAVE_ADDRESS         0xAA
#define EEPROM_ADDRESS 0xAA
#define I2C_WR                    0 //(I2C_SLAVE_ADDRESS | I2C_WR) = #define WD_DADR			0xAA
#define I2C_RD                    1 //(I2C_SLAVE_ADDRESS | I2C_RR) = #define RD_DADR			0xAB
#define LOOP_SIZE                 1

//debug led
#define ooo  P01=ENABLE;P10=DISABLE;P11=DISABLE;
#define ogr	 P01=ENABLE;myDelay(500);P01=DISABLE;P10=ENABLE;myDelay(500);P10=DISABLE;P11=ENABLE;myDelay(500);P11=DISABLE;
#define ogrf  P01=ENABLE;myDelay(100);P01=DISABLE;P10=ENABLE;myDelay(100);P10=DISABLE;P11=ENABLE;myDelay(100);P11=DISABLE;
#define ggg  P01=DISABLE;P10=ENABLE;P11=DISABLE; //Green 
#define gggf  P01=DISABLE;P10=ENABLE;P11=DISABLE;myDelay(50);P10=DISABLE;myDelay(50);P10=ENABLE;myDelay(50);P10=DISABLE;myDelay(50);P10=ENABLE;myDelay(50);P10=DISABLE; //Green 
#define rrrf  P01=DISABLE;P10=DISABLE;P11=ENABLE;myDelay(50);P11=DISABLE;myDelay(50);P11=ENABLE;myDelay(50);P11=DISABLE;myDelay(50);P11=ENABLE;myDelay(50);P11=DISABLE; //redc
#define ooof  P01=ENABLE;P10=DISABLE;P11=DISABLE;myDelay(50);P01=DISABLE;myDelay(50);P01=ENABLE;myDelay(50);P01=DISABLE;myDelay(50);P01=ENABLE;myDelay(50);P01=DISABLE; //redc
#define rrr  P01=DISABLE;P10=DISABLE;P11=ENABLE; 
#define ccc  P01=DISABLE;P10=DISABLE;P11=DISABLE; 
#define all  P01=ENABLE;P10=ENABLE;P11=ENABLE;
//debug led

static uint8_t power_mode = POWER_OFF_MODE;
uint8_t ReceivedData = 0;//uart
uint8_t iData = 0;//i2c
static uint8_t first_pressed = TRUE;

void myDelay(unsigned int value);
void UART0_Config(void); 
void InitLEDPins(void);
void ResetLEDPins(void);
void GPIO_Config(void);
void power_down_mode(void);
void I2C_Error(void);
void I2C_Write_Process(UINT8 u8DAT);
void I2C_Read_Process(UINT8 u8DAT);
void Init_I2C(void);

void I2C_Error(void)
{
	while (1);    
}

void I2C_Write_Process(UINT8 u8DAT)
{
    unsigned char  u8Count;
    /* Write Step1 */
    set_I2CON_STA; /* Send Start bit */
    clr_I2CON_SI;
    while (!SI); /*Check SI set or not  */
    if (I2STAT != 0x08) //Start /*Check status value after every step   */
		I2C_Error();

    /* Write Step2 */
    clr_I2CON_STA; /*STA=0*/
    I2DAT = (I2C_SLAVE_ADDRESS | I2C_WR);
    clr_I2CON_SI;
    while (!SI); /*Check SI set or not */
    if (I2STAT != 0x18) //Master Transmit Address ACK           
        I2C_Error();

    /* Write Step3 */
    for (u8Count = 0; u8Count < LOOP_SIZE; u8Count++)
    {
		I2DAT = u8DAT;
		clr_I2CON_SI;
		while (!SI); /*Check SI set or not*/
        if (I2STAT != 0x28) //Master Transmit Data ACK              
            I2C_Error();
        u8DAT = ~u8DAT;        
    }

    /* Write Step4 */
    set_I2CON_STO;
    clr_I2CON_SI;
    while (STO); /* Check STOP signal */

}
	


void I2C_Read_Process(UINT8 u8DAT)//read 0x2c(soc register) from RT9426A
{
    unsigned char  u8Count;
    /* Read Step1 */
    set_I2CON_STA;
    clr_I2CON_SI;      
    while (!SI);                                //Check SI set or not
    if (I2STAT != 0x08)                         //Check status value after every step
        I2C_Error();

    /* Step13 */
    clr_I2CON_STA;                                    //STA needs to be cleared after START codition is generated
    I2DAT = (I2C_SLAVE_ADDRESS | I2C_WR);
	clr_I2CON_SI;
    while (!SI);                                //Check SI set or not
    if (I2STAT != 0x18)              
        I2C_Error();

	I2DAT = (0x2C);//read 0x2c(soc register) from RT9426A
	clr_I2CON_SI;
	while (!SI);                                //Check SI set or not
    if (I2STAT != 0x28)              
        I2C_Error();

	set_I2CON_STA; //repeat start
	clr_I2CON_SI;
	while (!SI); 
	if (I2STAT != 0x10)              
        I2C_Error();

	
	clr_I2CON_STA;
    I2DAT = (I2C_SLAVE_ADDRESS | I2C_RD);
	clr_I2CON_SI;
    while (!SI);                                //Check SI set or not
	if (I2STAT != 0x40)              
        I2C_Error();
	
	set_I2CON_AA;
	clr_I2CON_SI;  
	while (!SI); 
	if (I2STAT !=0x50)
		I2C_Error();
	
	iData = I2DAT;
	
	/* Step16 */
    set_I2CON_STO;
    clr_I2CON_SI;
	I2C0_SI_Check();

}

void power_down_mode(void)
{
	/*MUST DISABLE BOD to low power */
//	BOD_DISABLE;            /* Disable BOD for less power consumption*/

	/* Real into power down mode */

//set_PCON_POF;
	set_PCON_PD;

}

void PinInterrupt_ISR (void) interrupt 7
{
_push_(SFRS);

	if (PIF&=SET_BIT0)
	{
#ifdef Debug_EVB
		P12 = 0;//EVB LED on   //tododelete
#endif
		CLEAR_PIN_INTERRUPT_PIT0_FLAG;
	}
	while(P00 == DISABLE) //key release exit while //P00 = DPB state key
	{		
		Init_I2C();
		I2C_Read_Process(0x2C);//RT9426A StateOfCharge(SOC %)
		clr_I2CON_I2CEN;//TS_JerryLo 20230220 Must-do 

		if (iData>0x64){
			//ogrf;
		}else if(iData >= 0x43 && iData <= 0x64) { //67~100
			P01=DISABLE;P10=ENABLE;P11=DISABLE; //Green 
		} else if(iData >= 0x1F && iData < 0x43) { //31~66
			P01=ENABLE;P10=DISABLE;P11=DISABLE; //Orange
		} else if(iData < 0x1F) { //<=30
			P01=DISABLE;P10=DISABLE;P11=ENABLE; //Red
		} else {
			//ogr;
		}
		myDelay(100);
	}
	ResetLEDPins();//clear all led after key release
_pop_(SFRS);
}
 
void GPIO_Config(void)
{
#ifdef Debug_EVB
	P12_PUSHPULL_MODE;//evb LED
#endif
	P01_PUSHPULL_MODE;//Orange
	P10_PUSHPULL_MODE;//Green
	P11_PUSHPULL_MODE;//Red
	P30_QUASI_MODE;//PWR_ON_INT
	P17_QUASI_MODE;//MCU_UPDATE
}

void InitLEDPins(void)
{
#ifdef Debug_EVB
	P12_PUSHPULL_MODE;//evb LED
#endif
	P01_PUSHPULL_MODE;//Orange
	P10_PUSHPULL_MODE;//Green
	P11_PUSHPULL_MODE;//Red
	ResetLEDPins();
}

void ResetLEDPins(void)
{
	//Turn off LED
#ifdef Debug_EVB
	P12 = DISABLE;//EVB
#endif
	P01 = DISABLE;//Orange
	P10 = DISABLE;//Green
	P11 = DISABLE;//Red
}

void myDelay(unsigned int value)
{
	/*16MHz, 5 clock cycles*/
	unsigned int i,j;
	for(i = 0; i < value; i++)
		for(j = 0; j < 1600; j++); 
}


void Init_I2C(void)
{
    P13_OPENDRAIN_MODE;          // Modify SCL pin to Open drain mode. don't forget the pull high resister in circuit
    P14_OPENDRAIN_MODE;          // Modify SDA pin to Open drain mode. don't forget the pull high resister in circuit

    /* Set I2C clock rate */
    I2CLK = I2C_CLOCK; 

    /* Enable I2C */
    set_I2CON_I2CEN;                                 
}

void UART0_Config(void)
{
	P06_QUASI_MODE;//UART0_TX
	P07_QUASI_MODE;//UART0_RX
	UART_Open(16000000,UART0,9600);
	//ENABLE_UART0_PRINTF;
	//printf("\n infuntion UART0_Config");
}

void main() {
	
	/*Clock configuration */
	MODIFY_HIRC(HIRC_16);//16Mhz
	
	/*GPIO configuration */
	GPIO_Config();

	/*I2C configuration */
	//todo I2C_GasGauge_Init();
	Init_I2C();
	
	/*LED GPIO configuration */
	InitLEDPins();


	//UART0_Config();// only for test. need to delete
	//printf ("\n\n**** FW_VER = %bX",FW_VER);/ only for test. need to delete
	//printf("\nPO = "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(P0));/ only for test. need to delete
	//printf("\nP3 = "BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(P3));

	if(P30 == DISABLE)//PA1 low
	{
		power_mode = POWER_OFF_MODE;
		
		P00_INPUT_MODE;//P00 = PC6 (BTPAIR_BATDET)
		P00 = 0;
		ENABLE_INT_PORT0;
		ENABLE_BIT0_LOWLEVEL_TRIG;
		ENABLE_PIN_INTERRUPT;
		ENABLE_GLOBAL_INTERRUPT;
	}
	else//PA1 high
	{
		power_mode = POWER_ON_MODE;
		
		/*UART configuration */
		UART0_Config();	
	}
	
	while(1)
	{
		if(power_mode == POWER_ON_MODE) 
		{
			ReceivedData = 0;
			ReceivedData = Receive_Data(UART0);

			switch(ReceivedData)
			{
				case RED_LED_ON:					
#ifdef Debug_EVB
					P12 = ENABLE;//Red
#endif
					P11 = ENABLE;break;
				
				case GREEN_LED_ON:
					P10 = ENABLE;break;
				
				case ORANGE_LED_ON:
					P01 = ENABLE;break;
				
				case RED_LED_OFF:						
#ifdef Debug_EVB
					P12 = DISABLE;//Red
#endif
					P11 = DISABLE;break;
				
				case GREEN_LED_OFF:
					P10 = DISABLE;break;
				
				case ORANGE_LED_OFF:
					P01 = DISABLE;break;
				
				case CMD_GET_FWVER:
					UART_Send_Data(UART0,FW_VER);break;
				
				default:
					UART_Send_Data(UART0,CMD_NOT_SUPPORT);break;
			}
		} 
		else 
		{
			ResetLEDPins();
			power_down_mode();
		}
		myDelay(100);
	}
}