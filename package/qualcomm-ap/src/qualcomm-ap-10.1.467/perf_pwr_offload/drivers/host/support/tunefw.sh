#!/bin/bash

# Copyright 2006 Atheros Communications, Inc.

# This script allows a user to view or alter tunable parameters
# in the target firmware operating system.  This is most useful
# as part of the board initialization process during manufacture.
#
# All tunable parameters are 4-byte global variables.  This script
# uses "nm" on the firmware's ELF executable file to determine a
# tunable parameter's address.
#
# Then it uses "objdump -h" to determine the file offset of the
# tunable in the firmware's binary file.  It prints the old value
# and substitutes the desired new value in the binary file.  It
# does not modify the ELF executable in any way.
#
# This script has no knowledge of particular parameters so no
# sanity checks on parameter values are performed.
#
# Example tunable parameters:
#   serial_enable
#   cpu_clock_setting
#   bank0_config_value
#   flash_dataset_index_addr
#   target_software_id
#
# As a preferable alternative, target firmware allows most
# tunable parameters to be written from the Host during startup.

Usage()
{
        THIS_CMD=`$BASENAME_CMD $0`
        echo
        echo "$THIS_CMD reads and/or changes tunable parameters in the firmware image."
        echo
        echo "After tuning the flash image, firmware must be flashed"
        echo "in order for changes to take effect."
        echo
        echo "After tuning the RAM image, firmware must loaded"
        echo "in order for changes to take effect."
        echo
        echo "Usage: '$THIS_CMD [options] parameter_name [new_value]'"
        echo
        echo "Options:"
        echo "  --bin=filename          Specify the name of a binary file"
        echo "                          such as ecos.flash.bin or ecos.ram.bin"
        echo
        echo "  --image=filename        Specify the name of an object image file"
        echo "                          such as ecos.flash.out or ecos.ram.out"
        echo
        echo "  --force                 Force $THIS_CMD to make changes even if"
        echo "                          image and binary filenames do not correspond"
        echo
        echo "  --help                  See this help listing"
        echo
        echo "Examples:"
        echo "$THIS_CMD --image=image/ecos.ram.out --bin=bin/ecos.ram.bin cpu_clock_setting 0x203"
        echo "$THIS_CMD bank0_config_value"
        exit 1
} 

GETOPT_CMD=${GETOPT_CMD:-getopt}
BASENAME_CMD=${BASENAME_CMD:-basename}

if [ -z "$NM_CMD" ]
then
        which mipsisa32-elf-nm > /dev/null 2>&1
        if [ $? -eq 0 ]
        then
                NM_CMD=mipsisa32-elf-nm
        else
                NM_CMD=nm
        fi
fi

# echo "NM_CMD is $NM_CMD" # debug

if [ -z "$OBJDUMP_CMD" ]
then
        which mipsisa32-elf-objdump > /dev/null 2>&1
        if [ $? -eq 0 ]
        then
                OBJDUMP_CMD=mipsisa32-elf-objdump
        else
                OBJDUMP_CMD=objdump
        fi
fi

# echo "OBJDUMP_CMD is $OBJDUMP_CMD" # debug


if [ "$#" -eq 0 ]
then
    set -- --help
fi

# echo "Argument list: $*" # debug

args=`$GETOPT_CMD -l bin::,image::,force,help "b:i:fh" "$@"`
if [ $? -ne 0 ]
then
    Usage
fi
set -- $args

# echo "Argument list after getopt is: $*" # debug

MORE=TRUE
while [ "$MORE" ]
do
        case "$1" in
        -i | --image)
                shift
                eval ELF_IMAGE="$1"
                ;;
        -b | --bin)
                shift
                eval BIN_IMAGE="$1"
                ;;
        -f | --force)
                eval FORCE="TRUE"
                ;;
        -h | --help)
                Usage
                ;;
        --)
                MORE=""
                ;;
        esac
        shift
done

# echo "FORCE is $FORCE" # debug

if [ -z "$ELF_IMAGE" -a -z "$BIN_IMAGE" ]
then
        if [ -z "$WORKAREA" ]
        then
                echo "Please set your WORKAREA environment variable."
                exit 1
        fi
fi

# echo "WORKAREA is $WORKAREA" # debug

ELF_IMAGE=${ELF_IMAGE:-$WORKAREA/target/AR6001/image/ecos.flash.out}
BIN_IMAGE=${BIN_IMAGE:-$WORKAREA/target/AR6001/bin/ecos.flash.bin}

# echo "ELF_IMAGE is $ELF_IMAGE" # debug
# echo "BIN_IMAGE is $BIN_IMAGE" # debug

if [ -z "$FORCE" -a \( `$BASENAME_CMD "$ELF_IMAGE" .out` != `$BASENAME_CMD "$BIN_IMAGE" .bin` \) ]
then
        echo "Unusual filenames.  If filenames are correct, run again with --force."
        echo "Object file: $ELF_IMAGE"
        echo "Binary file: $BIN_IMAGE"
        Usage
fi

if [ ! -r "$ELF_IMAGE" ]
then
        echo "ERROR: Cannot read ELF firmware image $ELF_IMAGE"
        Usage
fi

if [ ! -r "$BIN_IMAGE" ]
then
        echo "ERROR: Cannot read binary firmware image $BIN_IMAGE"
        Usage
fi

NEW_BIN_IMAGE=$BIN_IMAGE.$$

# echo "Number of args remaining is $#: $*" # debug

if [ -z "$ADDR" ]
then
        if [ "$#" -lt 1 ]
        then
                Usage
        fi

        eval PARAM_NAME=$1
        shift
fi

echo "Firmware ELF image: $ELF_IMAGE"
echo "Firmware binary: $BIN_IMAGE"
echo "Parameter name: $PARAM_NAME"

ADDR=${ADDR:-0x`$NM_CMD $ELF_IMAGE | grep $PARAM_NAME | cut -d ' ' -f 1`}
if [ "$ADDR" == "0x" ]
then
        echo "ERROR: Could not find parameter $PARAM_NAME"
        Usage
fi

echo "Address: $ADDR"

DEFAULT_VIRT_START=0x`$NM_CMD $ELF_IMAGE | grep data_start_addr | cut -d ' ' -f 1`
if [ "$DEFAULT_VIRT_START" == "0x" ]
then
        # Treat as OS image
        VIRT_START=${VIRT_START:-0x`$NM_CMD $ELF_IMAGE | grep rom_start | cut -d ' ' -f 1`}
        ALIGN=${ALIGN:-0x40}
else
        VIRT_START=${VIRT_START:-DEFAULT_VIRT_START}
        ALIGN=${ALIGN:-1}
fi

if [ "$VIRT_START" == "0x" ]
then
        echo "Cannot find virtual start address in object file: $ELF_IMAGE"
        Usage
fi
# echo "VIRT_START is: $VIRT_START" # debug

$OBJDUMP_CMD -w -h $ELF_IMAGE | tail +6 |
(
        STOP=""
        FILEOFF=0
        SIZE=0
        while [ -z $STOP ]
        do
                FILEOFF=$((($FILEOFF+$SIZE+$ALIGN-1)&~($ALIGN-1)))
                read INDEX SECTION SIZE VIRT PHYS REMAINDER
                if [ $? -ne 0 ]
                then
                        STOP=TRUE
                else
                        VIRT=0x$VIRT
                        PHYS=0x$PHYS
                        SIZE=0x$SIZE

                        if [ $(($VIRT>$ADDR)) -ne 0 ]
                        then
                                continue
                        fi
                        if [ $(($ADDR>($VIRT+$SIZE))) -ne 0 ]
                        then
                                continue
                        fi

                        BIN_OFFSET=$(($ADDR-$VIRT+$FILEOFF))
                        #BIN_OFFSET=$(($ADDR-$VIRT+($PHYS-$VIRT_START)))
                        # echo "VIRT is: $VIRT" # debug
                        # echo "PHYS is: $PHYS" # debug
                        # echo "FILEOFF is: $FILEOFF" # debug
                        # echo "BIN_OFFSET is: $BIN_OFFSET" # debug
                        # echo "File offset in binary: $BIN_OFFSET"
                        OLD_VALUE=0x`dd if=$BIN_IMAGE bs=1 skip=$BIN_OFFSET count=4 2> /dev/null |
                                od -t x4 |
                                head -1 |
                                cut -d ' ' -f 2`
                        echo "Old value: $OLD_VALUE"
                        if [ $# -eq 1 ]
                        then
                                eval NEW_VALUE=$1
                                (
                                        dd if=$BIN_IMAGE bs=1 count=$BIN_OFFSET 2>/dev/null
                                        NV1=$(($NEW_VALUE & 0xff))
                                        NV2=$((($NEW_VALUE >> 8) & 0xff))
                                        NV3=$((($NEW_VALUE >> 16) & 0xff))
                                        NV4=$((($NEW_VALUE >> 24) & 0xff))
                                        echo -n -e `printf "\\%o\\%o\\%o\\%o" $NV1 $NV2 $NV3 $NV4`
                                        dd if=$BIN_IMAGE bs=1 skip=$(($BIN_OFFSET + 4)) 2>/dev/null
                                ) > $NEW_BIN_IMAGE
                                mv $NEW_BIN_IMAGE $BIN_IMAGE
                                NEW_VALUE=0x`dd if=$BIN_IMAGE bs=1 skip=$BIN_OFFSET count=4 2> /dev/null |
                                        od -t x4 |
                                        head -1 |
                                        cut -d ' ' -f 2`
                                echo "New value: $NEW_VALUE"
                        fi
                        exit 0
                fi
        done
)
