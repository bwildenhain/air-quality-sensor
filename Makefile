#!/usr/bin/make -f

all:
	gcc -O0 -g -pedantic -Wall -Werror -std=gnu99 `pkg-config --cflags libusb` -o air01 air01.c `pkg-config --libs libusb`
	gcc -O0 -g -pedantic -Wall -Werror -std=gnu99 `pkg-config --cflags libusb-1.0` -o air10 air10.c `pkg-config --libs libusb-1.0`

lcean: clean
clean:
	rm -f air air01 air10
