//############################################
//This file stores main parameters and variables 
//#############################################
#ifndef MORNING_CLOCK_H
#define MORNING_CLOCK_H

#include <avr/io.h>
#include "bits_macros.h"

FUSES =
{
	.low = 0xA4,		//Select cpu clk: Internal calibrated RC-oscillator @ 8MHz
	.high = 0xC0,		
	//.extended = 0xE3,	//Default settings
};

//DEFINES
//Ports defines
//Port C indication port
#define DRV_PORT PORTC
#define DRV_DDR DDRC
#define DRV_CONN 0b00001111
//Port D buttons port
#define BUT_PORT PORTD
#define BUT_DDR DDRD
//Port C battery port
#define BAT_PORT PORTC
#define BAT_DDR DDRC
#define BAT_PIN PINC
#define BAT_CONN 5

//Buttons macro and defines

#define menu_pressed BitIsClear(PIND, 4)
#define menu_unpressed BitIsSet(PIND, 4)
#define BUTTON_DELAY_CONST 300 //constant, time between value change (~0.5 sec)
#define BUTTON_DEBOUNCING 3 // debounce const ~10ms

//Basic parameters defines
#define BRIGHTNESS_INIT 1 //start value of brightness (2 max, 1 dim)
#define indication_period 60//sec  if indication flag 1, other cinst
//Other
#define	PWM_TOP 0xFF	//PWM_TOP don't change
#define CLEAR_DOT 0b00000000
#define SET_DOT 0b10000000

// EEPROM Variables
unsigned char EEMEM dawn_hour =6; //alarm hour
unsigned char EEMEM dawn_minute =50;//alarm minute
unsigned char EEMEM brightness = BRIGHTNESS_INIT; 
unsigned char EEMEM indication_flag = 1;// 1 is constant indication, 0 -one 1 minute
unsigned char EEMEM dawn_period = 25; //dawn period from 15 to 30 step 5
unsigned char EEMEM week_day_al[7]={1,1,1,1,1,0,0}; //week days alarm set



// 7 segment translation array
enum seg_nick_name {
	seg_to_A=10, 
	seg_to_d=11,
	seg_to_S=12,
	seg_to_b=13,
	seg_to_r=14,
	seg_to_I=15,
	seg_to_n=16,
	seg_to_P=17,
	seg_to_void=18
	};
uint8_t segment[] =
//pgfedcba+some symbols
{
	0b00111111,     //0
	0b00000110,     //1
	0b01011011,     //2
	0b01001111,     //3
	0b01100110,     //4
	0b01101101,     //5
	0b01111101,     //6
	0b00000111,     //7
	0b01111111,     //8
	0b01101111,     //9
	0b01110111,		//A
	0b01011110,		//d
	0b01101101,		//S
	0b01111100,		//b
	0b01010000,		//r
	0b00110000,		//I
	0b01010100,		//n
	0b01110011,		//P
	0b00000000		//void _
};

// menu types enum
enum menu_item
{
	Clock,
	Alarm,
	DAY_SET,
	Al_Mon,
	Al_Tue,
	Al_Wed,
	Al_Thu,
	Al_Fri,
	Al_Sut,
	Al_Sun,
	Bright,
	Indication,
	Dawn_Time,
	//TBD correction
	MENU_END

} menu;

#endif