// SignallerK 2019 -v 1.4.0
//added back_up battery control
//corrected small bugs and refactored code
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "bits_macros.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include "Led_PWM_256.h"
#include "morning_clock.h"
#include <util/atomic.h>

//time structure
typedef struct{
	unsigned char second;
	unsigned char minute;
	unsigned char hour;
	unsigned char week_day;
	
} time;

time t;



//working variables
uint8_t digits[4];
volatile uint8_t current_digit;

//set alarm to on mode
unsigned char dawn_on = 1;
// alarm variables
volatile uint8_t dawn_counter =0;
uint8_t dawn_step; //sec
uint8_t PWM_SEC_Counter=0;

//set indication and brightness
uint8_t indication_counter=indication_period;//sec
volatile uint8_t brightness_counter=BRIGHTNESS_INIT;
uint8_t update_display_flag=1;
//buttons variables
uint8_t m_pressed = 0, h_pressed=0; //button is pressed
uint16_t button_delay=0;           //button delay counter ()


void BIN2BCD(uint8_t *buffer, unsigned int n)
{
	buffer[0]=0;
	while(n>=10) {buffer[0]++;n-=10;}
	buffer[1]=n;
}

void SPI_MasterInit(void)
{
	
	/* Enable SPI, Master, set clock rate fck/16 */
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}
void SPI_MasterTransmit(char cData)
{
	/* Start transmission */
	SPDR = cData;
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)))
	;
}




int main(void)
{
	// wait for quartz stabilization
	#ifdef NDEBUG
	_delay_ms(500);
	#endif

	// Input/Output Ports initialization
	// Port B initialization

	// Function: Bit7=In Bit6=In Bit5=Out Bit4=In Bit3=Out Bit2=OUT Bit1=Out Bit0=In
	DDRB=(0<<DDB7) | (0<<DDB6) | (1<<DDB5) | (0<<DDB4) | (1<<DDB3) | (1<<DDB2) | (1<<DDB1) | (0<<DDB0);

	// State: Bit7=T Bit6=T Bit5=0 Bit4=T Bit3=0 Bit2=1 Bit1=0 Bit0=T
	PORTB=(0<<PORTB7) | (0<<PORTB6) | (0<<PORTB5) | (0<<PORTB4) | (0<<PORTB3) | (1<<PORTB2) | (0<<PORTB1) | (0<<PORTB0);

	// Ports initialization
	DRV_PORT=0x00;
	DRV_DDR=DRV_CONN;

	BUT_PORT|= 0b11100;         //buttons set
	BUT_DDR=0;
	//battery port
	BAT_PORT&=~(1<<BAT_CONN);
	BAT_DDR &=~(1<<BAT_CONN);


	SPI_MasterInit();
	
	// External Interrupt(s) initialization
	// INT0: On
	// INT0 Mode: Any change
	// INT1: On
	// INT1 Mode: Any change
	GICR|=(1<<INT1) | (1<<INT0);
	MCUCR=(0<<ISC11) | (1<<ISC10) | (0<<ISC01) | (1<<ISC00);
	GIFR=(1<<INTF1) | (1<<INTF0);

	// Timer/Counter 0 initialization - indication counter
	// Clock source: System Clock
	// Clock value: 125,000 kHz
	// Timer Period: 2,048 ms
	TCCR0=(0<<CS02) | (1<<CS01) | (1<<CS00);
	TCNT0=0x00;

	// Timer/Counter 1 initialization -pwm counter
	// Clock source: System Clock
	// Clock value: 500,000 kHz
	// Mode: Fast PWM top=0x00FF
	// OC1A output: Non-Inverted PWM
	// Timer Period: 0,512 ms

	TCCR1A=(1<<COM1A1) | (0<<COM1A0) | (0<<COM1B1) | (0<<COM1B0) | (0<<WGM11) | (1<<WGM10);//0x81
	TCCR1B=(0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (1<<WGM12) | (0<<CS12) | (1<<CS11) | (0<<CS10);//0x0A
	TCNT1=0;// turn off timer
	OCR1A=0;
	TCCR1A=0x00; //disconnect timer;

	//###########################################	'
	//Prepare to start
	// turn off timer 2 interrupts
	
	TIMSK &= ~((1<<TOIE2) | (1<<OCIE2));

	// Timer 2 to async mode routine (clock quartz)
	ASSR |= (1<<AS2);

	TCNT2 = 0x00;
	TCCR2 = 0x05; //timer prescaler ratio 128.
	OCR2  = 0x00;

	// waiting for timer readiness.
	while (ASSR & ((1<<TCN2UB) | (1<<OCR2UB) | (1<<TCR2UB)));

	// Timer 2 interrupts on.
	TIMSK |= (1<<TOIE2);
	// Timer 0 interrupts on
	TIMSK |= (1<<TOIE0);
	
	// turn on global params
	menu = Clock;
	t.week_day=1;
	//calculate dawn step
	dawn_step = ((((uint16_t)eeprom_read_byte(&dawn_period))*60)+PWM_TOP/2)/PWM_TOP;
	
	//small trick for simulation, if eeprom wasn't downloaded
	#ifdef DEBUG
	eeprom_write_byte(&brightness,1);
	
	#endif
	//sleep mode
	set_sleep_mode(SLEEP_MODE_PWR_SAVE);
	// global interrupts on
	sei();

	while(1)
	{
		//Prepare indication data
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			if (update_display_flag)
			{
				switch (menu) {
					case Clock:
					BIN2BCD(digits, t.hour);
					BIN2BCD(digits+2, t.minute);
					break;
					
					case Alarm:
					BIN2BCD(digits, eeprom_read_byte(&dawn_hour));
					BIN2BCD(digits+2, eeprom_read_byte(&dawn_minute));
					break;
					case  DAY_SET:
					digits[0]=seg_to_d;//d
					digits[1]=seg_to_S;//S
					BIN2BCD(digits+2,t.week_day );
					digits[2]=seg_to_void;//void
					break;
					case Al_Mon:
					case Al_Tue:
					case Al_Wed:
					case Al_Thu:
					case Al_Fri:
					case Al_Sut:
					case Al_Sun:
					digits[0]=seg_to_d;//d
					digits[1]=seg_to_A;//A
					BIN2BCD(digits+2,menu-DAY_SET);
					digits[2]=seg_to_void;//void
					break;
					case Bright:
					digits[0]=seg_to_b;//b
					digits[1]=seg_to_r;//r
					BIN2BCD(digits+2,eeprom_read_byte(&brightness));
					digits[2]=seg_to_void;//void
					break;
					case Indication:
					digits[0]=seg_to_I;//I
					digits[1]=seg_to_n;//n
					BIN2BCD(digits+2,eeprom_read_byte(&indication_flag));
					digits[2]=seg_to_void;//void
					break;
					case Dawn_Time:
					digits[0]=seg_to_d;//d
					digits[1]=seg_to_P;//P
					BIN2BCD(digits+2,eeprom_read_byte(&dawn_period));
					break;
					
					default:
					menu=Clock;
				}
				update_display_flag=0;
			}
		}
		
		while(BitIsClear(BAT_PIN, BAT_CONN))
		{
			
			DRV_PORT|=0x0F;
			// Timer 0 interrupts off
			TIMSK &= ~(1<<TOIE0);
			
			//if light is on - turnoff
			if (dawn_counter)
			{
				TCNT1=0;// turn off timer
				OCR1A=0;
				TCCR1A=0x00; //disconnect timer;
				dawn_counter=0;//clear counter
				PWM_SEC_Counter=0;//clear counter
			}
			/*sleep_enable();
			sleep_cpu();
			sleep_disable();*/
			sleep_mode();
			if (BitIsSet(BAT_PIN, BAT_CONN))
			{
				// Timer 0 interrupts on
				TIMSK |= (1<<TOIE0);
			}
		}
		
	}

	return 0;
}

//###########################################
//Clock interrupt every 1 sec
ISR(TIMER2_OVF_vect) //overflow interrupt vector
{
	update_display_flag=1;
	//indication time check
	if(indication_counter)
	{
		indication_counter--;
		if (!indication_counter)
		{
			menu=Clock;
			//if constant indication is on,then reset counter to default value
			if(eeprom_read_byte(&indication_flag))
			indication_counter=indication_period;
		}
	}

	//PWM alarm routine
	
	if(dawn_counter)
	{
		if(++PWM_SEC_Counter==dawn_step)
		{
			PWM_SEC_Counter=0;
			
			dawn_counter--;
			OCR1A=pgm_read_byte(&cie[(uint8_t)(~dawn_counter)]);
			
			if (!dawn_counter)
			{
				TCNT1=0;// turn off timer
				OCR1A=0;
				TCCR1A=0x00; //disconnect timer;
			}
		}
	}
	
	
	//This code from avr appnoute count time
	
	if (++t.second==60)     //keep track of hours minutes
	{
		t.second=0;
		
		//Clear menu and button
		button_delay=0;
		
		
		if (++t.minute==60)
		{
			t.minute=0;

			if (++t.hour==24)
			{
				t.hour=0;
				if (++t.week_day==8)
				{
					t.week_day=1;
				}
			}
		}
		
		// dawn checking function
		if (dawn_on&&eeprom_read_byte(&week_day_al[t.week_day-1]))
		{
			
			uint8_t temp_dw_hour=eeprom_read_byte(&dawn_hour);
			uint8_t temp_dw_minute=eeprom_read_byte(&dawn_minute);
			//calculate actual start as alarm - dawn_period
			if (temp_dw_minute>=eeprom_read_byte(&dawn_period))
			{
				temp_dw_minute=temp_dw_minute-eeprom_read_byte(&dawn_period);
			}
			else
			{
				if (temp_dw_hour==00)
				{
					temp_dw_hour=24;
				}
				temp_dw_hour--;
				temp_dw_minute=60-eeprom_read_byte(&dawn_period)+temp_dw_minute;
			}
			//check if time to start
			if (temp_dw_hour==t.hour)
			{
				if (temp_dw_minute==t.minute)
				{
					dawn_counter=PWM_TOP;
					OCR1A=0;//
					TCNT1=0;
					TCCR1A=0x81; //turn on timer
				}
			}
		}
	}
}
//##############################
//Buttons interrupt

ISR(INT0_vect) //hour button
{
	h_pressed=(BitIsClear(PIND, 2));
	button_delay=0;
	
}

ISR(INT1_vect) //minute button
{
	m_pressed=(BitIsClear(PIND, 3));
	button_delay=0;
}

//##############################
//Main indication routine

ISR(TIMER0_OVF_vect)
{
	DRV_PORT|=0x0F;     //turn off indicator
	
	volatile uint8_t temp=0x00;
	
	/*Prepare dots to on			*/
	if (current_digit==1)   //if dots time
	{
		if (BitIsSet(t.second,0)) //dot every second
		{
			temp=CLEAR_DOT;
		}
		else
		{
			temp=SET_DOT;
		}
	}
	if (current_digit==3)   //if dawn on
	{
		if ((dawn_on==1&& menu==Clock)||((menu>=Al_Mon && menu<=Al_Sun) && eeprom_read_byte(&week_day_al[menu-Al_Mon]))) //alarm indication on
		{
			temp=SET_DOT;//set dot indicator for alarm
		}
		else
		{
			temp=CLEAR_DOT;//clear dot indicator for alarm
		}
	}
	// if indication is on prepare digit value
	if(indication_counter)
	{
		if (current_digit==0)   //if dots time
		{
			if (menu==Alarm) //
			{
				temp=SET_DOT;
			}
			else
			{
				temp=CLEAR_DOT;
			}
		}
		
		temp|=segment[digits[current_digit]];
	}
	brightness_counter -= eeprom_read_byte(&brightness);// check the brightness
	// turn on digit, if brightness 0
	if(!brightness_counter)
	{
		brightness_counter=2;
		SPI_MasterTransmit(~temp); //send digit via spi
		
		ClearBit(DRV_PORT, 3-current_digit);    //turn on current digit
		
		
		if (++current_digit==4)         //if all digits passed
		{
			current_digit=0;
		}
	}
	
	//Interrupt Button debounce
	if (button_delay==BUTTON_DEBOUNCING&&(h_pressed||m_pressed))
	
	{
		indication_counter=indication_period; //update counter
		update_display_flag=1;
		if (dawn_counter)  //if alarm is on, turn off it
		{
			dawn_counter=0;
			TCNT1=0;
			OCR1A=0;
			TCCR1A=0x00;; //disconnect timer
		}
		
	}
	//Button check main cycle
	if (++button_delay==BUTTON_DELAY_CONST) // around 0.4 sec for fixed value
	{
		button_delay=0;
		update_display_flag=1;
		if (menu_pressed&&indication_counter)// menu pressed
		{
			if(++menu== MENU_END)
			{
				menu=0;
				indication_counter=indication_period;
			}
		}
		
		if (h_pressed&&menu_unpressed&&!(m_pressed))  //hour button pressed
		{
			if (menu==Clock)
			{
				if (++t.hour==24)// corrected hours
				{
					t.hour=0;
				}
			}
			else if (menu==Alarm)
			{
				eeprom_write_byte(&dawn_hour, eeprom_read_byte(&dawn_hour)+1);
				
				if (eeprom_read_byte(&dawn_hour)==24)//correct alarm hour
				{
					eeprom_write_byte(&dawn_hour,0);
				}
			}
		}
		if (m_pressed&&menu_unpressed&&!(h_pressed)) //if minute button pressed
		{
			//updates menu item's value
			switch (menu)
			{
				case Clock: //correct minutes
				if (++t.minute==60)
				{
					t.minute=0;
				}
				break;
				case Alarm: //correct alarm minute
				eeprom_write_byte(&dawn_minute, eeprom_read_byte(&dawn_minute)+1);
				if (eeprom_read_byte(&dawn_minute)==60)
				{
					eeprom_write_byte(&dawn_minute,0);
				}
				break;
				//set day
				case DAY_SET:
				if (++t.week_day==8)
				{
					t.week_day=1;
				}
				break;
				//set if day alarm is on
				case Al_Mon:
				case Al_Tue:
				case Al_Wed:
				case Al_Thu:
				case Al_Fri:
				case Al_Sut:
				case Al_Sun:
				if (eeprom_read_byte(&week_day_al[menu-Al_Mon]))//set day alarms
				{
					eeprom_write_byte(&week_day_al[menu-Al_Mon],0);
				}
				else
				{
					eeprom_write_byte(&week_day_al[menu-Al_Mon],1);
				}
				break;
				//correct brightness
				case Bright:
				
				ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
				{
					if (eeprom_read_byte(&brightness)+1==3)
					{
						eeprom_write_byte(&brightness,1);
					}
					else
					eeprom_write_byte(&brightness, eeprom_read_byte(&brightness)+1);
					
					brightness_counter=2;
				}
				break;
				//set indication 1- constant, 0- 60 sec
				case Indication:
				eeprom_write_byte(&indication_flag, eeprom_read_byte(&indication_flag)+1);
				if (eeprom_read_byte(&indication_flag)==2)//set indication time
				{
					eeprom_write_byte(&indication_flag,0);
					
				}
				break;
				//set dawn period
				case Dawn_Time:
				eeprom_write_byte(&dawn_period, eeprom_read_byte(&dawn_period)+5);
				if (eeprom_read_byte(&dawn_period)>30)//correct brightness
				{
					eeprom_write_byte(&dawn_period, 15);
					
				}
				dawn_step = ((((uint16_t)eeprom_read_byte(&dawn_period))*60)+PWM_TOP/2)/PWM_TOP;
				break;
				default:
				break;
			}
			
		}
		
		
		if (h_pressed&&m_pressed&&menu_unpressed) //alarm turn off/on (2 buttons)
		{
			if(menu==Clock||menu==Alarm)
			{
				if(dawn_on)
				{
					dawn_on=0;
				}
				else
				{
					dawn_on=1;
				}
			}
			
		}
	}
	
}