/*
    air-quality-sensor - user space driver for USB Air Quality Sensor CO-20

    Copyright (C) 2014 Jan-Benedict Glaw

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <libusb.h>

#define DRIVER_NAME_LEN	1024
#define USB_BUF_LEN	1024

#define LEVEL_GREEN	1000
#define LEVEL_YELLOW	1500
#define LEVEL_RED	4000
#define LEVEL_BOOTUP	450

#define USB_VENDOR_CO2_STICK	0x03eb
#define USB_PRODUCT_CO2_STICK	0x2013

enum led_state {
	GREEN = 0,
	YELLOW,
	RED,
	BLINK,
};

static const char * const air_quality[] = {
	[GREEN]		= "GREEN",
	[YELLOW]	= "YELLOW",
	[RED]		= "RED",
	[BLINK]		= "BLINK",
};


static int
read_one_sensor (struct libusb_device *dev)
{
	int ret;
	uint16_t value;
	int len;
	enum led_state colour;
	struct libusb_device_handle *devh;
	char driver_name[DRIVER_NAME_LEN] = "";
	unsigned char usb_io_buf[USB_BUF_LEN] =	"\x40\x68\x2a\x54"
						"\x52\x0a\x40\x40"
						"\x40\x40\x40\x40"
						"\x40\x40\x40\x40";
	struct libusb_device *uplink;

	/* Open USB device.  */
	ret = libusb_open (dev, &devh);
	if (ret) {
		fprintf (stderr, "Failed to libusb_open(%p)\n", (void *) dev);
		ret = -1;
		goto out;
	}

	/* Ensure that the device isn't claimed.  */
	if (libusb_kernel_driver_active (devh, 0/*intrf*/)) {
		fprintf (stderr, "Warning: device is claimed by driver %s, "
			 "trying to unbind it.\n", driver_name);
		ret = libusb_detach_kernel_driver (devh, 0/*intrf*/);
		if (ret) {
			fprintf (stderr, "Failed to detatch kernel driver.\n");
			ret = -2;
			goto out;
		}
	}

	/* Claim device.  */
	ret = libusb_claim_interface (devh, 0/*intrf*/);
	if (ret) {
		fprintf (stderr, "usb_claim_interface() failed with error %d=%s\n",
			 ret, strerror (-ret));
		ret = -3;
		goto out;
	}

	/* Send query command.  */
	ret = libusb_interrupt_transfer (devh, 0x0002/*endpoint*/,
				   usb_io_buf, 0x10/*len*/, &len, 1000/*msec*/);
	if (ret < 0) {
		fprintf (stderr, "Failed to usb_interrupt_write() the initial buffer, ret = %d\n", ret);
		ret = -4;
		goto out_unlock;
	}

	/* Read answer.  */
	ret = libusb_interrupt_transfer (devh, 0x0081/*endpoint*/,
				  usb_io_buf, 0x10/*len*/, &len, 1000/*msec*/);
	if (ret < 0) {
		fprintf (stderr, "Failed to usb_interrupt_read() #1\n");
		ret = -5;
		goto out_unlock;
	}

	/* First read returned 0 bytes, do again.  */
	if (ret == 0) {
		ret = libusb_interrupt_transfer (devh, 0x0081/*endpoint*/,
					  usb_io_buf, 0x10/*len*/, &len, 1000/*msec*/);
	}

	/* Prepare value from first read.  */
	value =   ((unsigned char *) usb_io_buf)[3] << 8
		| ((unsigned char *) usb_io_buf)[2] << 0;

	/* Classify `value'.  */
	if (value == LEVEL_BOOTUP)
		colour = BLINK;
	else if (value <= LEVEL_GREEN)
		colour = GREEN;
	else if (value <= LEVEL_YELLOW)
		colour = YELLOW;
	else
		colour = RED;

	/* Output values.  */
	printf ("Device ");
	for (uplink = dev; uplink; uplink = libusb_get_parent (uplink))
		printf ("%d:", libusb_get_port_number (uplink));
	printf (" value = %u, quality = %s\n",
		(unsigned int) value, air_quality[colour]);

out_unlock:
	ret = libusb_release_interface (devh, 0/*intrf*/);
	if (ret) {
		fprintf (stderr, "Failed to usb_release_interface()\n");
		ret = -5;
	}
out:
	return ret;
}

static int
find_devices (int vendor, int product) {
	int ret = 0, ret2;
	struct libusb_device **devs;
	struct libusb_device *dev;
	struct libusb_device_descriptor desc;
	ssize_t num_devices;
	int i = 0;

	num_devices = libusb_get_device_list(NULL, &devs);
	if (num_devices > 0) {
		while ((dev = devs[i++]) != NULL) {
			ret2 = libusb_get_device_descriptor (dev, &desc);
			if (ret2) {
				ret |= ret2;
			} else {
				if (desc.idVendor == vendor
				    && desc.idProduct == product)
					ret |= read_one_sensor (dev);
			}
		}
		
		libusb_free_device_list (devs, 1);
	}

	return ret;
}
 
int
main (int argc, char *argv[])
{
	int ret;

        if (argc > 1) {                                                                                     fprintf (stderr, "Usage: %s - prints current air quality\n", argv[0]);
                exit (EXIT_FAILURE);                                                                }

	ret = libusb_init (NULL);
	if (ret) {
		fprintf (stderr, "Failed to libusb_init()\n");
		exit (EXIT_FAILURE);
	}

	ret = find_devices (USB_VENDOR_CO2_STICK, USB_PRODUCT_CO2_STICK);

	libusb_exit (NULL);

	exit (ret? EXIT_FAILURE: EXIT_SUCCESS);
}
