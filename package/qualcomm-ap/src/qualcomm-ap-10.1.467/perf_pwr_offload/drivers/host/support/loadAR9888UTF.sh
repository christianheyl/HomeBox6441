#!/bin/bash

# Under construction.
#
# EXAMPLE script to load and initialize using bmiloader, a BMI user agent.
# NB: Similar commands could also be used with udev.

BMILOADER=${BMILOADER:-/usr/local/bin/bmiloader}

RAMFW=$(echo ${RAMFW:-/lib/firmware/utf.bin})

BDDATA=${BDDATA:-/lib/firmware/fakeBoardData_AR6004.bin}

echo 1 > /sys/bus/pci/rescan
insmod /usr/local/lib/adf.ko
insmod /usr/local/lib/asf.ko
insmod /usr/local/lib/ath_hal.ko
insmod /usr/local/lib/ath_rate_atheros.ko
insmod /usr/local/lib/ath_dev.ko
insmod /usr/local/lib/umac.ko bmi=1 &

$BMILOADER --wait # Wait indefinitly for Target
$BMILOADER --set --address=0x4110 --param=1 # SOC_CORE_CLK_CTRL
$BMILOADER --set --address=0x4030 --param=0 # disable WDT
$BMILOADER --set --address=0x4040 --param=1
$BMILOADER --set --address=0x9024 --or=4    # nowdt

$BMILOADER --set --address=0x502c --param=1 # disable system sleep
$BMILOADER --set --address=0x9024 --or=8    # nosleep

$BMILOADER --set --address=0x403924 --param=1 # skip CPU PLL init
$BMILOADER --set --address=0x403928 --param=1 # skip BB PLL init

$BMILOADER --set --address=0x400880 --param=7          # hi_dbg_uart_txpin
$BMILOADER --set --address=0x9024 --or=2               # enable serial output

$BMILOADER --set --address=0x400800 --param=2          # hi_app_host_interest=HTC_PROTOCOL_VERSION/2
$BMILOADER --set --address=0x400810 --or=0x2208        # hi_option_flag
$BMILOADER --set --address=0x400844 --param=0          # hi_be

BDDATA_ADDR=$($BMILOADER --get --quiet --address=0x400854) # hi_board_data
$BMILOADER --write --address=$BDDATA_ADDR --file=$BDDATA
$BMILOADER --set --address=0x400858 --param=1          # hi_board_data_initialized=1

$BMILOADER --write --file=$RAMFW

if [ "1" ]; then
# Enable CPU PLL at 220MHz, core_clk@110MHz
$BMILOADER --set --address=0x4110 --param=1 # SOC_CORE_CLK_CTRL
$BMILOADER --set --address=0x5014 --or=0x10000
sleep 1
$BMILOADER --set --address=0x4020 --param=3 # CPU_CLOCK.STANDARD
$BMILOADER --set --address=0x42f0 --or=0x40 # CPU_PLL_CONFIG
$BMILOADER --set --address=0x42f0 --param=0x0016141 # CPU_PLL_CONFIG
$BMILOADER --set --address=0x42f0 --param=0x0016101 # CPU_PLL_CONFIG
sleep 1
$BMILOADER --set --address=0x5014 --and=0xfffeffff
sleep 1
$BMILOADER --set --address=0x4020 --param=2 # CPU_CLOCK.STANDARD
$BMILOADER --set --address=0x408240 --param=1 # cmnos_core_clk_div
$BMILOADER --set --address=0x403914 --param=220000000 # cmnos_cpu_speed
$BMILOADER --set --address=0x403910 --param=1 # cmnos_cpu_pll_init_done
fi

$BMILOADER --done

