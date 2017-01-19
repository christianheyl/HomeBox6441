#!/bin/sh

bindir=/opt/lantiq/bin

appl_file_list="gpe_table_tests gpon_dti_agent onu optic fapi gtop omci_simulate omcid otop ocal gexdump omci_usock_server"

for I in $appl_file_list; do
    if [ -e $bindir/$I ]; then
	vers=`$bindir/what.sh $bindir/$I`
	if [ "$vers" != "" ]; then
	    echo "$vers"
	fi
    fi
done
