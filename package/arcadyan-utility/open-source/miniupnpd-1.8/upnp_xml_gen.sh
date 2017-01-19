#!/bin/bash

# linghong_tan 2015.03.19
# Generate upnpdescstrings.h from glbcfg. The upnpdescstrings.h would be used for XML. 

if [ 'x$1' == 'x' -o 'x$2' == 'x' ]; then
	exit 1
fi

dos2unix -n $1 $1".tmp"
if [ $? != 0 ]; then
	exit 1
fi

output_file="upnpdescstrings.h"

echo "/* $Id: upnpdescstrings.h,v 1.8 2012/09/27 16:00:10 nanard Exp $ */" > $output_file
echo "/* miniupnp project " >> $output_file
echo "* http://miniupnp.free.fr/ or http://miniupnp.tuxfamily.org/" >> $output_file
echo "* (c) 2006-2012 Thomas Bernard" >> $output_file
echo "* This software is subject to the coditions detailed in" >> $output_file
echo "* the LICENCE file provided within the distribution */" >> $output_file
echo "#ifndef UPNPDESCSTRINGS_H_INCLUDED" >> $output_file
echo "#define UPNPDESCSTRINGS_H_INCLUDED" >> $output_file
echo "" >> $output_file
echo "#include \"config.h\"" >> $output_file

conffile=$1".tmp"
echo $conffile

#if [ -f 'upnpdescstrings.h' ];
	#exit 1
#fi

FRIENDLYNAME=`cat $conffile | grep 'ARC_UPNP_DeviceName=' |awk -F= '{print $2;}'`
MANUFACTURER=`cat $conffile | grep 'ARC_UPNP_Manufacturer=' |awk -F= '{print $2;}'`
MANUFACTURERURL=`cat $conffile | grep 'ARC_UPNP_ManufacturerURL=' |awk -F= '{print $2;}'`
MODELNAME=`cat $conffile | grep 'ARC_UPNP_ModelName=' |awk -F= '{print $2;}'`
MODELDESCRIPTION=`cat $conffile | grep 'ARC_UPNP_ModelDescription=' |awk -F= '{print $2;}'`
MODELURL=`cat $conffile | grep 'ARC_UPNP_ModelURL=' |awk -F= '{print $2;}'`
MODELNUMBER=`cat $conffile | grep 'ARC_SYS_FWVersion=' |awk -F= '{print $2;}'`
UPC="000000000000"

#r_prefix="ROOTDEV_"
for prefix in ROOTDEV_ WANDEV_ WANCDEV_
do
	echo "" >> $output_file

	#debug
	echo ""
	echo "prefix="$prefix

	for subfield in FRIENDLYNAME MANUFACTURER MANUFACTURERURL MODELNAME MODELDESCRIPTION MODELURL MODELNUMBER UPC
	do
		eval sf=\$$subfield

		#debug
		echo "subfield="$subfield
		echo "sf="$sf

		if [ $subfield == 'FRIENDLYNAME' ]; then
			if [ $prefix == 'WANDEV_' ]; then
				sf=$sf"(WANDevice)"
			elif [ $prefix == 'WANCDEV_' ]; then
				sf=$sf"(WAN Connection Device)"
			fi
		elif [ $subfield == 'MODELNAME' ]; then
			if [ $prefix == 'WANDEV_' ]; then
				sf=$sf" WAN Interface"
			elif [ $prefix == 'WANCDEV_' ]; then
				sf=$sf" WAN Connector"
			fi
		elif [ $subfield == 'MODELDESCRIPTION' ]; then
			if [ $prefix == 'WANDEV_' ]; then
				sf=$sf" (WAN Interface Device)"
			elif [ $prefix == 'WANCDEV_' ]; then
				sf=$sf" (WAN Connection Device)"
			fi
		fi

		echo "#define $prefix$subfield				\"${sf}\"" >> $output_file
	done
done


echo "#endif" >> "upnpdescstrings.h"

rm $conffile
