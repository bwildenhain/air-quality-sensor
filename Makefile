#!/usr/bin/make -f

all:
	gcc -O2 -g -Wall -Werror -pedantic -std=gnu99 `pkg-config --cflags libusb` -o air air.c `pkg-config --libs libusb`

lcean: clean
clean:
	rm -f air
