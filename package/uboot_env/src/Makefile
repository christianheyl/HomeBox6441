LIBPATH=-L.

TARGET = uboot_env

COMMON_SRCS = command.c upgrade_core.c flash.c crc32.c nandwrite.c
COMMON_OBJS = command.o upgrade_core.o flash.o crc32.o nandwrite.o

%.o: %.c
	$(CC) $(CFLAGS) -I. -c $< -o $@

%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@
	$(CC) -MM $(CFLAGS) $< | sed s/\\.o/.d/ >> $@


uboot_env : uboot_env.o $(COMMON_SRCS)
	which $(CC)
	$(CC) $(CFLAGS) -o $@ $^
	$(STRIP) $@

distclean: clean

clean:
	-rm -rf *.o *.d $(TARGET)

