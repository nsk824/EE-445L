/*
	Author Faniel Ghirmay
	An implimentation of helpful fixed point routines
*/

#include <stdint.h>
#include "fixed.h"
#include "ST7735.h"
#define BLACK 0

int8_t check_range();
void error_handler();
void error_handler2();
int32_t reciprocal(int32_t n);

/****************ST7735_sDecOut3***************
 converts fixed point number to LCD
 format signed 32-bit with resolution 0.001
 range -9.999 to +9.999
 Inputs:  signed 32-bit integer part of fixed-point number
 Outputs: none
 send exactly 6 characters to the LCD 
Parameter LCD display
 12345    " *.***"
  2345    " 2.345"  
 -8100    "-8.100"
  -102    "-0.102" 
    31    " 0.031" 
-12345    " *.***"
 */ 

void ST7735_sDecOut3(int32_t n){
		int8_t fixed_number[6];
		int32_t divisor = 1000;
		int i;
		int negative = 0;
	
		if(check_range()){
				error_handler();
				return;
		}
		if(n < 0){
			negative = 1;
			n = -n;
		}
		for(i = 0; i < 6; i++){
			if(i == 2){
				fixed_number[i] = '.';
			}else if(i == 0 && negative){
				fixed_number[i] = '-';
			}else if(i == 0 && !negative){
				fixed_number[i] = ' ';
			}else{
				int num = n/divisor;
				fixed_number[i] = (int8_t)(num + 48);
				n = n - (num * divisor);
				divisor = divisor / 10;
			}
		}
		ST7735_OutString((char*) fixed_number);  // display final result
}

/**************ST7735_uBinOut8***************
 unsigned 32-bit binary fixed-point with a resolution of 1/256. 
 The full-scale range is from 0 to 999.99. 
 If the integer part is larger than 256000, it signifies an error. 
 The ST7735_uBinOut8 function takes an unsigned 32-bit integer part 
 of the binary fixed-point number and outputs the fixed-point value on the LCD
 Inputs:  unsigned 32-bit integer part of binary fixed-point number
 Outputs: none
 send exactly 6 characters to the LCD 
Parameter LCD display
     0	  "  0.00"
     2	  "  0.01"
    64	  "  0.25"
   100	  "  0.39"
   500	  "  1.95"
   512	  "  2.00"
  5000	  " 19.53"
 30000	  "117.19"
255997	  "999.99"
256000	  "***.**"
*/

void ST7735_uBinOut8(uint32_t n){
		int8_t fixed_number[7];
		int32_t range = 8;
		int32_t temp;
		int8_t prev = 0;
		int32_t i = 0;
		int32_t hundred = 100;
		if(n >= 256000){
				error_handler2();
				return;
		}
		int32_t val = n >> range;
	  while(hundred != 0){
				temp = val / hundred;
			  if(temp != 0){
					prev = 1;
				}
				if(temp == 0 && !prev && i != 2){
					fixed_number[i] = ' ';
				}else{
					fixed_number[i] = temp + 48;
				}
				
				if(val != 0){
					val -= (hundred * temp);
				}
				
				hundred = hundred / 10;
				
				i += 1;
		}
		fixed_number[i] = '.';
		n = n - val; // the remaining bits for fraction
		int32_t frac = 500;
		int32_t mask = 0x80;
		int32_t frac_sum = 0;
		int k = 0;
		while(k < range){
			if(n & mask){
				frac_sum += frac;
			}
			frac /= 2;
			mask = mask >> 1;
			k+=1;
		}
		hundred = 100;
		temp = 0;
		i += 1;
		 while(hundred != 1){
				temp = frac_sum / hundred;
				fixed_number[i] = temp + 48;
				
				if(frac_sum != 0){
					frac_sum -= (hundred * temp);
				}
				hundred = hundred / 10;
				i += 1;
		}
		fixed_number[i+1]=0;
		ST7735_OutString((uint8_t*) fixed_number);  // display final result
}

/**************ST7735_XYplotInit***************
 Specify the X and Y axes for an x-y scatter plot
 Draw the title and clear the plot area
 Inputs:  title  ASCII string to label the plot, null-termination
          minX   smallest X data value allowed, resolution= 0.001
          maxX   largest X data value allowed, resolution= 0.001
          minY   smallest Y data value allowed, resolution= 0.001
          maxY   largest Y data value allowed, resolution= 0.001
 Outputs: none
 assumes minX < maxX, and miny < maxY
*/

int32_t x1, x2, y1, y2, domain, range;

void ST7735_XYplotInit(char *title, int32_t minX, int32_t maxX, int32_t minY, int32_t maxY){
	ST7735_FillScreen(BLACK);  
  ST7735_SetCursor(0,0);
	//ST7735_OutString("\n");
	ST7735_OutString((uint8_t*)title);
	ST7735_PlotClear(minY, maxY);
	x1 = minX;
	x2 = maxX;
	y1 = minY;
	y2 = maxY;
	domain = x2 - x1;
	range = y2 - y1;
}

/**************ST7735_XYplot***************
 Plot an array of (x,y) data
 Inputs:  num    number of data points in the two arrays
          bufX   array of 32-bit fixed-point data, resolution= 0.001
          bufY   array of 32-bit fixed-point data, resolution= 0.001
 Outputs: none
 assumes ST7735_XYplotInit has been previously called
 neglect any points outside the minX maxY minY maxY bounds
*/

void ST7735_XYplot(uint32_t num, int32_t bufX[], int32_t bufY[]){
	uint32_t j;
	int16_t x;
	int16_t y;
	
	for(j=0;j<num;j++){
		if(bufX[j] < x1 || bufX[j] > x2  || bufY[j] < y1 || bufY[j] > y2){
			continue;
		}
	  	x = 127-(127*(x2 - bufX[j])/domain);

  		y = 32+(127*(y2-bufY[j]))/range;
		
			if(x<0)x = 0;
			if(x>127)x=127;
			if(y<32) y = 32;
      if(y>159) y = 159;
			ST7735_DrawFastVLine(x, y, 159-y, ST7735_BLUE);
		/*
		
      ST7735_DrawPixel(x,   y, ST7735_BLUE);
			ST7735_DrawPixel(x+1, y,   ST7735_BLUE);
			ST7735_DrawPixel(x,   y+1, ST7735_BLUE);
			ST7735_DrawPixel(x+1, y+1, ST7735_BLUE);
		*/
  }  
	
}

/*
	Helper function to check is the number is with in our range of representable numbers
*/

int8_t check_range(int32_t n){
		return ((n/1000) > 9) || ((n/1000) < -9);
}


/*
	Helper function to output error message to screen
*/
void error_handler(){
		int8_t fixed_val[6];
		int8_t error_sign = 42;
		int i;
		for(i = 0; i < 6; i++){
			fixed_val[i] = error_sign;
			if(i == 2){
				fixed_val[i] = 46;
			}else if(i == 0){
				fixed_val[i] = 32;
			}
		}
		ST7735_OutString((char*)fixed_val);
}

void error_handler2(){
		int8_t fixed_val[6];
		int8_t error_sign = 42;
		int i;
		for(i = 0; i < 6; i++){
			fixed_val[i] = error_sign;
			if(i == 3){
				fixed_val[i] = 46;
			}
		}
		ST7735_OutString((char*)fixed_val);
}

int32_t reciprocal(int32_t n){
	int32_t numerator = 1;
	
	int32_t val = numerator / n ;
	if(val == 0){
		numerator *= 10;
	}

	return 0;
}
