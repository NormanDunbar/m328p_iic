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
 
 * iic_extras.c
 * Extra iic functions
 */

#include <iic/common.h>
#include <iic/iic_extras.h>
#include <iic/iic.h>

#ifdef ADDRESS_SERVER

volatile uint8_t address_arr[16]={3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
// address size = 8 * 16 = 128 possible addresses
// 0 and 1 (GC and our address, respectively) should begin as taken.

bool handle_address_negotiation(uint8_t command){
	if(command == IIC_COMMAND_REQUEST_ADDRESS){
		// scan through address table and find the lowest free address
		for(int i = 0; i < 16; i++){
			for(int j = 0; j < 8; j++){
				if(!(address_arr[i] & i(1 << j))){
					address_arr[i] |= (1 << j); // set address as taken
					// broadcast newly-allocated address
					iic_write_two(0x0, IIC_COMMAND_ADDRESS_ALLOCATION, (i << 3) + j);
					return true;
				}
			}
		}
	}else{
		return false;
	}
}

bool handle_address_release(uint8_t command, uint8_t slave_address, uint8_t dispute_byte){
	if(command == IIC_COMMAND_RELEASE_ADDRESS){
		if(address_arr[(slave_address & 0x78) >> 3] & (1 << (slave_address & 0x07))){
			address_arr[(slave_address & 0x78) >> 3] &= ~(1 << (slave_address & 0x07));
			iic_write_one(slave_address, IIC_COMMAND_RELEASE_ACKNOWLEDGE);
			return true
		}else{
			iic_write_one(slave_address, IIC_COMMAND_RELEASE_NOT_ALLOCATED);
			return true
		}
	}else{
		return false;
	}
}
#endif
