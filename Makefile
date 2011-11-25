#avr-gcc -mmcu=atmega8 -o hello.elf hello.c
#avr-objcopy -R .eeprom -0 ihex hello.elf hello.hex
#avrdude -p m8 -c avrisp2 -Pusb -U flash:w:hello.hex 

ELFS=hello.elf

%.hex: %.elf
	avr-objcopy -R .eeprom -0 ihex $< $@
%.elf: %.c
	avr-gcc -mmcu=atmega8 -o $@ $<

flash: $(ELFS)
	avrdude -p m8 -c avrisp2 -Pusb -U flash:w:$(ELF)
