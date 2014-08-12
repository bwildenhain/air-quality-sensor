AC_INIT([air-quality-sensor], [0.1], [air-quality-sensor@benedikt-wildenhain.de])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC
AM_PROG_CC_C_O
PKG_CHECK_MODULES([LIBUSB1_0], [libusb-1.0])

libusb_is_old=0
AC_CHECK_LIB(usb-1.0, libusb_get_parent, [], [libusb_is_old=1])
AC_CHECK_LIB(usb-1.0, libusb_get_port_number, [], [libusb_is_old=1])

if test $libusb_is_old == 1
then
    AC_DEFINE([LIBUSB_OLD], [yes], [Libusb is old and does not give information about topology of connected devices])
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
PKG_PROG_PKG_CONFIG
