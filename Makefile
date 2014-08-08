#!/usr/bin/make -f

CFLAGS=-O4 -g -pedantic -Wall -Werror -std=gnu99 

all: air01 air10

air01: air01.c
	$(CC) $(CFLAGS) `pkg-config --cflags libusb` -o $@ $< `pkg-config --libs libusb`
	
air10: air10.c
	$(CC) $(CFLAGS) `pkg-config --cflags libusb-1.0` -o $@ $< `pkg-config --libs libusb-1.0`

lcean: clean
clean:
	rm -f air air01 air10
