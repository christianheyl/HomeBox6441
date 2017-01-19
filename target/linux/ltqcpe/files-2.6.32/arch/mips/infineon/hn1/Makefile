#
# Makefile for the platform specific kernel interface routines under Linux.
#

all: ifxmips_hn1.a

obj-y   := hn1.o devices.o ifxmips_clk.o
obj-$(CONFIG_HN1_EVAL_BOARD) += hn1_eval_board.o
obj-$(CONFIG_HN1_REF_BOARD) += hn1_ref_board.o
obj-$(CONFIG_IFX_CLOCK_CHANGE) += ifxmips_fill_icache.o

EXTRA_AFLAGS := $(CFLAGS)

clean:
	rm -f *.o *.a

