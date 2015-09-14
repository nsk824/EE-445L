// ADCTestMain.c
// Runs on LM4F120/TM4C123
// This program periodically samples ADC channel 0 and stores the
// result to a global variable that can be accessed with the JTAG
// debugger and viewed with the variable watch feature.
// Daniel Valvano
// May 18, 2014

/* This example accompanies the book
   "Embedded Systems: Real Time Interfacing to Arm Cortex M Microcontrollers",
   ISBN: 978-1463590154, Jonathan Valvano, copyright (c) 2014

 Copyright 2014 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// center of X-ohm potentiometer connected to PE3/AIN0
// bottom of X-ohm potentiometer connected to ground
// top of X-ohm potentiometer connected to +3.3V through X/10-ohm ohm resistor
#include <stdint.h>
#include "ADCSWTrigger.h"
#include "inc/tm4c123gh6pm.h"
#include "PLL.h"
#include "ST7735.h"
#include "fixed.h"

#define GPIO_PORTF2             (*((volatile uint32_t *)0x40025010))

void DisableInterrupts(void); // Disable interrupts
void EnableInterrupts(void);  // Enable interrupts
long StartCritical (void);    // previous I bit, disable interrupts
void EndCritical(long sr);    // restore I bit to previous value
void WaitForInterrupt(void);  // low power mode
uint32_t Calc_Jitter(); // calculate jitter
void Calc_PMF(); // calculate freq
void User_Func();
volatile uint32_t ADCvalue;
volatile uint32_t Elapsed_Time;
volatile uint32_t Current_Time;
volatile uint32_t Prev_Time;
volatile uint32_t ADC_Data[1000];
volatile uint32_t ADC_Elapsed_Time[1000];
volatile uint32_t ADC_Freq[1000];
volatile uint32_t Buffer_Counter = 0;

//void(*task)(void)

void Timer1_Init(void){
  volatile uint32_t delay;
  SYSCTL_RCGCTIMER_R |= 0x02;   // 0) activate TIMER1
  delay = SYSCTL_RCGCTIMER_R;   // allow time to finish activating
  TIMER1_CTL_R = 0x00000000;    // 1) disable TIMER1A during setup
  TIMER1_CFG_R = 0x00000000;    // 2) configure for 32-bit mode
  TIMER1_TAMR_R = 0x00000002;   // 3) configure for periodic mode, down-count 
  TIMER1_TAILR_R = 0xFFFFFFFF;  // 4) reload value
  TIMER1_TAPR_R = 0;            // 5) bus clock resolution
  TIMER1_CTL_R = 0x00000001;    // 10) enable TIMER1A
}



// This debug function initializes Timer0A to request interrupts
// at a 10 Hz frequency.  It is similar to FreqMeasure.c.
void Timer0A_Init10HzInt(void){
  volatile uint32_t delay;
  DisableInterrupts();
  // **** general initialization ****
  SYSCTL_RCGC1_R |= SYSCTL_RCGC1_TIMER0;// activate timer0
  delay = SYSCTL_RCGC1_R;          // allow time to finish activating
  TIMER0_CTL_R &= ~TIMER_CTL_TAEN; // disable timer0A during setup
  TIMER0_CFG_R = TIMER_CFG_16_BIT; // configure for 16-bit timer mode
  // **** timer0A initialization ****
                                   // configure for periodic mode
  TIMER0_TAMR_R = TIMER_TAMR_TAMR_PERIOD;
  TIMER0_TAPR_R = 249;             // prescale value for 10us
  TIMER0_TAILR_R = 999;           // start value for 10 Hz interrupts
  TIMER0_IMR_R |= TIMER_IMR_TATOIM;// enable timeout (rollover) interrupt
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;// clear timer0A timeout flag
  TIMER0_CTL_R |= TIMER_CTL_TAEN;  // enable timer0A 16-b, periodic, interrupts
  // **** interrupt initialization ****
                                   // Timer0A=priority 2
  NVIC_PRI4_R = (NVIC_PRI4_R&0x00FFFFFF)|0x40000000; // top 3 bits
  NVIC_EN0_R = NVIC_EN0_INT19;     // enable interrupt 19 in NVIC
}
void Timer0A_Handler(void){
  TIMER0_ICR_R = TIMER_ICR_TATOCINT;    // acknowledge timer0A timeout
	Current_Time = TIMER1_TAILR_R;
  GPIO_PORTF2 = 0x04;                   // profile
	//Elapsed_Time = TIMER1_TAILR_R;
  ADCvalue = ADC0_InSeq3();
	Elapsed_Time = Prev_Time - Current_Time;
	//Elapsed_Time = Elapsed_Time - TIMER1_TAILR_R;
	Prev_Time = Current_Time;
  GPIO_PORTF2 = 0x00;
	if(Buffer_Counter <= 1000){
		if(Buffer_Counter == 1000){
				Buffer_Counter += 1;
				return;
		}
		ADC_Data[Buffer_Counter] = ADCvalue;
		ADC_Elapsed_Time[Buffer_Counter] = Elapsed_Time;
		Buffer_Counter += 1;
	}
}
int main(void){
  PLL_Init();                           // 25 MHz
	

	
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOF; // activate port F
   ADC0_InitSWTriggerSeq3(0);            // allow time to finish activating
 // ADC0_InitAllTriggerSeq3(0);           // allow time to finish activating
	Timer0A_Init10HzInt();                // set up Timer0A for 10 Hz interrupts
  Timer1_Init();												// Intitalize timer1 count down
	
  GPIO_PORTF_DIR_R |= 0x04;             // make PF2 out (built-in LED)
  GPIO_PORTF_AFSEL_R &= ~0x04;          // disable alt funct on PF2
  GPIO_PORTF_DEN_R |= 0x04;             // enable digital I/O on PF2
                                        // configure PF2 as GPIO
  GPIO_PORTF_PCTL_R = (GPIO_PORTF_PCTL_R&0xFFFFF0FF)+0x00000000;
  GPIO_PORTF_AMSEL_R = 0;               // disable analog functionality on PF
  GPIO_PORTF2 = 0;                      // turn off LED
	 EnableInterrupts();
   ST7735_InitR(INITR_REDTAB);
	ST7735_FillScreen(0);  // set screen to black
	ST7735_SetCursor(0,0);
  ST7735_XYplotInit("Lab 2 PMF Averaging \n", 0, 4096, 0, 50);
	ST7735_OutString("1 point \n");
  while(1){
     WaitForInterrupt();
		 
//    GPIO_PORTF2 = 0x04;                 // profile
//    ADCvalue = ADC0_InSeq3();
//    GPIO_PORTF2 = 0x00;
		
		
		if(Buffer_Counter == 1000){
		    break;
		}
  }
		uint32_t Jitter;
	//	DisableInterrupts();
		//	TIMER1_CTL_R = 0x00000000;    // 10) enable TIMER1A
			GPIO_PORTF2 = 0x04;                   // profile
			
			
			Jitter = Calc_Jitter();  
			 uint32_t n = ADCvalue;
 	  //	 ST7735_OutUDec(n);
			Calc_PMF();  // will populate the frequency table ADC_Data = x-axis ADC_Freq = y-axis
			// Next line is call to the plot point function we created in lab1
		  //	ST7735_XYplotInit("PMF", 0, 4096, 0, 50);
		  //  ST7735_XYplot(1000, ADC_Data, ADC_Freq);
			int j = 0;
			uint32_t x = 0;
			uint32_t y = 0;
	    int i = 0;
			//ST7735_PlotClear(32, 159);
		//	for(j = 0; j < 1000; j+=1){
				ST7735_XYplotInit("Lab 2 PMF", 0, 4095, 0, 1000);
				ST7735_XYplot(1000, ADC_Data, ADC_Freq);
			 	// ST7735_PlotBar(ADC_Freq[j]);
				// ST7735_PlotBar(ADC_Freq[j]);
				 //ST7735_PlotBar(30);
				//ST7735_PlotNext();
				
			//	  y = 32+(127*(400-ADC_Freq[j]))/400;
		  //		x = 127-(127*(4095 - ADC_Data[j])/4095);
			//	  if(x<0)x = 0;
			//		if(x>127)x=127;
			//		if(y<32) y = 32;
			//	ST7735_PlotBar(y);
			//	if(y>159) y = 159;
			 // 	if(x > i){
				//		ST7735_PlotNext();
				 //	  i += 1;
				 // }
				
				/*
				if(j < 14){
				ST7735_OutUDec(ADC_Data[j]);
				ST7735_OutString(" ");
				ST7735_OutUDec(ADC_Freq[j]);
					ST7735_OutString("\n");
				}
				*/
//			}
			GPIO_PORTF2 = 0x00;
		//	EnableInterrupts();
}
uint32_t Calc_Jitter(){
	uint32_t i = 0;
	uint32_t small_diff = ADC_Data[0];
	uint32_t large_diff = ADC_Data[0];
	for(i = 0; i < 1000; i+=1){
		if(ADC_Data[i] < small_diff){
			small_diff = ADC_Data[i];
		}
		if(ADC_Data[i] > large_diff){
			large_diff = ADC_Data[i];
			
		}
	}
	return large_diff - small_diff;
}

/*
For each of the adc value this function calculates the frequency

*/


void Calc_PMF(){
	uint32_t i = 0;
	
	for(i=0; i < 1000; i+=1){
			uint32_t j = 0;
			uint32_t freq = 0;
			for(j=0; j<1000; j+=1){
				if(ADC_Data[i] == ADC_Data[j]){
						freq +=1;
				}
			}
			ADC_Freq[i] = freq;
	}

}
