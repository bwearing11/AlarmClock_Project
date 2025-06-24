#include <stdint.h>
#include <TM4C123.h>
//timer function(s)
void setupTimer(void);
//clock functions
void GPIOF_Setup(void);
void clock12Hour(void);
void clock24Hour(void);
//alarm functions
void activateAlarm(void);
void deactivateAlarm(void);
void alarmLED(void);
void buzzerActivate(void);
void setAlarm(void);
 //potentiometer
void initPot(void);

volatile uint32_t count = 0;                 //global count for the clock, increments once per second
volatile uint32_t match = 0;								 //global alarm match, in total seconds
	
// Assume a system clock of 50MHz
int main(void) {
	//GPIOF_Setup();
	setupTimer();
	initPot();
	
	while (1);
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
	
	TIMER2 -> TAILR |= 0xFFFF;    			//Preload setup
	TIMER2 -> TAPR &= ~0x2FB;
	TIMER2 -> TAPR |= 0x2FB;
	
	//set interrupt priority
	TIMER2 -> IMR |= 0x1;         			//Interrupt setup
	NVIC	-> ISER[0] |= (1<<23);
	
	TIMER2 -> ICR |= 0x1;         			//Enable timer
	TIMER2 -> CTL |= 0x1;
	
}	

void TIMER2A_Handler(){
	while((TIMER2 -> RIS)!= 0x1);    		//wait for first bit of RIS register to set
	TIMER2 -> ICR |= 0x1;
	count++;                           	//increment the clock count once per second 

	activateAlarm();                    //check if the alarm is set an activate if match is met
}


void clock12Hour(){
	//algorithm for 12 hour clock
	int seconds_12;
	int minutes_12;
	int hours_12; 	
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
	int seconds_24;
	int minutes_24;
	int hours_24;
	
	seconds_24 = count % 60;							//algorithm for 24 hour clock
	minutes_24 = (count / 60) % 60;
	hours_24 = (count / 3600) % 60;
	
}

void GPIOF_Setup(){
	
}

void activateAlarm(){
	if(match == 0){                 //check if match is 0, meaning alarm hasnt been set
		return;
	}
	else if(match == count){				//check if the match is the count meaning set off the alarm
																	//turn off yellow LED
		alarmLED();                   //set off red and blue led alternating
		buzzerActivate();							//activate piezo buzzer															
	}
}

void deactivateAlarm(){
	//turn off piezo buzzer
	//turn off alarm lights
}

void alarmLED(){
	//set off LED alternating red and blue
	GPIOF_Setup();
	int i, j;
	for(j = 0; j > 25000; j++){									//overall loop
		GPIOF -> DATA &= ~0x4;										//clear LED
		GPIOF -> DATA |= 	0x1;										//set red
	
		for(i = 0; i > 2500; i++);								//wait before turning on other LED
		
		GPIOF -> DATA &= ~0x1;										//clear red
		GPIOF -> DATA |= 0x4;											//set blue
	}
}

void buzzerActivate(){
	//DAC setup and turn on buzzer
}

void setAlarm(){
	int hourSec;				//after coversion to seconds
	int minSec;
	int secSec;
	
	int hour;						//will be data from keypad
	int min;
	int sec;
	int tot;
	
	//take in data from keypad
	
	//convert time into total seconds (algorithm to covert hours/mins/seconds into total)
	hourSec = hour * 60 * 60;
	minSec = min * 60;
	secSec = sec;
	
	tot = hourSec + minSec + secSec;
	//set match as total seconds
	match = tot;
	//turn on yellow LED
	GPIOF -> DATA |= 0xA;
}

void initPot(){
	SYSCTL -> RCGCGPIO |= (1<<4);              //GPIO & ADC Clocks
	SYSCTL -> RCGCADC |= (1<<0);
	
	GPIOE -> AFSEL |= (1<<1);                   //GPIOE Setup for ADC
	GPIOE -> DEN &= ~(1<<1);										//clear direction and digitalEN registers
	GPIOE -> DIR &= ~(1<<1);												
	GPIOE -> AMSEL |= (1<<0);										//enable analog
	
																							//Sequencer setup	
	ADC0 -> PC |= 0x3;													//max sampling rate	
	ADC0 -> ACTSS &= ~(1<<3);										//disable sampler 3	
																							//config sequencer prio
	ADC0 -> EMUX |= 0x00;												//sofware trigger mode										
	ADC0 -> SSMUX3 |=	0x1;											//config adc input channel
	ADC0 -> SSCTL3 &= ~0x2;
	ADC0 -> SSCTL3 |= 0x4;											//set flags on sample capture
	
	ADC0 -> IM |= 0x1;													//enable interrupts					
	
	NVIC -> ISER[0] |= (1<<17);									//NVIC Setup
	NVIC -> IP[17] |= 0x0;											//set low prio for interrupts	

	ADC0 -> ACTSS |= (1<<3);										//Enable Sample Sequencer



}		
