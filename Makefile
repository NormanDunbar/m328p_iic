include common.makerules

PNAME=project

OTHER_MODULES=iic.o

#####################################################
# Silent mode by default                            #
# (set the environment variable VERBOSE to override #
#####################################################
#ifndef VERBOSE
#.SILENT:
#endif

#####################
# Default Target    #
# Makes all modules #
# (no unit tests)   #
#####################
# Builds all modules and runs the final executable
$(PNAME).hex : $(PNAME).c $(OTHER_MODULES)
	echo "$(T_COMP) $(PNAME).c -> $(PNAME).o"
	avr-gcc -Wall -g --std=c11 -Os -mmcu=atmega328p -c $(PNAME).c
	echo "$(T_LINK) $(PNAME).o -> $(PNAME).elf"
	avr-gcc -Wall -g --std=c11 -mmcu=atmega328p -o $(PNAME).elf $(PNAME).o $(OTHER_MODULES)
	echo "$(T_HEX) $(PNAME).elf -> $(PNAME).hex"
	avr-objcopy -j .text -j .data -O ihex $(PNAME).elf $(PNAME).hex

TERM: 
	avrdude -p atmega328p -c avrisp -b 19200 -P /dev/ttyUSB0 -t

iic.o: iic.c
	echo "$(T_COMP) iic.c -> iic.o"
	avr-gcc -Wall -g --std=c11 -Os -mmcu=atmega328p -c iic.c


UPLOAD : $(PNAME).hex
	echo "$(T_UPL) $(PNAME).hex"
	avrdude -p atmega328p -c avrisp -b 19200 -P /dev/ttyUSB0 -U flash:w:$(PNAME).hex

################
# Util Targets #
################
# Cleans up compiled object files / binaries
clean :
	-rm $(PNAME).o $(PNAME).hex $(PNAME).elf
