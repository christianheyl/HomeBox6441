#!/bin/bash

if [ "_$1" = "_" ]; then
   WORKAREA=`pwd`
else
   WORKAREA=$1
fi

if [ -d "$WORKAREA" ]
then
    cd $WORKAREA
else
    echo "Dir \"$WORKAREA\" does not exist."
    exit 1
fi

# Peregrine Tensilica setup
source /trees/sw/peregrine/build/RD11.sh

# Silicon firmware env variables
export WORKAREA=`pwd`
export TARGET=AR9888
export TARGET_VER=1
export TARGET_REV=1
export FPGA_BB=1
export FPGA_FLAG=0
# Temp till its use is removed from firmware sources
export PEREGRINE_COMPUTEX=1

# Make Silicon firmware
cd target
make
if [ $? -ne 0 ]         # Test exit status of make
then
  echo "make firmware failed!"
  exit 1
fi

# Convert athwlan.bin to athwlan_bin.h
FWBIN=$WORKAREA/target/.output/AR9888/hw.$TARGET_VER/bin/athwlan.bin
rm -f /tmp/athwlan_bin.h
perl $WORKAREA/host/os/darwin/tools/bin2hex.txt $FWBIN 1 1 > /tmp/athwlan_bin.h
