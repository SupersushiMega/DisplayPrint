/* Tiny TFT Graphics Library - see http://www.technoblogy.com/show?L6I

   David Johnson-Davies - www.technoblogy.com - 13th June 2019
   ATtiny85 @ 8 MHz (internal oscillator; BOD disabled)
   
   CC BY 4.0
   Licensed under a Creative Commons Attribution 4.0 International license: 
   http://creativecommons.org/licenses/by/4.0/
*/

#define F_CPU 8000000UL                 // set the CPU clock
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <math.h>
#include "st7735.h"

#define BACKLIGHT_ON PORTB |= (1<<PB2)
#define BACKLIGHT_OFF PORTB &= ~(1<<PB2)						

#define LED_OFF PORTC &= ~(1<<PC3)
#define LED_ON PORTC |= (1<<PC3)

//Buttons 
#define T1 	!(PIND & (1<<PD6))
#define T2	!(PIND & (1<<PD2))
#define T3	!(PIND & (1<<PD5))

/* some RGB color definitions                                                 */
#define BLACK        0x0000      
#define RED          0x001F      
#define GREEN        0x07E0      
#define YELLOW       0x07FF      
#define BLUE         0xF800      
#define CYAN         0xFFE0      
#define White        0xFFFF     
#define BLUE_LIGHT   0xFD20      
#define TUERKISE     0xAFE5      
#define VIOLET       0xF81F		
#define WHITE		0xFFFF

#define SEK_POS 10,110

#define RELOAD_ENTPRELL 1 

// Pins already defined in st7735.c
extern int const DC;
extern int const MOSI;
extern int const SCK;
extern int const CS;
// Text scale and plot colours defined in st7735.c
extern int fore; 		// foreground colour
extern int back;      	// background colour
extern int scale;     	// Text size


volatile uint8_t ms10,ms100,sec,min, entprell;


char stringbuffer[20]; // buffer to store string 

const uint8_t size = 128;

ISR (TIMER1_COMPA_vect);

volatile uint8_t ISR_zaehler = 0;
volatile uint8_t ms = 0;
volatile uint8_t second = 0;
volatile uint8_t minute = 0;
volatile uint8_t hour = 0;
volatile uint8_t CountUp = 1;
ISR (TIMER0_OVF_vect)
{
	TCNT0 = 0;
	ISR_zaehler++;
	if(ISR_zaehler == 12)
	{
		ms++;
		ISR_zaehler = 0;
		if (ms == 10)
		{
			if (CountUp == 1)
			{
				if (ms == 10)
				{
					second ++;
					ms = 0;
					if (second == 60)
					{
						minute ++;
						second = 0;
						if (minute == 60)
						{
							hour ++;
							minute = 0;
						}
					}
				}
			}
			else
			{
				second --;
				ms = 0;
				if (second == 255)
				{
					minute --;
					second = 59;
					if (minute == 255)
					{
						hour --;
						minute = 59;
					}
				}
			}
		}
	}
}//End of ISR
uint8_t DrawPointer (uint8_t Number, uint8_t SizeOfRotation, uint8_t Radius, int16_t Color, uint8_t TrailWidth, int16_t TrailColor, int8_t DiffToLast);

int main(void)
{
	DDRB |= (1<<DC) | (1<<CS) | (1<<MOSI) |( 1<<SCK); 	// All outputs
	PORTB = (1<<SCK) | (1<<CS) | (1<<DC);          		// clk, dc, and cs high
	DDRB |= (1<<PB2);									//lcd Backlight output
	PORTB |= (1<<CS) | (1<<PB2);                  		// cs high
	DDRC |= (1<<PC3);									//Reset Output
	DDRD |= (1<<PD7);									//Reset Output
	PORTD |= (1<<PD7);	
									//Reset High
	DDRD &= ~((1<<PD6) | (1<<PD2) | (1<<PD5)); 	//Taster 1-3
	PORTD |= ((1<<PD6) | (1<<PD2) | (1<<PD5)); 	//PUllups für Taster einschalten
	
		//Timer 1 Configuration
	OCR1A = 1249;	//OCR1A = 0x3D08;==1sec
	
    TCCR1B |= (1 << WGM12);
    // Mode 4, CTC on OCR1A

    TIMSK1 |= (1 << OCIE1A);
    //Set interrupt on compare match

    TCCR1B |= (1 << CS11) | (1 << CS10);
    // set prescaler to 64 and start the timer

    sei();
    // enable interrupts
    
    ms10=0;
    ms100=0;
    sec=0;
    min=0;
    entprell=0;
	
	BACKLIGHT_ON;
	LED_ON;

	setup();
	
	//Konfiguration Timer Overflow
	//==================================================================
	TCCR0A	= 0x00;
	TCCR0B	= 0x04;
	TIMSK0	|= (1 << TOIE0);
	TIFR0 |= (1 << TOV0);
	sei();
	//==================================================================
	
	enum {START, CHOICE, TIMER, COUNTDOWNSET, COUNTDOWN};
	
	uint8_t state = START;
	
	uint16_t count1;	//Variable used for Counting in for loops
	uint16_t count2;	//Variable used for Counting in for loops
	
	char buffer[20];
	
	uint8_t BoxSize;
	
	uint8_t SetPos = 0;		//Variable used to select which Part of StartTime is being edited
	uint8_t StartTime[] = {0, 0, 0};	//Countdown Start Time (sec, min, hour)
	
	uint8_t LastSec = 0;
	uint8_t LastMin = 0;
	uint8_t LastHour = 0;
	
	while (1)
	{
		switch(state)
		{
			case START:
			{
				MoveTo(0, size);
				for (count1 = 0; count1 != size; count1++)
				{
					for (count2 = 0; count2 != size; count2++)
					{
						fore = Colour(count1* 2, 255 - (count1 + count2), count2 * 2);
						PlotPoint(count1, count2);
					}
				}
				fore = White;
				MoveTo(0,20);
				scale = 3;
				PlotString("Welcome");
				MoveTo(0,10);
				scale = 1;
				PlotString("Push any button to");
				MoveTo(0,0);
				PlotString("continue");
				while(state == START || T1 || T2 || T3)
				{
					if(T1 || T2 || T3)
					{
						state = CHOICE;
					}
				}
				break;
			}
			
			case CHOICE:
			{
				ClearDisplay();
				count2 = 0;
				BoxSize = 30;
				for (count1 = ((size/3) / 4); count1 <= size; count1 += (size/3))
				{
					fore = WHITE;
					switch(count2)
					{
						case 0:
						{
							MoveTo(BoxSize + 2, count1);
							PlotString("Timer");
							fore = BLUE;
							break;
						}
						
						case 1:
						{
							MoveTo(BoxSize + 2, count1);
							PlotString("Countdown");
							fore = GREEN;
							break;
						}
						
						case 2:
						{
							MoveTo(BoxSize + 2, count1);
							PlotString("Back");
							fore = YELLOW;
							break;
						}
					}
					MoveTo(0, count1);
					FillRect(BoxSize, BoxSize);
					count2++;
				}
				
				while(state == CHOICE || T1 || T2 || T3)
				{
					if(T1)
					{
						ClearDisplay();
						state = TIMER;
					}
					
					else if(T2)
					{
						ClearDisplay();
						state = COUNTDOWNSET;
					}
					
					else if(T3)
					{
						state = START;
					}
				}
				break;
			}
			
			case TIMER:
			{
				CountUp = 1;
				second = 0;
				minute = 0;
				hour = 0;
				LastSec = 0;
				LastMin = 0;
				LastHour = 0;
				ClearDisplay();
				while (!T3)
				{
					if (T1)
					{
						second = 0;
						minute = 0;
						hour = 0;
						ClearDisplay();
					}
					fore = WHITE;
					MoveTo(0,0);
					//==============================================================
					
					//Hour Output
					//==============================================================
					if (hour < 10)
					{
						sprintf(buffer, "0%d:", hour);
						PlotString(buffer);
					}
					else
					{
						sprintf(buffer, "%d:", hour);
						PlotString(buffer);
					}
					//==============================================================
					
					//Minute Output
					//==============================================================
					if (minute < 10)
					{
						sprintf(buffer, "0%d:", minute);
						PlotString(buffer);
					}
					else
					{
						sprintf(buffer, "%d:", minute);
						PlotString(buffer);
					}
					//==============================================================
					
					//Second Output
					//==============================================================
					if (second < 10)
					{
						sprintf(buffer, "0%d", second);
						PlotString(buffer);
					}
					else
					{
						sprintf(buffer, "%d", second);
						PlotString(buffer);
					}
					//==============================================================
					
					//Pointer Drawing
					//==============================================================
					LastSec = DrawPointer(second, 60, 40, WHITE, 2, RED, second - LastSec);
					LastMin = DrawPointer(minute, 60, 30, WHITE, 2, GREEN, minute - LastMin);
					LastHour = DrawPointer(hour, 24, 20, WHITE, 2, BLUE, hour - LastHour);
				}
				state = CHOICE;
				break;
			}
			
			case COUNTDOWNSET:
			{
				fore = RED;
				MoveTo(size / 2, size / 2);
				scale = 3;
				PlotString("SET MODE");
				
				fore = WHITE;
				scale = 1;
				MoveTo(0, 0);
				//Hour Output
				//==============================================================
				if (StartTime[2] < 10)
				{
					sprintf(buffer, "0%d:", StartTime[2]);
					PlotString(buffer);
				}
				else
				{
					sprintf(buffer, "%d:", StartTime[2]);
					PlotString(buffer);
				}
				//==============================================================
				
				//Minute Output
				//==============================================================
				if (StartTime[1] < 10)
				{
					sprintf(buffer, "0%d:", StartTime[1]);
					PlotString(buffer);
				}
				else
				{
					sprintf(buffer, "%d:", StartTime[1]);
					PlotString(buffer);
				}
				//==============================================================
				
				//Second Output
				//==============================================================
				if (StartTime[0] < 10)
				{
					sprintf(buffer, "0%d", StartTime[0]);
					PlotString(buffer);
				}
				else
				{
					sprintf(buffer, "%d", StartTime[0]);
					PlotString(buffer);
				}
				//==============================================================
				if (T1)
				{
					StartTime[SetPos]++;
				}
				
				else if (T2)
				{
					SetPos++;
				}
				
				else if (T3)
				{
					StartTime[SetPos]--;
				}
				
				if (SetPos > 2)
				{
					state = COUNTDOWN;
				}
				
				break;
			}
			
			case COUNTDOWN:
			{
				CountUp = 0;
				second = StartTime[0];
				minute = StartTime[1];
				hour = StartTime[2];
				
				LastSec = StartTime[0];
				LastMin = StartTime[1];
				LastHour = StartTime[2];
				
				ClearDisplay();
				while (!T3)
				{
					if (T1)
					{
						state = COUNTDOWNSET;
						SetPos = 0;
						ClearDisplay();
					}
					fore = WHITE;
					MoveTo(0,0);
					//==============================================================
					
					//Hour Output
					//==============================================================
					if (hour < 10)
					{
						sprintf(buffer, "0%d:", hour);
						PlotString(buffer);
					}
					else
					{
						sprintf(buffer, "%d:", hour);
						PlotString(buffer);
					}
					//==============================================================
					
					//Minute Output
					//==============================================================
					if (minute < 10)
					{
						sprintf(buffer, "0%d:", minute);
						PlotString(buffer);
					}
					else
					{
						sprintf(buffer, "%d:", minute);
						PlotString(buffer);
					}
					//==============================================================
					
					//Second Output
					//==============================================================
					if (second < 10)
					{
						sprintf(buffer, "0%d", second);
						PlotString(buffer);
					}
					else
					{
						sprintf(buffer, "%d", second);
						PlotString(buffer);
					}
					//~ //==============================================================
					
					//~ //Pointer Drawing
					//~ //==============================================================
					LastSec = DrawPointer(second, 60, 40, WHITE, 2, RED, second - LastSec);
					LastMin = DrawPointer(minute, 60, 30, WHITE, 2, GREEN, minute - LastMin);
					LastHour = DrawPointer(hour, 24, 20, WHITE, 2, BLUE, hour - LastHour);
				}
				state = CHOICE;
				break;
			}
		}
	}
		
		

	  
	for (;;)
	{

	}//end of for()
}//end of main

uint8_t DrawPointer (uint8_t Number, uint8_t SizeOfRotation, uint8_t Radius, int16_t Color, uint8_t TrailWidth, int16_t TrailColor, int8_t DiffToLast)
{
	float radian;
	MoveTo(size / 2, size / 2);	//Move to the middle of the display
	fore = Color;
	radian = ((Number * (360 / SizeOfRotation)) + 90) * (M_PI / 180);	//calculate
	DrawTo((size / 2) - cos(radian) * Radius, ((size / 2) + sin(radian) * Radius));
	
	if(DiffToLast != 0)
	{	
		radian = (((Number - DiffToLast) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
		MoveTo(size / 2, size / 2);
		fore = BLACK;
		DrawTo((size / 2) - cos(radian) * Radius, ((size / 2) + sin(radian) * Radius));
		
		if (TrailWidth != 0)
		{
			radian = (((Number - 1) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
			int8_t i;
			int8_t j;
			if (Number % SizeOfRotation != 0 || 1)
			{
				for (j = 0; j != SizeOfRotation; j++)
				{
					for (i = 0 - floor(TrailWidth / 2); i != round(TrailWidth / 2); i++)
					{
						if (j <= Number)
						{
							fore = TrailColor;
						}
						else
						{
							fore = BLACK;
						}
						radian = (((j) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
						MoveTo((size / 2) - cos(radian) * (Radius + i), ((size / 2) + sin(radian) * (Radius + i)));
						radian = (((j - 1) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
						DrawTo((size / 2) - cos(radian) * (Radius + i), ((size / 2) + sin(radian) * (Radius + i)));
					}
				}
					//~ radian = (((Number) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
					//~ MoveTo((size / 2) - cos(radian) * (Radius + i), ((size / 2) + sin(radian) * (Radius + i)));
					//~ radian = (((Number - 1) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
					//~ DrawTo((size / 2) - cos(radian) * (Radius + i), ((size / 2) + sin(radian) * (Radius + i)));
			}
		}
			
			//~ else
			//~ {
				//~ fore = BLACK;
				//~ for (Number = 0; Number != SizeOfRotation; Number++)
				//~ {
					//~ for (i = 0 - floor(TrailWidth / 2); i != round(TrailWidth / 2); i++)
					//~ {
						//~ radian = (((Number) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
						//~ MoveTo((size / 2) - cos(radian) * (Radius + i), ((size / 2) + sin(radian) * (Radius + i)));
						//~ radian = (((Number - 1) * round(360 / SizeOfRotation)) + 90) * (M_PI / 180);
						//~ DrawTo((size / 2) - cos(radian) * (Radius + i), ((size / 2) + sin(radian) * (Radius + i)));
					//~ }
				//~ }
			//~ }
	}
	return Number;
}

ISR (TIMER1_COMPA_vect)
{
	
}
