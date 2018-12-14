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
 
 * iic_extras.h
 * Extra IIC functions (e.g. automatic address negotiation.
 */

#pragma once
#include <iic/common.h>

typedef uint8_t IIC_COMMAND_t; //TODO left off here

//===========================================================================//
//== Dynamic-address-allocation commands                                   ==//
//===========================================================================//

/* IIC_COMMAND_REQUEST_ADDRESS
 * target address: 0x01 (address server) ONLY
 * length: 0
 * purpose: request an address from the address server
 */
#define IIC_COMMAND_REQUEST_ADDRESS 0xA0

/* IIC_COMMAND_ADDRESS_ALLOCATION
 * target address: 0x00 (general-call) ONLY
 * length: 1
 * syntax: [ command | NEW_ADDRESS ]
 * purpose: inform the device which last sent IIC_COMMAND_REQUEST_ADDRESS that
 *          the address "NEW_ADDRESS" has been allocated for it.
 */
#define IIC_COMMAND_ADDRESS_ALLOCATION 0xA1

/* IIC_COMMAND_NO_ROOM_ON_BUS
 * target address: 0x00 (general-call) ONLY
 * length: 0
 * purpose: inform the device which last sent IIC_COMMAND_REQUEST_ADDRESS that
 *          an address could not be allocated for it, as the iic bus is
 *          currently full.
 * note: if you *really* need an address, try RELEASE_REQUEST'ing addresses
 *       until one of them fails to dispute.
 */
#define IIC_COMMAND_NO_ROOM_ON_BUS 0xA2

/* IIC_COMMAND_RELEASE_ADDRESS
 * target address: 0x01 (address server) ONLY
 * length: 1
 * syntax: [ command | ADDRESS_TO_RELEASE ]
 * purpose: request that the address "ADDRESS_TO_RELEASE" be un-allocated. Used
 *          by slaves that are about to disconnect from the bus, and by the
 *          address server when it wants to release an inactive address.
 */
#define IIC_COMMAND_RELEASE_REQUEST 0xA9

/* IIC_COMMAND_RELEASE_ACKNOWLEDGE
 * target address: 0x00 (general-call) ONLY
 * length: 1
 * syntax: [ command | RELEASED_ADDRESS ]
 * purpose: inform the device at RELEASED_ADDRESS that its address has been
 *          un-allocated, and it should immediately stop listening at that
 *          address. Also inform other devices that the address is now free.
 */
#define IIC_COMMAND_RELEASE_ACKNOWLEDGE 0xAA
#define IIC_COMMAND_RELEASE_DISPUTED 0xAB
#define IIC_COMMAND_RELEASE_FORCE 0xAC
#define IIC_COMMAND_RELEASE_NOT_ALLOCATED 0xAD // sent when we have no record of a slave at the requested address


//===========================================================================//
//== LED control commands                                                  ==//
//===========================================================================//
/* IIC_COMMAND_LED_WRITE_WORD
 * target address: any
 * length: 3
 * syntax: [ command | CHANNEL | LOW_BYTE | HIGH_BYTE ]
 * purpose: write the RGB channel at CHANNEL to the value represented by the
 *          16-bit integer (HIGH_BYTE << 8) | (LOW_BYTE)
 *
 * note: channel values are:
 *       * 0 = RED
 *       * 1 = GREEN
 *       * 2 = BLUE
 */
#define IIC_COMMAND_LED_WRITE_WORD 0x20

/* IIC_COMMAND_LED_SET_PATTERN
 * target address: any
 * length: 1
 * syntax: [ command | PATTERN_NUMBER ]
 * purpose: set the pattern for an LED device to use.
 */
#define IIC_COMMAND_LED_SET_PATTERN 0x21

#ifdef ADDRESS_SERVER
bool handle_address_negotiation(uint8_t command);
bool handle_address_release(uint8_t command, uint8_t slave_address, uint8_t dispute_byte);
void do_address_response(uint8_t allocated_address);
#endif
