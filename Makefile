TARGET=mesh.hex
OBJECTS=main.o

RFM12BASEDIR=rfm12-1.1
RFM12LIBDIR=$(RFM12BASEDIR)/src
RFM12LIB=$(RFM12LIBDIR)/librfm12.a
RFM12CONFIG=$(RFM12LIBDIR)/rfm12_config.h

CFLAGS=-Wall -Wextra -Werror -I$(RFM12LIBDIR) -I$(RFM12LIBDIR)/include
LDFLAGS=-L$(RFM12LIBDIR) -lrfm12

all: $(TARGET)

$(RFM12LIB):
	$(MAKE) -C $(RFM12BASEDIR)/

%.o: %.c $(RFM12CONFIG)
	avr-gcc $(CFLAGS) -mmcu=atmega8 -c $<

$(TARGET:.hex=.elf): $(RFM12LIB) $(OBJECTS)
	avr-gcc -mmcu=atmega8 -o $@ $(^:$(RFM12LIB)=) $(LDFLAGS)

$(TARGET): $(TARGET:.hex=.elf)
	avr-objcopy -R .eeprom -O ihex $< $@

flash: $(TARGET)
	su -c "avrdude -p m8 -c avrisp2 -Pusb -U flash:w:$(TARGET)"

erase:
	su -c "avrdude -p m8 -c avrisp2 -Pusb -e"

clean:
	$(RM) $(OBJECTS) $(TARGET)
	$(MAKE) -C rfm12-1.1 clean


.PHONY: $(RFM12LIB) clean all erase flash
