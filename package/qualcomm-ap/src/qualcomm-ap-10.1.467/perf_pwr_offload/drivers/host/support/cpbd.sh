#!/bin/sh
export IMAGEPATH=${IMAGEPATH:-$WORKAREA/host/.output/$ATH_PLATFORM/image}
sudo $WORKAREA/host/support/loadAR6000.sh unloadall
sudo $WORKAREA/host/support/loadAR6000.sh bmi
sudo $IMAGEPATH/bmiloader -r -a 0x207fe00 -l 512 -f bdimg.$$
sudo $IMAGEPATH/eeprom -r -f bdimg.$$.org
sudo diff bdimg.$$ bdimg.$$.org
if [ $? -eq 0 ] ;then
    echo Board Data has resided in EEPROM
else
    sudo $IMAGEPATH/eeprom -w -f bdimg.$$
    sudo $IMAGEPATH/eeprom -r -f bdimg.$$.rb
    sudo diff bdimg.$$ bdimg.$$.rb
    if [ $? -eq 0 ] ;then
       echo Board Data Copied from Flash to EEPROM sucessfully
    else
       echo EEPRON read or write failed
    fi
   
fi
sudo $WORKAREA/host/support/loadAR6000.sh unloadall
