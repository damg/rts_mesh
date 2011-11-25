#avr-gcc -mmcu=atmega8 -o hello.elf hello.c
#avr-objcopy -R .eeprom -0 ihex hello.elf hello.hex
#avrdude -p m8 -c avrisp2 -Pusb -U flash:w:hello.hex 

HEXES=blinky.hex

all: $(HEXES)

%.hex: %.elf
	avr-objcopy -R .eeprom -O ihex $< $@
%.elf: %.c
	avr-gcc -mmcu=atmega8 -o $@ $<

flash: $(HEXES)
	avrdude -p m8 -c avrisp2 -Pusb -U flash:w:$(HEX)

clean:
	$(RM) *~ *.o *.elf *.hex
