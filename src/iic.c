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

 * iic.c
 * main library file
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#include <iic/iic.h>
#include <iic/common.h>

void setup_iic(
	uint8_t address, 
	bool slave_enable, 
	bool respond_to_general_call, 
	uint8_t bitrate,
	iic_prescaler_t bitrate_prescaler,
	uint8_t retry_max,
	uint8_t (*callback)(volatile iic_t *iic, uint8_t received_data)
){
	IIC_MODULE.state = IIC_DISCONNECTED;
	IIC_MODULE.slave_enable = slave_enable;
	IIC_MODULE.error_state = IIC_NO_ERROR;
	IIC_MODULE.callback = callback;
	IIC_MODULE.retry_max = retry_max;

	if(slave_enable){
		TWAR = (address << 1) | (respond_to_general_call);
	}

	TWBR = bitrate;
	TWSR |= bitrate_prescaler;
}

void enable_iic(){
	TWCR = TWCR_ENABLE;
	IIC_MODULE.state = IIC_IDLE;
}

void disable_iic(){
	TWCR = TWCR_DISABLE;
	IIC_MODULE.state = IIC_DISCONNECTED;
}

void iic_write_one(uint8_t remote_address, uint8_t dat){
	IIC_MODULE.data_ready = false;
	IIC_MODULE.data_buf = dat;
	IIC_MODULE.transaction_len = 1;
	IIC_MODULE.data_buf_index = 0;
	IIC_MODULE.remote_addr_buf = remote_address;
	IIC_MODULE.intent = IIC_MASTER_TRANSMITTER;
	IIC_MODULE.state = IIC_TRYING_TO_SEIZE_BUS;
	TWCR = TWCR_START;
}

void iic_write_two(uint8_t remote_address, uint8_t dat_low, uint8_t dat_high){
	IIC_MODULE.data_ready = false;
	IIC_MODULE.data_buf = dat_low;
	IIC_MODULE.data_buf_high = dat_high;
	IIC_MODULE.transaction_len = 2;
	IIC_MODULE.data_buf_index = 0;
	IIC_MODULE.remote_addr_buf = remote_address;
	IIC_MODULE.intent = IIC_MASTER_TRANSMITTER;
	IIC_MODULE.state = IIC_TRYING_TO_SEIZE_BUS;
	TWCR = TWCR_START;
}

void iic_write_many(uint8_t remote_address, uint8_t *data_buffer, uint8_t buffer_len){
	if(buffer_len == 1){ // one-byte mode is handled separately
		IIC_MODULE.data_buf = data_buffer[0];
	}else if(buffer_len == 2){ // two-byte mode is handled separately
		IIC_MODULE.data_buf = data_buffer[0];
		IIC_MODULE.data_buf_high = data_buffer[1];
	}else{
		IIC_MODULE.big_data_buf = data_buffer;
	}

	IIC_MODULE.data_ready = false;
	IIC_MODULE.remote_addr_buf = remote_address;
	IIC_MODULE.intent = IIC_MASTER_TRANSMITTER;
	IIC_MODULE.state = IIC_TRYING_TO_SEIZE_BUS;
	IIC_MODULE.transaction_len = buffer_len;
	IIC_MODULE.data_buf_index = 0;
	TWCR = TWCR_START;
}

void iic_read_one(uint8_t remote_address){
	IIC_MODULE.force_small_multibyte_read = false;
	IIC_MODULE.data_ready = false;
	IIC_MODULE.remote_addr_buf = remote_address;
	IIC_MODULE.transaction_len = 1;
	IIC_MODULE.data_buf_index = 0;
	IIC_MODULE.intent = IIC_MASTER_RECEIVER;
	IIC_MODULE.state = IIC_TRYING_TO_SEIZE_BUS;
	TWCR = TWCR_START;
}

void iic_read_two(uint8_t remote_address){
	IIC_MODULE.force_small_multibyte_read = false;
	IIC_MODULE.data_ready = false;
	IIC_MODULE.remote_addr_buf = remote_address;
	IIC_MODULE.transaction_len = 2;
	IIC_MODULE.data_buf_index = 0;
	IIC_MODULE.intent = IIC_MASTER_RECEIVER;
	IIC_MODULE.state = IIC_TRYING_TO_SEIZE_BUS;
	TWCR = TWCR_START;
}

void iic_read_many(uint8_t remote_address, uint8_t *buffer, uint8_t buffer_len){
	IIC_MODULE.data_ready = false;
	IIC_MODULE.remote_addr_buf = remote_address;
	IIC_MODULE.transaction_len = buffer_len;
	IIC_MODULE.big_data_buf = buffer;
	IIC_MODULE.data_buf_index = 0;
	IIC_MODULE.force_small_multibyte_read = true;
	IIC_MODULE.intent = IIC_MASTER_RECEIVER;
	IIC_MODULE.state = IIC_TRYING_TO_SEIZE_BUS;
	TWCR = TWCR_START;
}

void iic_clear_error(){
	IIC_MODULE.error_state = IIC_NO_ERROR;
}

ISR(TWI_vect){
	switch(TWSR & TW_STATUS_MASK){
		case TW_START:
		case TW_REP_START:; // kludge to allow declaring a variable directly after the case statement.
			bool read_mode = false;
			if(IIC_MODULE.intent == IIC_MASTER_TRANSMITTER){
				IIC_MODULE.state = IIC_MASTER_TRANSMITTER;
				read_mode = false;
			}else if(IIC_MODULE.intent == IIC_MASTER_RECEIVER){
				IIC_MODULE.state = IIC_MASTER_RECEIVER;
				read_mode = true;
			}

			TWDR = (IIC_MODULE.remote_addr_buf << 1) | (read_mode << 0);
			TWCR = TWCR_NEXT;
			break;
		

		// ================================================================
		// Master-transmitter mode
		// ================================================================
		case TW_MT_SLA_ACK: // slave is acknowledging address - send data
			if(IIC_MODULE.transaction_len <= 2){
				TWDR = IIC_MODULE.data_buf;
				IIC_MODULE.data_buf_index++;
			}else{
				TWDR = IIC_MODULE.big_data_buf[IIC_MODULE.data_buf_index++];
			}
			IIC_MODULE.retry_count = 0;
			TWCR = TWCR_NEXT;
			break;

		case TW_MT_SLA_NACK: // no slave is acknowledging address - retry or abort
			if(IIC_MODULE.retry_count++ >= IIC_MODULE.retry_max){
				IIC_MODULE.error_state = IIC_MT_ADDR_NACK;
				IIC_MODULE.state = IIC_IDLE;
				IIC_MODULE.intent = IIC_IDLE;
				IIC_MODULE.retry_count = 0;
				TWCR = TWCR_STOP;
			}else{
				TWCR = TWCR_START | TWCR_NEXT; // retry
			}
			break;

		case TW_MT_DATA_ACK: // slave is acknowledging data
			IIC_MODULE.retry_count = 0;
			if(IIC_MODULE.transaction_len == IIC_MODULE.data_buf_index){
				// end transaction
				IIC_MODULE.state = IIC_IDLE;
				IIC_MODULE.intent = IIC_IDLE;
				TWCR = TWCR_STOP;
			}else if(IIC_MODULE.transaction_len == 2){
				TWDR = IIC_MODULE.data_buf_high;
				IIC_MODULE.data_buf_index++;
				TWCR = TWCR_NEXT;
			}else{
				TWDR = IIC_MODULE.big_data_buf[IIC_MODULE.data_buf_index++];
				TWCR = TWCR_NEXT;
			}
			break;

		case TW_MT_DATA_NACK: // slave has not acknowledged data
			if(IIC_MODULE.retry_count++ >= IIC_MODULE.retry_max){
				// If we're out of retries, abort
				IIC_MODULE.retry_count = 0;
				IIC_MODULE.error_state = IIC_MT_DATA_NACK;
				IIC_MODULE.state = IIC_IDLE;
				IIC_MODULE.intent = IIC_IDLE;
				TWCR = TWCR_STOP;
			}else{
				// otherwise, retry
				if(IIC_MODULE.transaction_len == 1){
					TWDR = IIC_MODULE.data_buf;
				}else if(IIC_MODULE.transaction_len == 2){
					TWDR = IIC_MODULE.data_buf_index == 1 ? IIC_MODULE.data_buf : IIC_MODULE.data_buf_high;
				}else{
					TWDR = IIC_MODULE.big_data_buf[IIC_MODULE.data_buf_index-1];
				}
				TWCR = TWCR_NEXT;
			}
			break;

		/*case TW_MT_ARB_LOST: // we lost arbitration to another master - abort & set error
			IIC_MODULE.error_state = IIC_MT_ARBITRATION_LOST;
			IIC_MODULE.state = IIC_IDLE;
			IIC_MODULE.intent = IIC_IDLE;
			TWCR &= ~(TWINT | TWSTA);
			TWCR |= TWINT;
			break;*/ // this is the same as TW_MR_ARB_LOST, so it's handled down there.
			// if we were ALSO selected as a slave, see either:
			// TW_ST_ARB_LOST_SLA_ACK, in the SLAVE_TRANSMITTER section, below, or 
			// TW_SR_ARB_LOST_SLA_ACK, in the SLAVE_RECEIVER section, further below.
		

		// ================================================================
		// Master-receiver mode
		// ================================================================
		case TW_MR_SLA_ACK: // slave is acknowledging address & ready to read - continue.
			IIC_MODULE.data_ready = false;
			IIC_MODULE.retry_count = 0;
			TWCR = IIC_MODULE.transaction_len == 1 ? TWCR_LAST_BYTE : TWCR_NEXT;
			break;

		case TW_MR_SLA_NACK: // no slave is acknowledging - retry or abort
			if(IIC_MODULE.retry_count++ >= IIC_MODULE.retry_max){
				IIC_MODULE.error_state = IIC_MR_ADDR_NACK;
				IIC_MODULE.state = IIC_IDLE;
				IIC_MODULE.intent = IIC_IDLE;
				IIC_MODULE.retry_count = 0;
				TWCR = TWCR_STOP;
			}else{
				TWCR = TWCR_START | TWCR_NEXT; // retry
			}
			break;

		case TW_MR_DATA_ACK: // slave has sent data, which we acknowledged
			if(IIC_MODULE.transaction_len == 1){
				// this should never happen, since we're always going to NACK the last byte
				IIC_MODULE.data_buf = TWDR;
				IIC_MODULE.data_ready = true;
				IIC_MODULE.state = IIC_IDLE;
				IIC_MODULE.intent = IIC_IDLE;
				TWCR = TWCR_LAST_BYTE; // continue NACKing
			}else if(IIC_MODULE.transaction_len == 2){
				// Ask for the last byte
				IIC_MODULE.data_buf = TWDR;
				IIC_MODULE.state = IIC_IDLE;
				IIC_MODULE.intent = IIC_IDLE;
				TWCR = TWCR_LAST_BYTE;
			}else{
				// Get the next byte
				IIC_MODULE.big_data_buf[IIC_MODULE.data_buf_index] = TWDR;
				if(IIC_MODULE.data_buf_index++ == IIC_MODULE.transaction_len - 2){
					// The next byte is the last - NACK it
					TWCR = TWCR_LAST_BYTE;
				}else{
					TWCR = TWCR_NEXT;
				}
			}
			break;

		case TW_MR_DATA_NACK: // slave has sent the last data byte - finish up
			if(IIC_MODULE.transaction_len == 1){
				IIC_MODULE.data_buf = TWDR;
			}else if(IIC_MODULE.transaction_len == 2){
				IIC_MODULE.data_buf_high = TWDR;
			}else{
				IIC_MODULE.big_data_buf[IIC_MODULE.data_buf_index] = TWDR;
			}
			IIC_MODULE.data_ready = true;
			IIC_MODULE.state = IIC_IDLE;
			IIC_MODULE.state = IIC_IDLE;
			TWCR = TWCR_STOP;
			break;

		case TW_MR_ARB_LOST: // we lost arbitration to another master - abort & set error
			IIC_MODULE.error_state = IIC_MR_ARBITRATION_LOST;
			IIC_MODULE.state = IIC_IDLE;
			IIC_MODULE.intent = IIC_IDLE;
			TWCR = TWCR_NEXT;
			break;
		

		// ================================================================
		// Slave transmitter
		// ================================================================
		case TW_ST_SLA_ACK: // master requests data - call the callback function and send result
			IIC_MODULE.state = IIC_SLAVE_TRANSMITTER;
			// NOTE: IIC_MODULE.intent should be IDLE now.
			IIC_MODULE.data_buf = IIC_MODULE.callback(&IIC_MODULE, 0);
			TWDR = IIC_MODULE.data_buf;
			TWCR = TWCR_NEXT;
			break;

		case TW_ST_ARB_LOST_SLA_ACK: // we lost arbitration and were selected as a slave
		                             // set an error state, call the callback, and send 
																 // what the callback returns.
			IIC_MODULE.error_state = IIC_ARBITRATION_LOST_AND_ST_SELECTED;
			IIC_MODULE.state = IIC_SLAVE_TRANSMITTER;
			// NOTE: IIC_MODULE.intent will still be IIC_MASTER_TRANSMITTER or IIC_MASTER_RECEIVER.
			IIC_MODULE.data_buf = IIC_MODULE.callback(&IIC_MODULE, 0);
			TWDR = IIC_MODULE.data_buf;
			TWCR = TWCR_NEXT;
			break;

		case TW_ST_DATA_ACK: // master has successfully received data - finish.
		case TW_ST_LAST_DATA:
			IIC_MODULE.state = IIC_IDLE;
			IIC_MODULE.intent = IIC_IDLE;
			TWCR = TWCR_NEXT;
			break;

		case TW_ST_DATA_NACK: // master has not received data - set error & finish.
			IIC_MODULE.state = IIC_IDLE;
			IIC_MODULE.intent = IIC_IDLE;
			IIC_MODULE.error_state = IIC_ST_DATA_NACK;
			TWCR = TWCR_NEXT;
			break;


		// ================================================================
		// slave-receiver mode
		// ================================================================
		case TW_SR_SLA_ACK: // master is sending data - acknowledge.
		case TW_SR_GCALL_ACK:
			IIC_MODULE.state = IIC_SLAVE_RECEIVER;
			IIC_MODULE.data_ready = false;
			TWCR = TWCR_NEXT;
			break;

		case TW_SR_ARB_LOST_GCALL_ACK:
		case TW_SR_ARB_LOST_SLA_ACK: // we lost arbitration and were selected as a slave
		                             // set an error state and acknowledge.
			IIC_MODULE.error_state = IIC_ARBITRATION_LOST_AND_SR_SELECTED;
			IIC_MODULE.state = IIC_SLAVE_RECEIVER;
			IIC_MODULE.data_ready = false;
			TWCR = TWCR_NEXT;
			break;

		case TW_SR_DATA_NACK: // we NACK'ed this byte to indicate EOT - continue, but set error flag.
		case TW_SR_GCALL_DATA_NACK:
			IIC_MODULE.error_state = IIC_SR_DATA_NACK;
		case TW_SR_DATA_ACK: // call the callback function with the returned data
		case TW_SR_GCALL_DATA_ACK:
			IIC_MODULE.callback(&IIC_MODULE, TWDR);
			// NOTE: if this SR cycle follows an arbitration loss from an MT-cycle attempt,
			// IIC_MODULE.data_buf will still contain the data that were going to be transmitted.
			// Check for this condition by seeing if the error state is IIC_ARBITRATION_LOST_AND_SR_SELECTED
			// AND that the intent state is IIC_MASTER_TRANSMITTER.
			
			IIC_MODULE.state = IIC_SLAVE_RECEIVER_WAITING;
			IIC_MODULE.intent = IIC_SLAVE_RECEIVER_WAITING;
			IIC_MODULE.data_ready = true;
			TWCR = TWCR_NEXT;
			break;
		
		case TW_SR_STOP: // master canceled or finished data send
			IIC_MODULE.state = IIC_IDLE;
			IIC_MODULE.intent = IIC_IDLE;
			TWCR = TWCR_NEXT;
			break;


		// ================================================================
		// misc
		// ================================================================
		case TW_BUS_ERROR: // someone is being naughty with the iic lines
			IIC_MODULE.error_state = IIC_BUS_ERROR;
			IIC_MODULE.state = IIC_IDLE;
			IIC_MODULE.state = IIC_IDLE;
			TWCR = TWCR_NEXT;
			break;

		default: // something else - just abort, acknowledge and hope it goes away
			TWCR = TWCR_STOP;
	}
}
