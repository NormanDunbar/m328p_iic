/* 
   Copyright 2018 Alexander Shuping

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 
 * project.c
 * test project for the library
 */

#ifndef F_CPU
	#define F_CPU 8000000UL
#endif

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>
#include <util/twi.h>

#include "iic.h"

//#define SLAVE

#ifdef SLAVE
	//#define MASTER_MODE
	#define ADDRESS 0x6A
	#define REMOTE_ADDRESS 0x69
	#define BITRATE_PRESCALER 0
	#define BITRATE 0
#else
	#define MASTER_MODE
	#define ADDRESS 0x69
	#define REMOTE_ADDRESS 0x6A
	#define BITRATE_PRESCALER 0
	#define BITRATE 0
uint8_t sine_lut[] = {0x80,0x83,0x86,0x89,0x8c,0x8f,0x92,0x95,
0x98,0x9b,0x9e,0xa2,0xa5,0xa7,0xaa,0xad,
0xb0,0xb3,0xb6,0xb9,0xbc,0xbe,0xc1,0xc4,
0xc6,0xc9,0xcb,0xce,0xd0,0xd3,0xd5,0xd7,
0xda,0xdc,0xde,0xe0,0xe2,0xe4,0xe6,0xe8,
0xea,0xeb,0xed,0xee,0xf0,0xf1,0xf3,0xf4,
0xf5,0xf6,0xf8,0xf9,0xfa,0xfa,0xfb,0xfc,
0xfd,0xfd,0xfe,0xfe,0xfe,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xfe,0xfe,0xfe,0xfd,
0xfd,0xfc,0xfb,0xfa,0xfa,0xf9,0xf8,0xf6,
0xf5,0xf4,0xf3,0xf1,0xf0,0xee,0xed,0xeb,
0xea,0xe8,0xe6,0xe4,0xe2,0xe0,0xde,0xdc,
0xda,0xd7,0xd5,0xd3,0xd0,0xce,0xcb,0xc9,
0xc6,0xc4,0xc1,0xbe,0xbc,0xb9,0xb6,0xb3,
0xb0,0xad,0xaa,0xa7,0xa5,0xa2,0x9e,0x9b,
0x98,0x95,0x92,0x8f,0x8c,0x89,0x86,0x83,
0x80,0x7c,0x79,0x76,0x73,0x70,0x6d,0x6a,
0x67,0x64,0x61,0x5d,0x5a,0x58,0x55,0x52,
0x4f,0x4c,0x49,0x46,0x43,0x41,0x3e,0x3b,
0x39,0x36,0x34,0x31,0x2f,0x2c,0x2a,0x28,
0x25,0x23,0x21,0x1f,0x1d,0x1b,0x19,0x17,
0x15,0x14,0x12,0x11,0xf,0xe,0xc,0xb,
0xa,0x9,0x7,0x6,0x5,0x5,0x4,0x3,
0x2,0x2,0x1,0x1,0x1,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x1,0x1,0x1,0x2,
0x2,0x3,0x4,0x5,0x5,0x6,0x7,0x9,
0xa,0xb,0xc,0xe,0xf,0x11,0x12,0x14,
0x15,0x17,0x19,0x1b,0x1d,0x1f,0x21,0x23,
0x25,0x28,0x2a,0x2c,0x2f,0x31,0x34,0x36,
0x39,0x3b,0x3e,0x41,0x43,0x46,0x49,0x4c,
0x4f,0x52,0x55,0x58,0x5a,0x5d,0x61,0x64,
0x67,0x6a,0x6d,0x70,0x73,0x76,0x79,0x7c};
#endif
/*#define BITRATE_PRESCALER IIC_PRESCALER_1_gc
#define BITRATE 200*/

uint8_t stored_data = 0;

uint8_t iic_callback_fun(iic_t *iic, uint8_t received_data){
	if(iic -> error_state != IIC_NO_ERROR){
		PORTD = PORTD ^ (1 << PD6); // toggle an error LED
	}

	if(iic -> state == IIC_SLAVE_RECEIVER){
		stored_data = received_data; // store data
		OCR0A = stored_data;
	}

	if(iic -> state == IIC_SLAVE_TRANSMITTER){
		return stored_data;
	}

	PORTD = PORTD ^ (1 << PD5); // toggle an LED to indicate data received
	return 0;
}

int main(void){
	// Setup things
	
	#ifdef MASTER_MODE
	DDRD |= (1 << DDD5) | (1 << DDD6) | (1 << DDD7);
	DDRB |= (1 << DDB1) | (1 << DDB2) | (1 << DDB3) | (1 << DDB4);
	PORTD = 1 << PD5;
	_delay_ms(500);
	PORTD = 1 << PD6;
	_delay_ms(500);
	PORTD = 1 << PD7;
	_delay_ms(500);
	PORTD = 0;
	setup_iic(ADDRESS, false, false, BITRATE, BITRATE_PRESCALER, 20, &iic_callback_fun);
	#else
	DDRD |= (1 << DDD6);
	// PWM on OCOA (PD6)
	TCCR0A |= (2 << COM0A0) | (1 << WGM00);
	OCR0A = 0;
	TCCR0B |= (1 << CS00) | (0 << WGM02);
	setup_iic(ADDRESS, true, true, BITRATE, BITRATE_PRESCALER, 20, &iic_callback_fun);
	#endif
	_delay_ms(500);

	enable_iic();
	sei();

	uint8_t dat = 0;

	while(1){
		#ifdef MASTER_MODE
		PORTB &= ~((1 << PB1) | (1 << PB2) | (1 << PB3));
		PORTB |= (1 << PB4);
		iic_clear_error();
		iic_write_one(REMOTE_ADDRESS, sine_lut[++dat]);
		while(IIC_MODULE.state != IIC_IDLE);
		if(IIC_MODULE.error_state != IIC_NO_ERROR){
			PORTD = PORTD ^ (1 << PD5);
			PORTB |= (1 << PB3);
			iic_clear_error();
		}else{
			PORTD |= (1 << PD7);
		}
		iic_clear_error();
		iic_read_one(REMOTE_ADDRESS);
		while(IIC_MODULE.state != IIC_IDLE);
		if(IIC_MODULE.error_state != IIC_NO_ERROR){
			PORTD = PORTD ^ (1 << PD5);
			PORTB |= (1 << PB1);
			iic_clear_error();
		}else{
			if(IIC_MODULE.data_buf != sine_lut[dat]){
				PORTD |= (1 << PD5);
				PORTB |= (1 << PB2);
			}
		}

		
		/*do{
			iic_clear_error();
			iic_write_one(REMOTE_ADDRESS, sine_lut[++dat]);
			while(IIC_MODULE.state != IIC_IDLE);
		}while(IIC_MODULE.error_state == IIC_MT_ADDR_NACK);
		if(IIC_MODULE.error_state != IIC_NO_ERROR){
			PORTD = PORTD ^ (1 << PD5);
			PORTB |= (1 << PB3);
			iic_clear_error();
		}else{
			PORTD |= (1 << PD7);
		}
		//_delay_ms(10);

		PORTB &= ~((1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4));
		do{
			iic_clear_error();
			iic_read_one(REMOTE_ADDRESS);
			while(IIC_MODULE.state != IIC_IDLE);
		}while(IIC_MODULE.error_state == IIC_MR_ADDR_NACK);
		if(IIC_MODULE.error_state != IIC_NO_ERROR){
			if(IIC_MODULE.error_state != IIC_MR_DATA_NACK){
				PORTD = PORTD ^ (1 << PD5);
				PORTB |= (1 << PB1);
			}
			iic_clear_error();
		}else{
			if(IIC_MODULE.data_buf != sine_lut[dat]){
				PORTD |= (1 << PD5);
				PORTB |= (1 << PB2);
			}
		}*/
		_delay_ms(10);

		#else
		if(IIC_MODULE.state == IIC_SLAVE_TRANSMITTER){
			PORTB |= (1 << PB3);
		}else{
			PORTB &= ~(1 << PB3);
		}
		#endif
	}

	return 0;
}
