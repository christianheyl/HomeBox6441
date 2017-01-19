#!/bin/sh
#
# Platform-Dependent load script for x86 systems (PCI/PCIe)
# 
#


CFG80211_AVAIL="no"
CFG80211_LOADED="no"


modprobe -l cfg80211 | grep cfg80211 > /dev/null
if [ $? -eq 0 ]; then
# cfg80211 kernel module is available
CFG80211_AVAIL="yes"	
fi

lsmod | grep cfg80211 > /dev/null   
if [ $? -eq 0 ]; then
# cfg80211 kernel module is loaded
CFG80211_LOADED="yes"	
fi

case $1 in
	loadbus)
	
	# nothing to do for PCI

	;;
	unloadbus)
	
	# nothing to do for PCI
	
	;;
	loadAR6K)
	if [ "$CFG80211_AVAIL" = "yes" ]; then
   	   if [ "$CFG80211_LOADED" = "no" ]; then
               modprobe -q cfg80211
               if [ $? -ne 0 ]; then
                  echo "*** Failed to install cfg80211 kernel module"
               fi
   	    fi
   	fi
   	
	echo "loading AR6K module... Args = ($AR6K_MODULE_ARGS) , logfile:$AR6K_TGT_LOGFILE"
	$IMAGEPATH/recEvent --logfile=$AR6K_TGT_LOGFILE --srcdir=$WORKAREA/include/ /dev/null 2>&1 &
    
    /sbin/insmod $IMAGEPATH/$AR6K_MODULE_NAME.ko $AR6K_MODULE_ARGS htc_credit_flow=1
    if [ $? -ne 0 ]; then
        echo "*** Failed to install AR6K Module"
        exit -1
    fi
	;;
	unloadAR6K)
	echo "unloading AR6K module..."
	/sbin/rmmod -w $AR6K_MODULE_NAME.ko
	killall recEvent
	;;
	*)
		echo "Unknown option : $1"
	
esac



