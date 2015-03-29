AC_INIT([air-quality-sensor], [0.1.3], [air-quality-sensor@benedikt-wildenhain.de])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_PROG_CC
AM_PROG_CC_C_O
PKG_CHECK_MODULES([LIBUSB1_0], [libusb-1.0])

AC_CHECK_FUNCS(getopt_long)

dnl Copyright will apply as long as these sources are in use, e.g., are
dnl being compiled, which is reasonable year to claim the copyright.
AC_DEFINE([COPYRIGHT_YEAR], [m4_esyscmd([date +%Y])], [year in copyright message])

libusb_is_old=0
AC_CHECK_LIB(usb-1.0, libusb_get_parent, [], [libusb_is_old=1])
AC_CHECK_LIB(usb-1.0, libusb_get_port_number, [], [libusb_is_old=1])

if test $libusb_is_old == 1
then
    AC_DEFINE([LIBUSB_OLD], [yes], [Libusb is old and does not give information about topology of connected devices])
fi


if test $cross_compiling = no; then
    AM_MISSING_PROG(HELP2MAN, help2man)
else
    HELP2MAN=:
fi

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
PKG_PROG_PKG_CONFIG
