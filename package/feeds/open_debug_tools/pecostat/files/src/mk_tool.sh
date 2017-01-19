#!/bin/sh

export PATH=/opt/com_toolchains/backfire/64bit_toolchain/toolchain-mips_r2_gcc-4.3.3+cs_uClibc-0.9.30.1/usr/bin/
:$PATH

echo 'make -f Makefile_tool'
make -f Makefile_tool
