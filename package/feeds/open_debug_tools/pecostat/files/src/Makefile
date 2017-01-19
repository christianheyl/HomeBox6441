
# Written by Zenon Fortuna, zenon@mips.com, zenon@fortuna.org

VERSION=0.9.2

MODDIR=$(LINUX_DIR)

all: pecostat_lkm.ko

ifneq ($(KERNELRELEASE),)

  ifneq ($(CONFIG_MODULES),y)
    $(error Your kernel does not support loadable modules: CONFIG_MODULES undefined)
  endif
  $(warning "Verification: CONFIG_MODULES=y, as required")

  ifeq ($(PERF_IRQ),)
    EXTRA_CFLAGS += -DnoIRQ
  endif

  obj-m += pecostat_lkm.o

else

obj-m += pecostat_lkm.o

pecostat_lkm.ko: Makefile pecostat_lkm.c
	make modules -C $(MODDIR) M=$(SUBDIRS) ARCH=$(ARCH)
	ln -f pecostat_lkm.ko pecostat.ko
endif

pecostat_tool.o: pecostat_tool.c
	$(GCC) $(CFLAGS) -c pecostat_tool.c

pecostat: pecostat_tool.o
	$(GCC) $(CFLAGS) pecostat_tool.o -o pecostat


clean:
	rm -f *.o *.ko pecostat Module.symvers modules.order .*.cmd
	rm -rf .tmp_*
