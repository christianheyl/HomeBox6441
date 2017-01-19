#!/bin/sh

#
# This file is added for regDomain patch test purpose.
#

usage() {
	echo "Usage: `basename $0` [-r|--romonly]"
	exit 1
}

create_diffile() {
	if [ ! -r "$OLDFILE" ]; then
		echo "Original regDomain binary file, '$OLDFILE', not found."
		exit 1
	fi
	if [ ! -r "$NEWFILE" ]; then
		echo "New regDomain binary file, '$NEWFILE', not found."
		exit 1
	fi
	#use BDIFF to create a diff file
	$BDIFF -nooldmd5 -nonewmd5 -d $OLDFILE $NEWFILE $DIFFILE
}

create_img() {
	if [ ! -r "$DSETSCRIPT" ]; then
		echo "Dset script file, '$DSETSCRIPT', not found."
		exit 1
	fi
	echo "Create dset image '$DSETOUTPUT'."
	$MKDSETIMG --desc=$DSETSCRIPT --out=$DSETOUTPUT --idxaddr=$IDXADDR --verbose >$DSETINFO
	PATCHADDR=`$CAT $DSETINFO | $GREP "Data starts" | $CUT -d ' ' -f 4`
	INDEXADDR=`$CAT $DSETINFO | $GREP "Index is" | $CUT -d ' ' -f 4`
	echo "PATCHADDR=$PATCHADDR, INDEXADDR=$INDEXADDR"
}

get_ramidx() {
	if [ ! -r "$FLASHIMG" ]; then
		echo "Flash image file, '$FLASHIMG', not found."
		exit 1
	fi
	RAMIDX=`$NM $FLASHIMG | $GREP "dset_RAM_index" | $CUT -d ' ' -f 1`
	echo "RAMIDX=$RAMIDX"
}

load_bmi() {
	echo "Load bmi"
	$LOADAR6000 bmi
	if [ ! -z "$ROMONLY" ]; then
		$EEPROM --transfer
	fi
}

load_unloadall() {
	$LOADAR6000 unloadall
}

load_patch() {
	echo "Load patch to $PATCHADDR"
	$BMILOADER -i $NETIF --write --address=$PATCHADDR --file=$DSETOUTPUT
}

write_index() {
	echo "Write index address $INDEXADDR to 0x$RAMIDX."
	$BMILOADER -i $NETIF --write --address=0x$RAMIDX --param=$INDEXADDR
}

load_done() {
	$BMILOADER --done
}

device_exist() {
	$IWCONFIG | $GREP $NETIF >$DEVNULL
}

check_result() {
	$IWLIST $NETIF frequency | $GREP "3 channels" >$DEVNULL
}

clear_tmpfile() {
	$RM -f $DIFFILE
	$RM -f $DSETOUTPUT
	$RM -f $DSETINFO
}

NM=${NM:-mipsisa32-elf-nm}
RM=${RM:-rm}
CAT=${CAT:-cat}
GREP=${GREP:-grep}
CUT=${cut:-cut}
IFCONFIG=${IFCONFIG:-ifconfig}
IWCONFIG=${IWCONFIG:-iwconfig}
IWLIST=${IWLIST:-iwlist}
NETIF=${NETIF:-eth1}
DEVNULL=${DEVNULL:-/dev/null}
BDIFF=${BDIFF:-$WORKAREA/host/.output/$ATH_PLATFORM/image/bdiff}
BMILOADER=${BMILOADER:-$WORKAREA/host/.output/$ATH_PLATFORM/image/bmiloader}
EEPROM=${EEPROM:-$WORKAREA/host/.output/$ATH_PLATFORM/image/eeprom}
MKDSETIMG=${MKDSETIMG:-$WORKAREA/host/.output/$ATH_PLATFORM/image/mkdsetimg}
LOADAR6000=${LOADAR6000:-$WORKAREA/host/support/loadAR6000.sh}

OLDFILE=${OLDFILE:-$WORKAREA/host/.output/bin/regulatoryData_AG.bin}
NEWFILE=${NEWFILE:-$WORKAREA/host/.output/bin/regulatoryData_AG.diff}
DIFFILE=${DIFFILE:-/tmp/diffile}

DSETSCRIPT=${DSETSCRIPT:-$WORKAREA/host/tests/dsetpatch/debug-dsets.txt}
DSETOUTPUT=${DSETOUTPUT:-/tmp/dsetpatch.$$}
DSETINFO=${DSETINFO:-/tmp/dsetinfo.$$}

FLASHIMG=${FLASHIMG:-$WORKAREA/target/AR6001/image/ecos.flash.out}

#
#
#

RAMIDX=""
INDEXADDR=""
PATCHADDR=""
ROMONLY=""
IDXADDR=${IDXADDR:-0x80013ffc}

while [ $# -ne 0 ]
do
	case $1 in
	-r | --romonly)
		ROMONLY="yes"
		shift
		;;
	*)
		usage
		;;
	esac
done

#Verify environment variables
if [ -z "$WORKAREA" ]
then
        echo "Please set your WORKAREA environment variable."
        exit 1
fi

if [ -z "$ATH_PLATFORM" ]
then
        echo "Please set your ATH_PLATFORM environment variable."
        exit 1
fi

if [ ! -x "$BMILOADER" ]; then
        echo "Loader application, '$BMILOADER', not found."
        exit 1
fi

if [ ! -x "$BDIFF" ]; then
	echo "Diff application, '$BDIFF', not found."
	exit 1
fi

if [ ! -x "$MKDSETIMG" ]; then
	echo "Make dset image application, '$MKDSETIMG', not found."
	exit 1
fi

#If drivers have been loaded, unloadall
if device_exist; then
	$IFCONFIG $NETIF down
	load_unloadall
fi

if load_bmi; then
	create_diffile
	create_img
	get_ramidx
	load_patch
	write_index
	load_done
	clear_tmpfile
	if check_result; then
		load_unloadall
		echo "RegDomain patch verified"
		exit 0
	else
		load_unloadall
		echo "RegDomain patch failed"
		exit 1
	fi
else
	load_unloadall
	exit 1
fi
