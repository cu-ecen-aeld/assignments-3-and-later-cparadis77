#
# Small Makefile to compile the Server application 'aesdsocket'
#
# usage:	make												to build for x86-64
#			make all											to build for x86-64
#			make CROSS_COMPILE=aarch64-none-linux-gnu- all		to build for ARM 64 bits
#
#			make clean			
#

#CC=gcc							#compiler
CC=g++							#compiler

#CFLAGS=-g -Wall				#compiler options
CFLAGS=-g -Wall -Werror			#compiler options

TARGET=aesdsocket   			#target file name

all: server

server: aesdsocket.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(TARGET) aesdsocket.c

.PHONY: clean

clean:
	rm -f $(TARGET)

.DEFAULT_GOAL := all
