CC := gcc
KERNEL := $(shell uname -r)
KERNELHEADERS := /lib/modules/$(KERNEL)/build/include
ASMHEADERS := /lib/modules/$(KERNEL)/build/arch/x86/include/
all:
	$(CC) -Wall -I$(KERNELHEADERS) -I$(ASMHEADERS) core_file_gen.c -o core_file_gen
	$(CC) -Wall format_byte_dump.c -o format_byte_dump
