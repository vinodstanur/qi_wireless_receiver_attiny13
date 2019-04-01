# AVR-GCC Makefile
PROJECT=qi
SOURCES=main.c
CC=avr-gcc
OBJCOPY=avr-objcopy
MMCU=attiny13
COM=COM2	
CFLAGS=-mmcu=$(MMCU) -Wall

$(PROJECT).hex: $(PROJECT).out
	$(OBJCOPY) -j .text -j .data -O ihex $(PROJECT).out $(PROJECT).hex

$(PROJECT).out: $(SOURCES)
	$(CC) $(CFLAGS) -Os -I./ -o $(PROJECT).out $(SOURCES)

p: $(PROJECT).hex
	avrdude -c arduino -p t13 -P $(COM) -b 19200 -U flash:w:$(PROJECT).hex:i

burn_fuse: $(PROJECT).hex
	avrdude -c arduino -p t13 -P $(COM) -b 19200 -U lfuse:w:0x7A:m	-U hfuse:w:0xF9:m	-U lock:w:0xFF:m
	
clean:
	del -f $(PROJECT).out
	del -f $(PROJECT).hex
	del -f $(PROJECT).s
