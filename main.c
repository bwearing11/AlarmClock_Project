#include <stdint.h>
#include <TM4C123.h>
#include <TM4C123GH6PM.h>

#define gpiob_ssi 0xF0;

void setupTimer(void);
void hackDelay(void);
void alarmLED(void);
void hackPot(void);
void initPot2(void);
void clock12Hour(void);
void clock24Hour(void);
void GPIOF_Setup(void);
void sysTickSetup(void);                     //piezo buzzer function
void activateAlarm(void);
void deactivateAlarm(void);
void setAlarm(void);
void resetTimer(void);
void transmitDelay(void);

void hourSetting(void);
void minuteSetting(void);

void ssiMaster(void);

volatile uint32_t count = 3600;                 //global count for the clock, increments once per second
volatile uint32_t count2 = 0;
volatile uint32_t alarmMatch = 0;
volatile uint32_t potActive = 1;


//clock variables
  volatile int seconds_12;
	volatile int minutes_12;
	volatile int hours_12; 
	volatile int seconds_24;
	volatile int minutes_24;
	volatile int hours_24;
	
	//global pot flag
	volatile int potFlag = 0;
	int dash = ~0x40;


int main(void){
  setupTimer();
	GPIOF_Setup();
	//alarmLED();
	//sysTickSetup();
	//activateAlarm();
	//resetTimer();
	//initPot2();
	//hackPot();
	ssiMaster();
	
	
	
	
	while(1){	
		if(count2 >= 4){           //Waits 4 counts of the 0.25s delay and 
			count2 = 0;              //then adds one to the second counter
			count++;
			}
		}
	
	}


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
	
	
	TIMER2 -> IMR |= 0x1;         			//Interrupt setup
	NVIC	-> ISER[0] |= (1<<23);
	
	TIMER2 -> ICR |= 0x1;         			//Enable timer
	TIMER2 -> CTL |= 0x1;
	
}	


void TIMER2A_Handler(){
	while((TIMER2 -> RIS)!= 0x1);    		//wait for first bit of RIS register to set
	TIMER2 -> ICR |= 0x1;
	count2++;   
	
	if(count2 == 4)       //Waits 4 counts of the 0.25s delay and 
	{                         //then adds one to the second counter
		count++;
		SSI2 -> DR = count;
	}
	
	
	
	
}	

void clock12Hour(){
	//algorithm for 12 hour clock
		
	int flag;       											//if flag = 1 pm if flag = 0 am	
	seconds_12 = count % 60;							//algorithm for 12 hour clock
	minutes_12 = (count / 60) % 60;
	if(count > 43200){                    //PM Time  (43200 is 12 hours in seconds)
		flag = 1;
		hours_12 = ((count / 3600) % 60) -12;
	}
	else{           
		flag = 0;                           //AM Time
		hours_12 = (count / 3600) % 60;
	}
}

void clock24Hour(){
	seconds_24 = count % 60;							//algorithm for 24 hour clock
	minutes_24 = (count / 60) % 60;
	hours_24 = (count / 3600) % 60;
}



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

void hackPot(){
     unsigned int adcVal;
		 volatile int potCount;
		 volatile int potCount2;
		 int x;
	   int var; 
		 int varMin;
		 int potHours;
	   int potMin;
	
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
    
												
													/*HACK POT CODE - TESTING ONLY */
   SYSCTL->RCGCGPIO |= 0x20; 							//enable GPIO for green LED
   GPIOF->DIR       |= 0x08; 							
   GPIOF->DEN       |= 0x08;  						
   
while(potActive != 0)                               //while the function hasnt been called to stop getting pot reading
    {
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
			
			else if(potFlag == 0){           //if potFlag 0 minute setting
				potCount2 = count;
				potCount2 = potCount2 / 60;    //find no of minutes passed using integer division
				
				potCount2 = potCount2 * 60;    //potCount to is no minutes passed to take off the count
				
				potMin = adcVal / 68;
				varMin = potMin * 60;
				
				count = count - potCount2;
				count = varMin + count;
			}
	 }
}




void ssiMaster(){
	SYSCTL -> RCGCGPIO |= 0x2;        //clocks for gpio and ssi
	while((SYSCTL->PRGPIO&0x2) == 0);
	
	SYSCTL->RCGCSSI |= 0x4; 
	while((SYSCTL->PRSSI&0x4) == 0);
	
	
	GPIOB -> DIR |= gpiob_ssi;
	GPIOB -> AFSEL |= gpiob_ssi;
	GPIOB -> PCTL &= ~0xFFFF0000;
	GPIOB -> PCTL |=  0x22220000;
	GPIOB -> DEN |= gpiob_ssi;
	
	SSI2 -> CR1 &= ~0x6;
	SSI2->CPSR &= ~0xFF;           //Clearing CPSDVSR
	SSI2->CPSR |= 0xFE;            //SSI Clock Prescale divisor
	SSI2->CR0 &= ~0xFFFF;          //Clearing SCR, SPO=0, SPH=0, Freescale
	SSI2->CR0 |= 0xFF00;           //SSI Serial clock rate 
	SSI2->CR0 |= 0xF;              //16	 bit data
	
	SSI2->CR1 |= 0x2;              //Enable SSI
	
		
}

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

void SysTick_Handler(){                    //Systick is responsible for piezzo buzzer
	int i;                          
	GPIOF -> DATA |= 0x10;
	for(i = 0; i < 25000; i++);
	GPIOF -> DATA &= ~0x10;

}

void activateAlarm(){
	sysTickSetup();                        //piezo buzzer
	alarmLED();														//activate alarm LEDs
}

void deactivateAlarm(){
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




