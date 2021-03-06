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

 * iic.h
 * function headers, enums, defines, structs for the iic library
 */

#pragma once
#include <avr/io.h>
#include <iic/common.h>

#ifdef __AVR_ATmega328PB__
// The 328PB has 2 iic modules - we're only using one
	#define TWCR TWCR0
	#define TWDR TWDR0
	#define TWSR TWSR0
	#define TWAR TWAR0
	#define TWBR TWBR0
	#define TWI_vect TWI0_vect
#endif

#define TWCR_ENABLE (1 << TWEN) | (1 << TWIE) | (1 << TWEA)
#define TWCR_DISABLE 0
#define TWCR_NEXT TWCR_ENABLE | (1 << TWINT)
#define TWCR_LAST_BYTE (1 << TWEN) | (1 << TWIE) | (1 << TWINT)
#define TWCR_START TWCR_ENABLE | (1 << TWSTA)
#define TWCR_STOP TWCR_ENABLE  | (1 << TWSTO) | (1 << TWINT)

typedef enum{
	IIC_PRESCALER_1_gc = 0,
	IIC_PRESCALER_4_gc = 1,
	IIC_PRESCALER_16_gc = 2,
	IIC_PRESCALER_64_gc = 3
} iic_prescaler_t;

typedef enum{
	IIC_TRYING_TO_SEIZE_BUS,
	IIC_SLAVE_TRANSMITTER,
	IIC_SLAVE_RECEIVER,
	IIC_SLAVE_RECEIVER_WAITING,
	IIC_MASTER_TRANSMITTER,
	IIC_MASTER_RECEIVER,
	IIC_IDLE,
	IIC_DISCONNECTED
} iic_state_t;

typedef enum{
	IIC_NO_ERROR,                         // Error type codes
	IIC_MT_ARBITRATION_LOST,              // A
	IIC_MR_ARBITRATION_LOST,              // B
	IIC_ARBITRATION_LOST_AND_ST_SELECTED, // C
	IIC_ARBITRATION_LOST_AND_SR_SELECTED, // D
	IIC_MT_ADDR_NACK,                     // E
	IIC_MT_DATA_NACK,                     // F
	IIC_MR_ADDR_NACK,                     // G
	IIC_MR_DATA_NACK,                     // H
	IIC_ST_DATA_NACK,                     // I
	IIC_SR_DATA_NACK,                     // J
	IIC_SR_STOP,                          // K
	IIC_BUS_ERROR                         // L
} iic_error_t;

typedef struct iic_t{
	bool        data_ready; // read data is ready in data_buf
	iic_error_t error_state; // errors on the IIC bus
	uint8_t     data_buf; // small data buffer
	uint8_t     data_buf_high; // extension for 2-byte commands
	uint8_t     *big_data_buf;  // multi-byte data buffer for 3-byte (or more) transactions
	uint8_t     data_buf_index; // index for multi-byte transactions
	uint8_t     remote_addr_buf; // remote address buffer
	iic_state_t state; // current state (slave/master/disconnected)
	iic_state_t intent; // the state the module is trying to reach
	bool        slave_enable; // allow the system to be addressed as a slave device
	bool        force_small_multibyte_read; // for people who call iic_read_many for 1 or 2-byte transactions
	uint8_t     transaction_len; // number of bytes left to tx/rx this transaction
	uint8_t     retry_max; // number of times to retry a data transmission before giving up
	uint8_t     retry_count; // number of times the current data transmission has been retried
	uint8_t (*callback)(volatile struct iic_t*, uint8_t); // callback function for slave functionality
} iic_t;

volatile iic_t IIC_MODULE;

void setup_iic(
	uint8_t address, 
	bool slave_enable,
	bool respond_to_general_call,
	uint8_t bitrate,
	iic_prescaler_t bitrate_prescaler, 
	uint8_t retry_max,
	uint8_t (*callback)(volatile iic_t *iic, uint8_t received_data)
	);

void enable_iic();
void disable_iic();

void iic_write_one(uint8_t remote_address, uint8_t dat);
void iic_write_two(uint8_t remote_address, uint8_t dat_low, uint8_t dat_high);
void iic_write_many(uint8_t remote_address, uint8_t *data_buffer, uint8_t buffer_len);
void iic_read_one(uint8_t remote_address);
void iic_read_two(uint8_t remote_address);
void iic_read_many(uint8_t remote_address, uint8_t *buffer, uint8_t buffer_len);

void iic_clear_error();
