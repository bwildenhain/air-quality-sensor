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

#include <config.h>

#define DRIVER_NAME_LEN	1024
#define USB_BUF_LEN	1024

#define LEVEL_GREEN	1000
#define LEVEL_YELLOW	1500
#define LEVEL_RED	4000
#define LEVEL_NO_DATA	450

#define USB_VENDOR_CO2_STICK	0x03eb
#define USB_PRODUCT_CO2_STICK	0x2013

#ifdef HAVE_GETOPT_LONG
#include <getopt.h>

static const struct option longopts[] = {
  {"help", 0, NULL, 'h'},
  {"verbose", 0, NULL, 'v'},
  {"version", 0, NULL, 'V'},
  {NULL, 0, NULL, 0}
};
#endif

enum led_state
{
  GREEN = 0,
  YELLOW,
  RED,
  NO_DATA,
};

static const char *const air_quality[] = {
  [GREEN] = "GREEN",
  [YELLOW] = "YELLOW",
  [RED] = "RED",
  [NO_DATA] = "NO_DATA",
};

static int
read_one_sensor (struct libusb_device *dev, int verbose)
{
  int ret;
  uint16_t value;
  int len;
  enum led_state colour;
  struct libusb_device_handle *devh, *devh_parent;
  char driver_name[DRIVER_NAME_LEN] = "";
  unsigned char usb_io_buf[USB_BUF_LEN] = "\x40\x68\x2a\x54"
    "\x52\x0a\x40\x40" "\x40\x40\x40\x40" "\x40\x40\x40\x40";
  struct libusb_device *uplink;

  /* Open USB device.  */
  ret = libusb_open (dev, &devh);
  if (ret)
    {
      fprintf (stderr, "Failed to libusb_open(%p)\n", (void *) dev);
      ret = -1;
      goto out;
    }

  /* Ensure that the device isn't claimed.  */
  if (libusb_kernel_driver_active (devh, 0 /*intrf */ ))
    {
      fprintf (stderr, "Warning: device is claimed by driver %s, "
	       "trying to unbind it.\n", driver_name);
      ret = libusb_detach_kernel_driver (devh, 0 /*intrf */ );
      if (ret)
	{
	  fprintf (stderr, "Failed to detach kernel driver.\n");
	  ret = -2;
	  goto out;
	}
    }

  /* Claim device.  */
  ret = libusb_claim_interface (devh, 0 /*intrf */ );
  if (ret)
    {
      fprintf (stderr, "usb_claim_interface() failed with error %d=%s\n",
	       ret, strerror (-ret));
      ret = -3;
      goto out;
    }

  /* Send query command.  */
  ret = libusb_interrupt_transfer (devh, 0x0002 /*endpoint */ ,
				   usb_io_buf, 0x10 /*len */ , &len,
				   1000 /*msec */ );
  if (ret < 0)
    {
      fprintf (stderr,
	       "Failed to usb_interrupt_write() the initial buffer, ret = %d\n",
	       ret);
      ret = -4;
      goto out_unlock;
    }

  /* Read answer.  */
  ret = libusb_interrupt_transfer (devh, 0x0081 /*endpoint */ ,
				   usb_io_buf, 0x10 /*len */ , &len,
				   1000 /*msec */ );
  if (ret < 0)
    {
      fprintf (stderr, "Failed to usb_interrupt_read() #1\n");
      ret = -5;
      goto out_unlock;
    }

  /* First read returned 0 bytes, do again.  */
  if (ret == 0)
    {
      ret = libusb_interrupt_transfer (devh, 0x0081 /*endpoint */ ,
				       usb_io_buf, 0x10 /*len */ , &len,
				       1000 /*msec */ );
    }

  /* Prepare value from first read.  */
  value = ((unsigned char *) usb_io_buf)[3] << 8
    | ((unsigned char *) usb_io_buf)[2] << 0;

  /* Classify `value'.  */
  if (value == LEVEL_NO_DATA)
    colour = NO_DATA;
  else if (value <= LEVEL_GREEN)
    colour = GREEN;
  else if (value <= LEVEL_YELLOW)
    colour = YELLOW;
  else
    colour = RED;

  /* Output values.  */
#ifndef LIBUSB_OLD
/* libusb_get_parent and libusb_get_port_number have been introduced in
 * https://github.com/libusb/libusb/blob/cfb8610242394d532778a483570089c2bed52c84/libusb/libusb.h
 * Everything from before is considered "old" in config.h
*/


  if (verbose)
    {
      uplink = libusb_get_parent (dev);
      struct libusb_device_descriptor desc;
      libusb_get_device_descriptor (uplink, &desc);

      unsigned char strDesc[256];

      ret = libusb_open (uplink, &devh_parent);
      if (ret)
	{
	  fprintf (stderr, "Failed to libusb_open(uplink, ...)\n");
	  goto out;
	}

      if (desc.iSerialNumber > 0)
	{
	  ret = libusb_get_string_descriptor_ascii
	    (devh_parent, desc.iSerialNumber, strDesc, 256);

	  if (ret < 0)
	    goto out;

	  printf ("Serial %s ", strDesc);
	}
    }

  printf ("Device ");
  int i = 0;
  for (uplink = dev; uplink; uplink = libusb_get_parent (uplink), i++)
    {
      if (i)
	printf (":");
      printf ("%d", libusb_get_port_number (uplink));
    }
#else
  printf ("Bus %03d ", libusb_get_bus_number (dev));
  printf ("Device %03d", libusb_get_device_address (dev));
#endif
  printf (" value = %u, quality = %s\n",
	  (unsigned int) value, air_quality[colour]);

out_unlock:
  ret = libusb_release_interface (devh, 0 /*intrf */ );
  if (ret)
    {
      fprintf (stderr, "Failed to usb_release_interface()\n");
      ret = -5;
    }
out:
  return ret;
}

static int
find_devices (int vendor, int product, int verbose)
{
  int ret = 0, ret2;
  struct libusb_device **devs;
  struct libusb_device *dev;
  struct libusb_device_descriptor desc;
  ssize_t num_devices;
  int i = 0;

  num_devices = libusb_get_device_list (NULL, &devs);
  if (num_devices > 0)
    {
      while ((dev = devs[i++]) != NULL)
	{
	  ret2 = libusb_get_device_descriptor (dev, &desc);
	  if (ret2)
	    {
	      ret |= ret2;
	    }
	  else
	    {
	      if (desc.idVendor == vendor && desc.idProduct == product)
		ret |= read_one_sensor (dev, verbose);
	    }
	}

      libusb_free_device_list (devs, 1);
    }

  return ret;
}

void
print_help ()
{
  printf (("Usage: %s [OPTION]\n\
Read the current air quality from a compatible attached sensor\n\n\
	-h, --help          display this help and exit\n\
        -v, --verbose	    be verbose\n\n\
        -V, --version       display version information and exit\n\n\
Report bugs on <https://github.com/bwildenhain/air-quality-sensor/issues> or send a mail to air-quality-sensor@benedikt-wildenhain.de\n\
Home page: <https://github.com/bwildenhain/air-quality-sensor>\n\
"), PACKAGE_NAME);
}

void
print_version ()
{
  printf ("%s (%s) %s\n", PACKAGE, PACKAGE_NAME, VERSION);

  printf (("\
Copyright (C) 2014 Jan-Benedict Glaw\n\
Copyright (C) %s Benedikt Wildenhain\n\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n"), COPYRIGHT_YEAR);
}

int
main (int argc, char *argv[])
{
  int ret, verbose = 0;


#ifdef HAVE_GETOPT_LONG
  char c;
  int option_index = 0;
  while ((c = getopt_long (argc, argv, "hVv", longopts, &option_index)) != -1)
    {
      switch (c)
	{
	case 'h':
	  print_help ();
	  exit (EXIT_SUCCESS);
	  break;
	case 'V':
	  print_version ();
	  exit (EXIT_SUCCESS);
	  break;
	case 'v':
	  verbose = 1;
	  break;
	case '?':
	  print_help ();
	  exit (EXIT_FAILURE);
	  break;
	}
    }
#else
  if (argc > 1)
    {
      fprintf (stderr,
	       "Compiled without GNU getopt, cannot handle any command line arguments");
      print_version ();
      exit (EXIT_FAILURE);
    }

#endif

  ret = libusb_init (NULL);
  if (ret)
    {
      fprintf (stderr, "Failed to libusb_init()\n");
      exit (EXIT_FAILURE);
    }

  ret = find_devices (USB_VENDOR_CO2_STICK, USB_PRODUCT_CO2_STICK, verbose);

  libusb_exit (NULL);

  exit (ret ? EXIT_FAILURE : EXIT_SUCCESS);
}
