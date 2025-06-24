#include <stdint.h>
#include <TM4C123.h>

//---------------------------------------------------------------------------------------
//Clock
void setupTimer(void);
void clock12Hour(void);
void clock24Hour(void);

volatile uint32_t count = 0;                 //global count for the clock, increments once per second
volatile uint32_t count2 = 0;
int flag = 0;       				//if flag = 1 pm if flag = 0 am
int timeflag = 0;           //timeflag = 0 (12 hour mode); timeflag = 1 (24 hour mode)	
int receiverCount = 0;
void hourSetting(void);
void minuteSetting(void);

//12 hours
volatile int seconds_12;
volatile int minutes_12;
volatile int hours_12; 	 
int secArray[2];	
int minArray[2];
int hourArray[2];	

//24 hours
volatile int seconds_24;
volatile int minutes_24;
volatile int hours_24;
int secArray24[2];	
int minArray24[2];
int hourArray24[2];	

//---------------------------------------------------------------------------------------
//Alarm
void hackDelay(void);
void alarmLED(void);

void activateAlarm(void);
void deactivateAlarm(void);
void setAlarm(void);
void resetTimer(void);
void transmitDelay(void);
volatile uint32_t alarmMatch = 0;



//---------------------------------------------------------------------------------------
//Potentiometer

void hackPot(void);
void initPot2(void);
volatile uint32_t potActive = 0;
volatile int potFlag = 1;
void pot_setup(void);


//---------------------------------------------------------------------------------------
//Buzzer
void GPIOF_Setup(void);
void sysTickSetup(void);                     //piezo buzzer function
volatile int buzzerFlag = 0;            //Buzzer off



//---------------------------------------------------------------------------------------
//Keypad
#define rows 0xF0
#define columns 0xE
void rows_setup(void);
void columns_setup(void);
void nvic_column(void);
volatile int j = 0;
void delay(void);
volatile int k;
volatile int p = 0;   //Must match timeflag value to start
void column_1(void);
void column_2(void);
void column_3(void);
volatile int y = 0;
volatile int read_sec;



//---------------------------------------------------------------------------------------
//SysTick (Dashes)

int SettingTime = 0;         //0 if time setting is off, 1 if time setting is on   
volatile int l = 0;

 
//---------------------------------------------------------------------------------------
//7 segment display
#define gpioa_ssi 0x24           //PA2,5
void setup_ssi0(void);
void delay(void);
volatile int i = 0;
int segdisp[] = {~0x3F, ~0x6, ~0x5B, ~0x4F, ~0x66, ~0x6D, ~0x7D, ~0x7, ~0x7F, ~0x6F};      //0 - 9 seven segment display (AM)
int segdispPM[] = {~0xBF, ~0x86, ~0xDB, ~0xCF, ~0xE6, ~0xED, ~0xFD, ~0x87, ~0xFF, ~0xEF};      //0 - 9 seven segment display (PM)	
int num;
int dash = ~0x40;	
void twelve_hourLatch(void);
void twentyfour_hourLatch(void);


	
//MAIN FUNCTION	
int main(void)
{
	__disable_irq();

	setupTimer();
	setup_ssi0();
	rows_setup();
	columns_setup();
	nvic_column();
	GPIOF_Setup();
	//alarmLED();
	//sysTickSetup();
	//activateAlarm();
	//resetTimer();
	//initPot2();
	pot_setup();
	
	__enable_irq();
	
	while(1)
	{
		
		//Keypad row cycling
		j = j%4;
		GPIOC->DATA &= ~(1<<(3+j));           //Turn off the previous row
		GPIOC->DATA |= (1<<(4+j));          //Turn on the next row
		j++;
		
		if(j == 4)
		{
			GPIOC->DATA &= ~(1<<7);         //Clear bit 7
		}
		
	
		
		//Clock
		if(timeflag == 0)    //12 hour mode
		{
			clock12Hour();
			twelve_hourLatch();
		}
		else if(timeflag == 1)     //24 hour mode
		{
			clock24Hour();
			twentyfour_hourLatch();
		}
		
			
	}

	
}



//-------------------------- Interrupt Service Routine -----------------------------
//Keypad
void GPIOB_Handler(void)
{
	GPIOB->ICR |= columns;         //Clear interrupt status
	
	if((GPIOB->DATA&(1<<1)) == (1<<1))         //Column 1
	{
		column_1();
	}
	
	else if((GPIOB->DATA&(1<<2)) == (1<<2))    //Column 2
	{
		column_2();
	}
	
	else if((GPIOB->DATA&(1<<3)) == (1<<3))    //Column 3
	{
		column_3();
	}
}




//Clock
void TIMER2A_Handler(){  

	while((TIMER2 -> RIS)!= 0x1);    		//wait for first bit of RIS register to set
	TIMER2 -> ICR |= 0x1;
	count2++;  

	if(count2 == 2)             //Toggle dashes every 0.5s
	{
		dash = ~0x40;        //Dash on
	}
		
	
	else if(count2 == 4)       //Waits 4 counts of the 0.25s delay and 
	{           
		count2 = 0;              //then adds one to the second counter
		count++;
		dash = 0xFF;        //Dash off
	}

}	//increment the clock count once per second 


void SysTick_Handler()
{                    //Systick is responsible for piezzo buzzer
	int o;      
	
	//Dashes
	//When SettingTime = 0, time setting mode is off. Dashes are flashing
	if(SettingTime == 0)
	{
		if(l<500)
		{
			dash = ~0x40;	
		}
		else if((l>=500) && (l<1000))
		{
			dash = 0xFF;
		}
		l++;
		l = l%10;
		
	}
	//When SettingTime = 1, time setting mode is on. Dashes are off
	else if(SettingTime == 1)
	{
		dash = 0xFF;           
	}
	
	
	//Buzzer
	if(buzzerFlag == 1)
	{
		GPIOF -> DATA |= 0x10;
		for(o = 0; o < 25000; o++);
		GPIOF -> DATA &= ~0x10;
	}
	
	

}


//----------------------------- Functions -----------------------------------------------------
//7 segment display
//SSI0 setup (PA2,5)  
void setup_ssi0(void)
{
	SYSCTL->RCGCSSI |= 0x1;                    //Enable clock for SSI0
	while((SYSCTL->PRSSI&0x1) == 0);
	
	SYSCTL->RCGCGPIO |= 0x1;                  //Enable clock for Port A
	while((SYSCTL->PRGPIO&0x1) == 0);
	
	//PA3
	GPIOA->DIR |= 0x8; 	//Output 
	GPIOA->AFSEL &= ~0x8;              //Regular GPIO
	GPIOA->DEN |= 0x8;                  //Digital enable
	
	
	 //PA2,5
	GPIOA->DIR |= gpioa_ssi;            //Output 
	GPIOA->AFSEL |= gpioa_ssi;          //Alternate 
	GPIOA->PCTL &= ~0xF00F00;
	GPIOA->PCTL |= 0x200200;
	GPIOA->DEN |= gpioa_ssi;            //Digital enable
	
	SSI0->CR1 &= ~0x6;             //Disable SSI, set as controller
	SSI0->CPSR &= ~0xFF;            //Clearing CPSDVSR
	SSI0->CPSR |= 0xFE;            //SSI Clock Prescale divisor
	SSI0->CR0 &= ~0xFFFF;          //Clearing SCR, SPO=0, SPH=0, Freescale
	SSI0->CR0 |= 0xFF00;              //SSI Serial clock rate 
	SSI0->CR0 |= 0x7;               //8 bit data
	SSI0->CR1 |= 0x2;              //Enable SSI
	
}



//delay
void delay(void)
{
	for(k=0; k<24000; k++)
	{
		__asm("nop");
	}
}




//---------------------------------------------------------------------------------------
//Clock
void setupTimer(){
	SYSCTL -> RCGCGPIO |= 0x2;       		//GPIOB Port Setup
	GPIOB -> DEN |= 0x1;
	GPIOB -> DIR |= 0x1;
	GPIOB -> AFSEL |= 0x1;
	GPIOB -> PCTL |= 0x7;
	
	SYSCTL -> RCGCTIMER |= 0x4;      		//Timer Clock
	SYSCTL -> PRTIMER |= 0x4;
	
	TIMER2 -> CTL &= ~0x1;         			//Disable timer2
	TIMER2 -> CFG |= 0x4;
	TIMER2 -> TAMR |= 0x2;

	TIMER2 -> TAILR |= 0xBC20;    			//Preload setup
	TIMER2 -> TAPR &= ~0xBE;
	TIMER2 -> TAPR |= 0xBE;
	
	//set interrupt priority
	TIMER2 -> IMR |= 0x1;         			//Interrupt setup
	NVIC	-> ISER[0] |= (1<<23);
	NVIC->IP[5] |= (1<<29);            //Priority 1
	
	TIMER2 -> ICR |= 0x1;         			
	TIMER2 -> CTL |= 0x1;            //Enable timer
	
}	





void clock12Hour(void){
	//algorithm for 12 hour clock
	//if flag = 1 pm if flag = 0 am
	
	seconds_12 = count % 60;							//algorithm for 12 hour clock
	minutes_12 = (count / 60) % 60;
	
	if((count>=1) && (count<=3599))
	{
		hours_12 = 12;
	}
	
	else if((count>=3600) && (count<=43199))
	{
		hours_12 = (count / 3600) % 12;
	}
	
	else if((count>=43200) && (count<=86398))                    //PM Time  (43200 is 12 hours in seconds)
	{	
		flag = 1;
		
		if((count>=43200) && (count<=46800))
		{
			hours_12 = ((count / 3600) % 60);
		}
		else if((count>=46801) && (count<=86398))
		{
			hours_12 = ((count / 3600) % 60) -12;
		}
		
	}
	
	else
	{   
		hours_12 = 12;
		flag = 0;                           //AM Time
		count = 0;
		
	}
}


void clock24Hour(void){
	
	
	seconds_24 = count % 60;							//algorithm for 24 hour clock
	minutes_24 = (count / 60) % 60;
	
	if(count == 86399)
	{
		hours_24 = (count / 3600) % 24;
		count = 0;
	}
	else
	{	
		hours_24 = (count / 3600) % 24;
	}	
}


//12 hour display latch
void twelve_hourLatch(void)
{
	//From the clock
		for(i=0; i<2; i++)
		{
			secArray[i] = seconds_12 % 10;          //Putting numbers into an array
			seconds_12 = seconds_12 / 10;

			minArray[i] = minutes_12 % 10;          //Putting numbers into an array
			minutes_12 = minutes_12 / 10;

			hourArray[i] = hours_12 % 10;          //Putting numbers into an array
			hours_12 = hours_12 / 10;

		}
		
		if(flag == 0)             //AM
		{
			num = secArray[0];
			SSI0->DR = segdisp[num];
			delay();
		}
		
		if(flag == 1)             //PM
		{
			num = secArray[0];
			SSI0->DR = segdispPM[num];
			delay();
		}
		
		
		num = secArray[1];
		SSI0->DR = segdisp[num];
		delay();
		SSI0->DR = dash;
		delay();
		num = minArray[0];
		SSI0->DR = segdisp[num];
		delay();
		num = minArray[1];
		SSI0->DR = segdisp[num];
		delay();
		SSI0->DR = dash;
		delay();
		num = hourArray[0];
		SSI0->DR = segdisp[num];
		delay();
		num = hourArray[1];
		SSI0->DR = segdisp[num];
		delay();
			
		GPIOA->DATA |= 0x8;              //Latch data onto display	
		GPIOA->DATA &= ~0x8;
		delay();
}






//24 hour display latch
void twentyfour_hourLatch(void)
{
	//From the clock
		for(i=0; i<2; i++)
		{
			secArray24[i] = seconds_24 % 10;          //Putting numbers into an array
			seconds_24 = seconds_24 / 10;

			minArray24[i] = minutes_24 % 10;          //Putting numbers into an array
			minutes_24 = minutes_24 / 10;

			hourArray24[i] = hours_24 % 10;          //Putting numbers into an array
			hours_24 = hours_24 / 10;

		}
		
	
		num = secArray24[0];
		SSI0->DR = segdisp[num];
		delay();
	
		num = secArray24[1];
		SSI0->DR = segdisp[num];
		delay();
		
		SSI0->DR = dash;
		delay();
		
		num = minArray24[0];
		SSI0->DR = segdisp[num];
		delay();
		
		num = minArray24[1];
		SSI0->DR = segdisp[num];
		delay();
		
		SSI0->DR = dash;
		delay();
		
		num = hourArray24[0];
		SSI0->DR = segdisp[num];
		delay();
		
		num = hourArray24[1];
		SSI0->DR = segdisp[num];
		delay();
			
		GPIOA->DATA |= 0x8;              //Latch data onto display	
		GPIOA->DATA &= ~0x8;
		delay();
}

//---------------------------------------------------------------------------------------
//Alarm
void hackDelay(){
	int i;
	
	for(i = 0; i < 1000000; i++);
}

void alarmLED(){
	while(1){
	hackDelay();
	GPIOF -> DATA &= ~0x4;											//red is 0x2 blue is 0x4
	GPIOF -> DATA |= 0x2;
	hackDelay();
	GPIOF -> DATA &= ~0x2;
	GPIOF -> DATA |= 0x4;


		}
	
}

void activateAlarm(){
	buzzerFlag = 1;            //Buzzer goes off
	sysTickSetup();                        //piezo buzzer
	alarmLED();														//activate alarm LEDs
}

void deactivateAlarm(){
	buzzerFlag = 0;
	GPIOF -> DATA &= ~0x16;
}

void hourSetting(){
	potActive = 0;                           //turn off pots and wait then turn them back on 
	hackDelay();                             //so that the count is changed correctly and then the other pot function can be used
	potActive = 1;
	potFlag = 1;
	hackPot();
	
}

void minuteSetting(){
	potActive = 0;
	hackDelay();
	potActive = 1;
	potFlag = 0;
	hackPot();
}


void resetTimer(){
	potActive = 0;
	TIMER2 -> CTL |= 0x1;             //turn on timer
}

void transmitDelay(){
	int i;
	for(i = 0; i < 25000; i++) 
	{
		__asm("nop");
	}
}


//----------------------------------------------------------------------------
//Buzzer
void GPIOF_Setup(){
	SYSCTL -> RCGCGPIO |= 0x20;						//GPIOF setup
	GPIOF -> DIR |= 0x16;
	GPIOF -> DEN |= 0x16;
}

void sysTickSetup(){
	SysTick -> CTRL &= ~0x1;            //setup and preload value
	SysTick -> LOAD |= 0xC350;
	SysTick -> VAL |= 0x1;
	
	SysTick -> CTRL |= 0x3;            //enable interrupts
	
	
}








//---------------------------------------------------------------------------------------

//For keypad rows (1 - 4)
//GPIO Port C setup (pins 4 - 7)
void rows_setup(void)
{
	SYSCTL->RCGCGPIO |= (1<<2);                   //Turn on Port C clock
	while((SYSCTL->PRGPIO&(1<<2)) == 0);
	
	GPIOC->DIR |= rows;                       //Output	
	GPIOC->AFSEL 	&= ~rows;                    //Regular GPIO
	GPIOC->DEN |= rows;                        //Digital enable
}


//For keypad columns (1 - 3)
//GPIO Port B (pins 1 - 3)
void columns_setup(void)
{
	SYSCTL->RCGCGPIO |= (1<<1);                   //Turn on Port B clock
	while((SYSCTL->PRGPIO&(1<<1)) == 0);
	
	GPIOB->DIR &= ~columns;                       //Input	
	GPIOB->AFSEL 	&= ~columns;                    //Regular GPIO
	GPIOB->PDR |= columns;                       //Pull down
	GPIOB->DEN |= columns;                        //Digital enable
	
	GPIOB->IM &= ~columns;                   //Clear interrupt
	GPIOB->IS &= ~columns;                   //Edge triggered
	GPIOB->IEV |= columns;                    //Rising edge
	GPIOB->IBE &= ~columns;                   //Single edge
	GPIOB->ICR |= columns;         //Clear interrupt status
	GPIOB->IM |= columns;                    //Enable interrupt
}

//NVIC setup for Port B (Keypad columns)
void nvic_column(void)
{
	NVIC->ISER[0] |= (1<<1);              //IRQ = 1
	NVIC->IP[0] |= (4<<13);                //Priority 4
}


//Column 1 keypad
void column_1(void)
{
	if((GPIOC->DATA&(1<<4)) == (1<<4))        //Row 1
	{
		//1
		
	}
	
	else if((GPIOC->DATA&(1<<5)) == (1<<5))        //Row 2
	{
		//4
		TIMER2->CTL &= ~0x1;      //Pause clock
		potFlag = 1;
		potActive = 1;
		hackPot();
	}
	
	else if((GPIOC->DATA&(1<<7)) == (1<<7))        //Row 4
	{
		//*
		potActive = 0;
		TIMER2->CTL |= 0x1;
	}
}


//Column 2 keypad
void column_2(void)
{
	if((GPIOC->DATA&(1<<4)) == (1<<4))        //Row 1
	{
		//2
	}
	
	else if((GPIOC->DATA&(1<<5)) == (1<<5))        //Row 2
	{
		//5
	}
	
	else if((GPIOC->DATA&(1<<7)) == (1<<7))        //Row 4
	{
		//0 (Toggle between 24 hour time and 12 hour time)
		p++;
		p = p%2;
		timeflag = p;
		
	}
}


//Column 3 keypad
void column_3(void)
{
	if((GPIOC->DATA&(1<<4)) == (1<<4))        //Row 1
	{
		//3
	}
	
	else if((GPIOC->DATA&(1<<5)) == (1<<5))        //Row 2
	{
		//6
		
		read_sec = count%60;                   //Read value in seconds
		
		if(timeflag == 0)    //12 hour mode
		{
			if(y == 0)
			{
				count = count - read_sec;             //Toggle seconds to 0s
				y++;
			}
			else if(y == 1)
			{
				count = count - read_sec + 30;       //Toggle seconds to 30s
				y = 0;
			}
			
		}
		else if(timeflag == 1)     //24 hour mode
		{
			if(y == 0)
			{
				count = count - read_sec;             //Toggle seconds to 0s
				y++;
			}
			else if(y == 1)
			{
				count = count - read_sec + 30;       //Toggle seconds to 30s
				y = 0;
			}
			
		}
		
	}
	
	else if((GPIOC->DATA&(1<<7)) == (1<<7))        //Row 4
	{
		//#
	}
}







//---------------------------------------------------------------------------------------
//Potentiometer
void pot_setup(void)
{
	SYSCTL->RCGCGPIO |= (1<<4);   				//ADC and GPIO Clocks
	SYSCTL->RCGCADC |= (1<<0);    

																				/* initialize PE3 for AIN0 input  */
	GPIOE->AFSEL |= (1<<3);      					//set alt function
	GPIOE->DEN &= ~(1<<3);        				//clear digital to disable
	GPIOE->AMSEL |= (1<<3);       				//enable analog


																				/* initialize sample sequencer3 */
	ADC0->ACTSS &= ~(1<<3);        				
	ADC0->EMUX &= ~0xF000;    						
	ADC0->SSMUX3 = 0;        						  
	ADC0->SSCTL3 |= (1<<1)|(1<<2);        
	ADC0->ACTSS |= (1<<3);         				//enable sequencer
}

	


void hackPot()
{
	unsigned int adcVal;
	volatile int potCount;
	volatile int potCount2;
	int var; 
	int varMin;
	int potHours;
	int potMin;				
  
	
	while(((GPIOC->DATA&(1<<7)) != (1<<7)) && ((GPIOB->DATA&(1<<1)) != (1<<1)))             //while the function hasnt been called to stop getting pot reading
	{
		//Keypad row cycling
				j = j%4;
				GPIOC->DATA &= ~(1<<(3+j));           //Turn off the previous row
				GPIOC->DATA |= (1<<(4+j));          //Turn on the next row
				j++;
				
				if(j == 4)
				{
					GPIOC->DATA &= ~(1<<7);         //Clear bit 7
				}
				
			
		
		ADC0->PSSI |= (1<<3);        			
		while((ADC0->RIS & 8) == 0);   	//Wait untill sample conversion completed
		adcVal = ADC0->SSFIFO3; 					
		ADC0->ISC = 8;          					

		/*Algorithm to turn adc value into 0-12, 0-24, 0-60 for clock*/

		if (potFlag == 1){                //if pot flag 1 hour setting
			potCount = count;
			potCount = potCount / 3600;     //find number of hours passed  - integer division means that it will always round down

			potCount = potCount * 3600;     //potCount is no hours passed to take off of the count

			potHours = adcVal /170;         //find what the potentiometer reading is 
			var = potHours * 3600;          //make var no of counts for potHours (what to add to the count)

			count = count - potCount;      //take no hours passed off the count
			count = var + count;           //add the new no hours to the count	
		}

		else if(potFlag == 0)
		{           //if potFlag 0 minute setting
			potCount2 = count;
			potCount2 = potCount2 / 60;    //find no of minutes passed using integer division

			potCount2 = potCount2 * 60;    //potCount to is no minutes passed to take off the count

			potMin = adcVal / 68;
			varMin = potMin * 60;

			count = count - potCount2;
			count = varMin + count;
				
   


			while(((GPIOC->DATA&(1<<7)) != (1<<7)) && ((GPIOB->DATA&(1<<1)) != (1<<1)))
			{
				//Keypad row cycling
				j = j%4;
				GPIOC->DATA &= ~(1<<(3+j));           //Turn off the previous row
				GPIOC->DATA |= (1<<(4+j));          //Turn on the next row
				j++;
				
				if(j == 4)
				{
					GPIOC->DATA &= ~(1<<7);         //Clear bit 7
				}
					
				ADC0->PSSI |= (1<<3);        			
				while((ADC0->RIS & 8) == 0) ;   	//Wait untill sample conversion completed
				adcVal = ADC0->SSFIFO3; 					
				ADC0->ISC = 8;          					
				
				
					
					
				
			}
		}
	}

}







