#
# Makefile for pmon
#

CFLAGS += -O2 -I$(KERNEL_INCLUDE_PATH) -DCONFIG_$(PLATFORM)

all: pmon

ppacmd: pmon.o

clean:
	rm -f *.o pmon
