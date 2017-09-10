bin_PROGRAMS = air-quality-sensor

air_quality_sensor_CFLAGS = $(LIBUSB1_0_CFLAGS)
air_quality_sensor_LDADD = $(LIBUSB1_0_LIBS)
air_quality_sensor_SOURCES = air10.c

# manpages
man_MANS = air-quality-sensor.1

# Appdata files

metainfodir = $(datadir)/metainfo
metainfo_DATA = de.benedikt-wildenhain.air-quality-sensor.metainfo.xml

EXTRA_DIST = $(man_MANS) manpage.include $(metainfo_DATA)

man_include = $(srcdir)/manpage.include

air-quality-sensor.1: $(air_quality_sensor_SOURCES) $(man_include)
	$(HELP2MAN) --no-info --include $(man_include) --name="User space driver for AppliedSensor's Indoor Air Monitor" ./$(bin_PROGRAMS) -o $@
