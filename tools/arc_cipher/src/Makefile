##### This makefile for arc_ciph
##### for kernel system image encryption
EXEC_FILE?= arc_cipher
INSTALL_PATH=/opt/hostutils/
CFLAGS = -g -Os -Wall -O2 -ggdb -lm -D__mips__ -DCONFIG_HDR_MKIMG
LDFLAGS = -lm
#-s -Os

CC  := gcc
LD	:= ld
AR	:= ar

SRCS:=$(wildcard *.c)

OBJS:=$(subst .c,.o,$(SRCS))


all: $(EXEC_FILE)

$(EXEC_FILE): $(OBJS)
	$(CC) ${CFLAGS} $^ ${LDFLAGS} -o $@


clean:
	rm -rf *.o
	rm -rf *.bak
	rm -rf $(EXEC_FILE)
	rm -rf *.d

install	: all
	mv -f $(EXEC_FILE) $(INSTALL_PATH)

include $(subst .c,.d,$(SRCS))

%.d: %.c
	$(CC) -M $(CFLAGS) $< >$@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@: ,g' < $@.$$$$ >$@; \
	rm -f $@.$$$$
