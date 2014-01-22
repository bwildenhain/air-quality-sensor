#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <libusb.h>

#define DRIVER_NAME_LEN	1024
#define USB_BUF_LEN	1024

#define AQ_GREEN	1000
#define AQ_YELLOW	1500
#define AQ_RED		4000
#define AQ_BOOTUP	450

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
	if (value == AQ_BOOTUP)
		colour = BLINK;
	else if (value <= AQ_GREEN)
		colour = GREEN;
	else if (value <= AQ_YELLOW)
		colour = YELLOW;
	else
		colour = RED;

	printf ("Device %d/%d/%d, value = %u, quality = %s\n",
		libusb_get_bus_number (dev),
		libusb_get_port_number (dev),
		libusb_get_device_address (dev),
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

	ret = libusb_init (NULL);
	if (ret) {
		fprintf (stderr, "Failed to libusb_init()\n");
		exit (EXIT_FAILURE);
	}

	ret = find_devices (USB_VENDOR_CO2_STICK, USB_PRODUCT_CO2_STICK);

	libusb_exit (NULL);

	exit (ret? EXIT_FAILURE: EXIT_SUCCESS);
}
