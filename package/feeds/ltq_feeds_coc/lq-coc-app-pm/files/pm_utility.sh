#!/bin/sh /etc/rc.common
START=78

start() {
	if [ A"$CONFIG_PACKAGE_LQ_COC_APP_PM" = "A1" ] && 
       [ A"$CONFIG_IFX_ETHSW_API_COC_PMCU" = "A1" ]; then
           echo "set polling mode for Ethernet Switch"
           /opt/lantiq/bin/pmcu_utility -s1 -mswitch -n0&
	fi
}
