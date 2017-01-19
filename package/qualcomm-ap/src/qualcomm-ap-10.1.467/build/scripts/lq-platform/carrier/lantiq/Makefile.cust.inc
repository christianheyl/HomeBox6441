#
# Makefile for building the Atheros wlan driver with customer specific setting
#
COPTS+= -I${KERNELPATH}/include/asm-mips/ifx

ifeq ($(strip ${PLATFORM_BYTE_SWAP}),1)
COPTS += -DPLATFORM_BYTE_SWAP
endif