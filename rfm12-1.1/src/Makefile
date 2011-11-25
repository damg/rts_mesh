CC=avr-gcc
CFLAGS=-Wall -Wextra -mmcu=atmega8
#DEFINES=-DRFM12_BASEBAND=RFM12_BAND_433 -DRFM12_TX_BUFFER_SIZE=32 -DRFM12_RX_BUFFER_SIZE=32
AR=avr-ar
RANLIB=avr-ranlib

TARGET=librfm12.a
OBJECTS=rfm12.o

all: compile

compile: $(TARGET)

$(TARGET): $(OBJECTS)
	$(RM) $@
	$(AR) rs $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(DEFINES) -c $<

clean:
	$(RM) $(TARGET) $(OBJECTS) *~
