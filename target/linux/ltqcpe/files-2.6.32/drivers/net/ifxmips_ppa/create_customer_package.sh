#!/bin/bash

if [ ! -n "$1" ] ; then
	echo "Usage : create_customer_package.sh SOURCE_PATH"
	exit 0
fi

####################Remove files###################

rm -rf ppa_api/ppa_api_sw_accel.c

####################Add Objs###################
mkdir -p ppa_api/

cp -f $1/drivers/net/lantiq_ppa/ppa_api/ppa_api_sw_accel.o ppa_api/ppa_api_sw_accel.o

rm -rf create_customer_package.sh

