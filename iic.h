// I2C utilities

#include <avr/io.h>
#include "common.h"

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
	IIC_MASTER_TRANSMITTER,
	IIC_MASTER_RECEIVER,
	IIC_IDLE,
	IIC_DISCONNECTED
} iic_state_t;

typedef enum{
	IIC_NO_ERROR,
	IIC_MT_ARBITRATION_LOST,
	IIC_MR_ARBITRATION_LOST,
	IIC_ARBITRATION_LOST_AND_ST_SELECTED,
	IIC_ARBITRATION_LOST_AND_SR_SELECTED,
	IIC_MT_ADDR_NACK,
	IIC_MT_DATA_NACK,
	IIC_MR_ADDR_NACK,
	IIC_MR_DATA_NACK,
	IIC_ST_DATA_NACK,
	IIC_SR_DATA_NACK,
	IIC_SR_STOP,
	IIC_BUS_ERROR
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
	uint8_t (*callback)(struct iic_t*, uint8_t); // callback function for slave functionality
} iic_t;

volatile iic_t IIC_MODULE;

void setup_iic(
	uint8_t address, 
	bool slave_enable,
	bool respond_to_general_call,
	uint8_t bitrate,
	iic_prescaler_t bitrate_prescaler, 
	uint8_t retry_max,
	uint8_t (*callback)(iic_t *iic, uint8_t received_data)
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
