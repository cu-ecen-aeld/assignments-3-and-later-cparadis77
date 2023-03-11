#
# usage:	make all											to build for x86-64
#			make clean
#
#			make CROSS_COMPILE=aarch64-none-linux-gnu- all		to build for ARM 64 bits
#			make CROSS_COMPILE=aarch64-none-linux-gnu- clean
#


CC=gcc          #compiler
TARGET=writer   #target file name

all:
	$(CROSS_COMPILE)$(CC) -g -Wall	-c -o writer.o writer.c
	$(CROSS_COMPILE)$(CC) -g -Wall -I/ writer.o -o $(TARGET)

clean:
	rm -f *.o $(TARGET) *.elf *.map

