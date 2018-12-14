#   Copyright 2018 Alexander Shuping
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
# Makefile for sample project

include common.makerules

#####################################################
# Silent mode by default                            #
# (set the environment variable VERBOSE to override #
#####################################################
ifndef VERBOSE
.SILENT:
endif

#####################
# Default Target    #
# Makes all modules #
# (no unit tests)   #
#####################
TOP: lib/iic.o
	echo "$(T_C)library build done."

# Builds all modules and runs the final executable
$(PNAME).hex : $(PNAME).c $(OTHER_MODULES)
	echo "$(T_COMP) $(PNAME).c -> $(PNAME).o"
	avr-gcc -Wall -g --std=c11 -Os -mmcu=atmega328p -c $(PNAME).c
	echo "$(T_LINK) $(PNAME).o -> $(PNAME).elf"
	avr-gcc -Wall -g --std=c11 -mmcu=atmega328p -o $(PNAME).elf $(PNAME).o $(OTHER_MODULES)
	echo "$(T_HEX) $(PNAME).elf -> $(PNAME).hex"
	avr-objcopy -j .text -j .data -O ihex $(PNAME).elf $(PNAME).hex

build/iic.o: src/iic.c build
	echo "$(T_COMP) src/iic.c -> build/iic.o"
	avr-gcc -Wall -g --std=c11 -Iinclude/ -Os -mmcu=atmega328p -c src/iic.c -o lib/iic.o

build:
	mkdir build

lib:
	mkdir lib

UPLOAD : $(PNAME).hex
	echo "$(T_UPL) $(PNAME).hex"
	avrdude -p atmega328p -c avrisp -b 19200 -P /dev/ttyUSB0 -U flash:w:$(PNAME).hex

TERM: 
	avrdude -p atmega328p -c avrisp -b 19200 -P /dev/ttyUSB0 -t


################
# Util Targets #
################
# Cleans up compiled object files / binaries
clean :
	-rm -r project.o project.hex project.elf lib
