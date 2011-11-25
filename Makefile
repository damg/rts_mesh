#avr-gcc -mmcu=atmega8 -o hello.elf hello.c
#avr-objcopy -R .eeprom -0 ihex hello.elf hello.hex
#avrdude -p m8 -c avrisp2 -Pusb -U flash:w:hello.hex 

TARGET=mesh.hex
OBJECTS=main.o

CFLAGS=-Wall -Wextra -Werror

all: $(TARGET)

%.o: %.c
	avr-gcc $(CFLAGS) -mmcu=atmega8 -c $<

$(TARGET:.hex=.elf): $(OBJECTS)
	avr-gcc $(CFLAGS) -mmcu=atmega8 -o $@ $^

$(TARGET): $(TARGET:.hex=.elf)
	avr-objcopy -R .eeprom -O ihex $< $@

flash: $(TARGET)
	su -c "avrdude -p m8 -c avrisp2 -Pusb -U flash:w:$(TARGET)"

erase:
	su -c "avrdude -p m8 -c avrisp2 -Pusb -e"

clean:
	$(RM) *~ *.o *.elf *.hex
