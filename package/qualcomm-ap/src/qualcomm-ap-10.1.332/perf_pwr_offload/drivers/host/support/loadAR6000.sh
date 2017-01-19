#!/bin/bash
# Loads and unloads the driver modules in the proper sequence
Help() {
    echo "Usage: $0 [options]"
    echo
    echo "With NO options, perform a default driver loading"
    echo
    echo "To specify the interface name:"
    echo "   $0 -i <interface name> or --ifname <interface name>"
    echo "To bypass WMI: "
    echo "   $0 1"
    echo "To run Test commands:"
    echo "   $0 -t or $ --test"
    echo "To recover Flash using ROM BSP,ie,force target boot from ROM:"
    echo "   $0 -r or $0 --rom"
    echo "To enable console print:"
    echo "   $0 --enableuartprint"
    echo "To disable console print:"
    echo "   $0 --disableuartprint"
    echo "To enable checksum offload:"
    echo "   $0 --csumoffload"
    echo "To get Dot11 hdrs from target:"
    echo "   $0 --processDot11Hdr"
    echo "To disable getting Dot11 hdrs from target:"
    echo "   $0 --disableprocessdot11hdr"
    echo "To specify the log file name:"
    echo "   $0 -l <log.file> or $0 --log <log.file>"
    echo "To specify the startup mode:"
    echo "   $0 -m <ap|sta|ibss> or $0 --mode <ap|sta|ibss>"
    echo "To specify the startup submode:"
    echo "   $0 -s <none|p2pgo|p2pclient|p2pdev> or $0 --submode <none|p2pgo|p2pclient|p2pdev>"
    echo "To specify a mac address:"
    echo "   $0 --setmac aa:bb:cc:dd:ee:ff"
    echo "To specify a regulatory domain:"
    echo "   $0 --setreg <regcode>"
    echo "To specify USB Audio Class Support"
    echo "   $0 --enableusbaudio"
    echo "To see this help information:"
    echo "   $0 --help or $0 -h "
    echo "To unload all of the drivers:"
    echo "   $0 unloadall"
    exit 0
}
if [ -z "$WORKAREA" ]
then
    echo "Please set your WORKAREA environment variable."
    exit -1
fi
if [ -z "$ATH_PLATFORM" ]
then
    echo "Please set your ATH_PLATFORM environment variable."
    exit -1
fi

fromrom=0
slowbus=${slowbus:-0}
tmode=0
dounload=""
ar6000args=""
ATH_BUS_DRIVER_OPTIONS=${ATH_BUS_DRIVER_OPTIONS}
bmi_enable=""
logfile="/tmp/dbglog.out"
mode=1
NETIF=${NETIF:-eth1}
temp=""
START_WLAN="TRUE"
TARGET_ONLY="FALSE"
ATH_BTHCI_ARGS=${ATH_BTHCI_ARGS:-"ar3khcibaud=3000000 hciuartscale=1 hciuartstep=8937"}
macaddr=""
loadar6konly=""
hif_usbaudioclass=""
board_type=${ATH_BOARDTYPE:-DB132}
refclk_hz=40000000

#
# SPECIAL Atheros SDIO stack modes for FPGA operation
#
# For AR6002_REV4 FPGA 
# Note: Bus clock setting is set "optimistically" at 12.5 Mhz for FPGA testing because
#       some STD host add-in cards report an incorrect SDIO bus clock capability.  
#       For example when 12Mhz is requests we get 6.25 Mhz instead. Requesting 12.5 will
#       actually get us 12Mhz. 
# TODO: For SDIO only?
if [ "$FPGA_FLAG" = "1" ]; then
ATH_BUS_DRIVER_OPTIONS="DefaultOperClock=12500000"
fi
#
# Alternatively:
#     To operate the FPGA at 25 (24 mhz) the Atheros bus driver needs to run in a non-standard bus mode.
#     This mode enables the SD/SDIO-HS (high-speed) clock logic (card and host) and allows operation at 
#     SDIO 1.1 rates with this logic enabled.
#     This is only known to work with the ENE STD host (standard host controller with SD 2.0 support).
# 
# ATH_BUS_DRIVER_OPTIONS="DefaultOperClock=25000000 ConfigFlags=2"
#
	
echo "Platform set to $ATH_PLATFORM"

if [ ! -z "$HOST_PLAT_SETUP_SCRIPT" ]; then
	# user set platform setup script externally
	if [ ! -e "$HOST_PLAT_SETUP_SCRIPT" ]; then		
		echo "Platform Setup Script not found! Script Path : $HOST_PLAT_SETUP_SCRIPT"
		echo "Please check HOST_PLAT_SETUP_SCRIPT env setting"
		exit -1
	fi
else
	# user did not specify one, use the default location...
	HOST_PLAT_SETUP_SCRIPT=$WORKAREA/host/support/platformscripts/plat_$ATH_PLATFORM.sh
	if [ ! -e "$HOST_PLAT_SETUP_SCRIPT" ]; then		
		echo "Platform Setup Script not found! : $HOST_PLAT_SETUP_SCRIPT"
		echo "Please check ATH_PLATFORM env setting"
		exit -1
	fi
fi

if [ ! -x "$HOST_PLAT_SETUP_SCRIPT" ]; then
	echo "Platform Setup Script is not executable! : $HOST_PLAT_SETUP_SCRIPT"
	exit -1
fi

echo "Platform Setup Script is: $HOST_PLAT_SETUP_SCRIPT"

while [ "$#" -ne 0 ]
do
case $1 in
        -i|--ifname)
        NETIF=$2
        shift 2
        ;;
        -t|--test )
        tmode=1
        ar6000args="$ar6000args testmode=1"
        shift
        ;;
        -r|--rom )
        fromrom=1
        slowbus=1
        ar6000args="$ar6000args skipflash=1"
        shift
        ;;
        -l|--log )
        shift
        logfile="$1"
        shift
        ;;
        -m|--mode )
        shift
        ar6000args="$ar6000args devmode=$1"
        echo $ar6000args 
        shift
        ;;
        -s|--submode )
        shift
        ar6000args="$ar6000args submode=$1"
        echo $ar6000args
        shift
        ;;
        --firmware_bridge )
        shift
        ar6000args="$ar6000args firmware_bridge=1"
        shift
        ;;
        --setmac )
        shift
        macaddr="$1"
        shift
        ;;
        --setreg )
        shift
        newreg="$1"
        shift
        ;;
        -h|--help )
        Help
        ;;
        bmi|hostonly|-hostonly|--hostonly)
        bmi_enable="yes"
        shift
        ;;
        bypasswmi)
        ar6000args="$ar6000args bypasswmi=1"
        shift
        ;;
        noresetok)
        ar6000args="$ar6000args resetok=0"
        shift
        ;;
        specialmode)
        ar6000args="$ar6000args bypasswmi=1, resetok=0"
        bmi_enable="yes"
        shift
        ;;
        unloadall)
        dounload="yes"
        shift
        ;;
        enableuartprint)
        ar6000args="$ar6000args enableuartprint=1"
        shift
        ;;
        disableuartprint)
        ar6000args="$ar6000args enableuartprint=0"
        shift
        ;;
        enablerssicompensation)
        ar6000args="$ar6000args enablerssicompensation=1"
        shift
        ;;
        csumoffload)
        ar6000args="$ar6000args csumOffload=1"
        shift
        ;;
        nomsi)
        ar6000args="$ar6000args msienable=0"
        shift
        ;;
        initbrk)
        ar6000args="$ar6000args initbrk=1"
        shift
        ;;
        processdot11hdr)
        ar6000args="$ar6000args processDot11Hdr=1"
        shift
        ;;
        disableprocessdot11hdr)
        ar6000args="$ar6000args processDot11Hdr=0"
        shift
        ;;
        logwmimsgs)
        ar6000args="$ar6000args logWmiRawMsgs=1"
        shift
        ;;
        setuphcipal)
        ar6000args="$ar6000args setuphcipal=1"
        shift
        ;;
        setuphci)
        ar6000args="$ar6000args setuphci=1"
        shift
        ;;
        loghci)
        ar6000args="$ar6000args loghci=1"
        shift
        ;;
        setupbtdev)
        ar6000args="$ar6000args setupbtdev=1 $ATH_BTHCI_ARGS"
        shift
        ;;
        --nostart|-nostart|nostart)
        START_WLAN="FALSE"
        shift
        ;;
        --targonly|-targonly|targonly)
        TARGET_ONLY="TRUE"
        shift
        ;;
        --uartfcpol)
        shift
        uartfcpol="$1"
        shift
        ;;
        ar6konly)
        loadar6konly="yes"
        shift
        ;;
        resettest)
        ar6000args="$ar6000args rtc_reset_only_on_exit=1"
        shift
        ;;
        skipregscan)
        ar6000args="$ar6000args regscanmode=1"
        shift
        ;;
        doinitregscan)
        ar6000args="$ar6000args regscanmode=2"
        shift
        ;;
        --macaddrmode )
        shift
        ar6000args="$ar6000args mac_addr_method=$1"
        echo $ar6000args 
        shift
        ;;
        --enableusbaudio)
        echo "Enable USB Audio Class Support!"
        hif_usbaudioclass="yes"
        shift
        ;;
        --wow_ext)
        wow_ext="1"
        shift
        ;;
        * )
            echo "Unsupported argument"
            exit -1
        shift
    esac
done

ar6000args="$ar6000args ifname=$NETIF"

#case $mode in
#        ap)
#        mode=2
#        ar6000args="$ar6000args fwmode=2"
#        ;;
#        sta)
#        mode=1
#        ar6000args="$ar6000args fwmode=1"
#        ;;
#        ibss)
#        mode=0
#        ar6000args="$ar6000args fwmode=0"
#        ;;
#        * )
#        ar6000args="$ar6000args fwmode=1"
#        ;;
#esac
	
export IMAGEPATH=${IMAGEPATH:-$WORKAREA/host/.output/$ATH_PLATFORM/image}
export AR6K_MODULE_NAME=ar6000

echo "Image path: $IMAGEPATH"
echo "Mac address: $macaddr"
	
# If "targonly" was specified on the command line, then don't
# load (or unload) the host driver.  Skip to target-side setup.
if [ "$TARGET_ONLY" = "FALSE" ]; then # {
	if [ "$dounload" = "yes" ]; then
		echo "..unloading all"
		# call platform script to unload AR6K module
		$HOST_PLAT_SETUP_SCRIPT unloadAR6K
		# call platform script to unload any busdriver support modules
		$HOST_PLAT_SETUP_SCRIPT unloadbus
		exit 0
	fi

	export ATH_SDIO_STACK_PARAMS="RequestListSize=128 $ATH_BUS_DRIVER_OPTIONS"
	export AR6K_MODULE_ARGS="bmienable=1 $ar6000args busspeedlow=$slowbus"
	export AR6K_TGT_LOGFILE=$logfile

	if [ "$loadar6konly" != "yes" ]; then # {
		# call platform script to load any busdriver support modules
		$HOST_PLAT_SETUP_SCRIPT loadbus
		if [ $? -ne 0 ]; then
			echo "Platform script failed : loadbus "
			exit -1
		fi
	fi # }
	# call platform script to load the AR6K kernel module
	$HOST_PLAT_SETUP_SCRIPT loadAR6K
	if [ $? -ne 0 ]; then
		echo "Platform script failed : loadAR6K "
		exit -1
	fi
fi # }

# At this point, all Host-side software is loaded.
# So we turn our attention to Target-side setup.

# If user requested BMI mode, he will handle everything himself.
# Otherwise, we load the Target firmware, load EEPROM and exit
# BMI mode.
exit_value=0
if [ "$bmi_enable" != "yes" ]; then # {

    if [ -e "/sys/module/ar6000/debugbmi" ]; then
        PARAMETERS="/sys/module/ar6000"
    fi

    if [ -e "/sys/module/ar6000/parameters/debugbmi" ]; then
        PARAMETERS="/sys/module/ar6000/parameters"
    fi

    if [ "$PARAMETERS" != "" ]; then
        # For speed, temporarily disable Host-side debugging
        save_dbg=`cat ${PARAMETERS}/debugdriver`
        echo 0 > ${PARAMETERS}/debugdriver
    fi
   
    if [ "$PARAMETERS" != "" ]; then
        if [ "$hif_usbaudioclass" == "yes" ]; then
        # Enable USB Audio Class Support
        echo 1 > ${PARAMETERS}/hif_usbaudioclass
        fi
    fi


    
    if [ "$TARGET_TYPE" = "" ]; then
        # Determine TARGET_TYPE
        eval export `$IMAGEPATH/bmiloader -i $NETIF --quiet --info | grep TARGET_TYPE`
    fi
    #echo TARGET TYPE is $TARGET_TYPE

    if [ "$TARGET_VERSION" = "" ]; then
        # Determine TARGET_VERSION
        eval export `$IMAGEPATH/bmiloader -i $NETIF --quiet --info | grep TARGET_VERSION`
    fi
    AR6002_VERSION_REV2=0x20000188
    AR6003_VERSION_REV2=0x30000384
    AR6003_VERSION_REV3=0x30000582
    AR6004_VERSION_REV1=0x30000623
    AR6004_VERSION_REV2=0x30000001
    #echo TARGET VERSION is $TARGET_VERSION

    # Addresses for selected Target addresses
    if [ "$TARGET_TYPE" = "AR6004" ]; then # {
        slp_addr=0x502c
        host_intr_addr=0x400800
        cpu_clk_addr=0x4020
        lpo_cal_time_addr=0x40d4
        lpo_cal_addr=0x40e0
        xtal_settle_addr=0x501c
        option_addr=0x180c0
    fi # }
    if [ "$TARGET_TYPE" = "AR9888" ]; then # {
        slp_addr=0x502c
        host_intr_addr=0x400800
        cpu_clk_addr=0x4020
        lpo_cal_time_addr=0x40d4
        lpo_cal_addr=0x40e0
        xtal_settle_addr=0x501c
        option_addr=0x9024 # TBDXX: Eventually use SCRATCH_0
    fi # }
    if [ "$TARGET_TYPE" = "AR6320" ]; then # {
        slp_addr=0x102c
        host_intr_addr=0x400800
        cpu_clk_addr=0x0020
        lpo_cal_time_addr=0x00d4
        lpo_cal_addr=0x00e0
        xtal_settle_addr=0x101c
        option_addr=0x80c0
    fi # }

    # Temporarily disable System Sleep
    old_options=`$IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$option_addr --or=8`
    old_sleep=`$IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$slp_addr --or=1`

    # Clock setup.
    if [ "$FPGA_FLAG" = "1" ]; then
        # Run at 40/44MHz by default.
        $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$cpu_clk_addr --param=0
    fi

    # It is the Host's responsibility to set hi_refclk_hz
    # according to the actual speed of the reference clock.
    #
    # hi_refclk_hz to 26MHz
    # (Commented out because 26MHz is the default.)
    # $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$(($host_intr_addr + 0x78)) --param=26000000
    if [ "$TARGET_VERSION" = "$AR6004_VERSION_REV2" ]; then
        refclk_hz=26000000
    fi
    if [ "$TARGET_TYPE" = "AR6004" ]; then
        $IMAGEPATH/bmiloader -i $NETIF --set --address=$(($host_intr_addr + 0x78)) --param=$refclk_hz
        #$IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$(($host_intr_addr + 0x80)) --param=11
    fi

    # LPO_CAL_TIME is based on refclk speed: 27500/Actual_MHz_of_refclk
    # So for 26MHz (the default), we would use:
    # param=$((27500/26))
    # $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$lpo_cal_time_addr --param=$param
    # LPO_CAL.ENABLE = 1
    $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$lpo_cal_addr --param=0x100000

    # Set XTAL_SETTLE to ~2Ms default; some crystals may require longer settling time.
    # In general, XTAL_SETTLE time is handled by the WLAN driver (because it needs
    # to match the MAC's SLEEP32_WAKE_XTL_TIME).  The driver's default is 2Ms; so for
    # typical use there's no need to set it here.
    # param=ceiling(DesiredSeconds*32768)
    # if [ "$TARGET_TYPE" = "AR6004" ]; then
    #     $IMAGEPATH/bmiloader -i $NETIF --quiet --set --address=$xtal_settle_addr --param=66
    # fi

    if [ "$macaddr" != "" -o "$newreg" != "" ]; then # {
        # TBD
        # If overriding MAC address or Regulatory Domain, then
        # don't use a single segmented load. Load each component
        # individually.
        RAMFW="";
    else # } {
        if [ "$TARGET_TYPE" = "AR6004" ]; then
            if [ "$FPGA_FLAG" = "1" ]; then
                # WAR - for McKinley fpga2.0, do not load fw.ram.bin
                #       set a non-existed path to load
                RAMFW=${RAMFW-$WORKAREA/target/AR6004/fpga/bin/fw.ram.bin}
            else
                if [ "$TARGET_VERSION" = "$AR6004_VERSION_REV2" ]; then
                    RAMFW=${RAMFW-$WORKAREA/target/AR6004/hw1.1/bin/fw.ram.bin}
                else
                    RAMFW=${RAMFW-$WORKAREA/target/AR6004/hw1.0/bin/fw.ram.bin}
                fi
            fi
        fi
    fi # }

    if [ "$TARGET_TYPE" = "AR6320" ]; then # {
        if [ "$FPGA_FLAG" = "1" ]; then
            RAMFW=${RAMFW-$WORKAREA/target/.output/AR6320/fpga.1/bin/fw.ram.bin}
        else
            RAMFW=${RAMFW-$WORKAREA/target/.output/AR6320/hw.1/bin/fw.ram.bin}
        fi
    fi # }

    if [ "$TARGET_TYPE" = "AR9888" ]; then # {
        if [ "$FPGA_FLAG" = "1" ]; then
          if [ "$macaddr" != "" ]; then # {
            $IMAGEPATH/eeprom --transfer --setmac=$macaddr --file=$EEPROM --interface=$NETIF
          fi
       fi
    fi # }

    if [ "$RAMFW" = "" ]; then # {
        # Loading each component individually rather than a single segmented file

        # Part 1: EEPROM {
        if [ "$TARGET_TYPE" = "AR6004" ]; then
            # set RAM location for the bin file
            EEPROM=${EEPROM-$WORKAREA/host/support/boardData_1_0.bin}
            newreg=""
        fi # }

        # Part 2: OTP {
        if [ "$TARGET_TYPE" = "AR6004" ]; then
            OTP=${OTP-""} # TBD
            LOADADDR=""
            BEGINADDR="0x999a00"
        fi
        if [ "$OTP" != "" ]; then
            if [ "$LOADADDR" != "" ]; then
                $IMAGEPATH/bmiloader -i $NETIF --write --address=$LOADADDR --file=$OTP --uncompress
            else
                $IMAGEPATH/bmiloader -i $NETIF --write --file=$OTP
            fi
            if [ $? -ne 0 ]; then
                echo "Failed to load OTP application."
                exit -1
            fi
            if [ "$macaddr" = "" ]; then
                if [ "$newreg" = "" ]; then
                    $IMAGEPATH/bmiloader -i $NETIF --execute --address=$BEGINADDR --param=0
                else
                    $IMAGEPATH/bmiloader -i $NETIF --execute --address=$BEGINADDR --param=2
                fi
            else
                if [ "$newreg" = "" ]; then
                    $IMAGEPATH/bmiloader -i $NETIF --execute --address=$BEGINADDR --param=1
                else
                    $IMAGEPATH/bmiloader -i $NETIF --execute --address=$BEGINADDR --param=3
                fi
            fi
        fi
        # }

        # Part 3: athwlan RAM firmware {
        if [ "$TARGET_TYPE" = "AR6004" ]; then
            wlanapp=${wlanapp-$WORKAREA/target/AR6004/hw1.0/bin/athwlan.bin}
            LOADADDR=""
            BEGINADDR=""
        fi
        if [ "$wlanapp" != "" ]; then # {
            if [ "$LOADADDR" != "" ]; then
                # Old non-segmented athwlan.bin file
                if [ -r $wlanapp.z77 ]; then
                    # If a compressed version of the WLAN application exists, use it
                    $IMAGEPATH/bmiloader -i $NETIF --write --address=$LOADADDR --file=$wlanapp.z77 --uncompress
                else
                    # ...otherwise, use an uncompressed RAM image.
                    $IMAGEPATH/bmiloader -i $NETIF --write --address=$LOADADDR --file=$wlanapp
                fi
            else
                $IMAGEPATH/bmiloader -i $NETIF --write --file=$wlanapp
            fi
            if [ $? -ne 0 ]; then
                echo "Failed to load wlan application."
                exit -1
            fi

            if [ "$BEGINADDR" != "" ]; then
                # If any RAM image was used, set the application start address.
                $IMAGEPATH/bmiloader -i $NETIF --begin --address=$BEGINADDR
            fi
        fi # }
        # }

        # Part 4: RAM DataSets {
        if [ "$TARGET_TYPE" = "AR6004" ]; then
            dsetimg="" # TBD
        fi

        if [ "$dsetimg" != "" ]; then
            $IMAGEPATH/bmiloader -i $NETIF --write --address=$LOADADDR --file=$dsetimg
            if [ $? -ne 0 ]; then
                echo "Failed to load DataSets."
                exit -1
            fi
            $IMAGEPATH/bmiloader -i $NETIF --write --address=$(($host_intr_addr + 0x18)) --param=$LOADADDR
        fi
        # }
    else # } {
        if [ "$TARGET_TYPE" = "AR6004" ]; then # {
            if [ "$FPGA_FLAG" = "1" ]; then # {
                $IMAGEPATH/eeprom --transfer --file=$EEPROM --interface=$NETIF
            else
                EEPROM=${EEPROM-$WORKAREA/host/support/boardData_2_1_${board_type}.bin}
                if [ "$TARGET_VERSION" = "$AR6004_VERSION_REV2" ]; then
                    EEPROM_ADDR=0x426400
                else
                    # data patch file : 0x42553C to 0x428000: Size = 0x2AC4
                    # board data      : 0x423900 to 0x425500: Size = 0x1c00
                    EEPROM_ADDR=0x423900
                fi
                $IMAGEPATH/bmiloader -i $NETIF --write --address=$EEPROM_ADDR --file=$EEPROM
                $IMAGEPATH/bmiloader -i $NETIF --write --address=$(($host_intr_addr + 0x54)) --param=$EEPROM_ADDR
            fi # }
            length=$(stat --format=%s $EEPROM)
            $IMAGEPATH/bmiloader -i $NETIF --set --address=$(($host_intr_addr + 0x58)) --param=$length
        fi # }
        # Segmented download of RAMFW includes:
	#   setup app
        #   generic Board Data
        #   OTP overrides
        #   RAM DataSets, if any
        #   athwlan RAM firmware image

        if [ -r $RAMFW ]; then
            echo Download RAM firmware
            $IMAGEPATH/bmiloader -i $NETIF --write --file=$RAMFW
        else
	    echo NO RAM firmware: $RAMFW
        fi
    fi # }

    # Additional platform-specific setup {
        
        if [ "$wow_ext" = "1" ]; then
        	# WOW extensions, enable and let firmware use defaults 
        	$IMAGEPATH/bmiloader -i $NETIF --set --address=0x5406ec --param=0x80000000
        fi

    # }
    
    if [ "$TARGET_TYPE" = "AR6004" ]; then
        if [ "$FPGA_FLAG" = "1" ]; then
            dbguart_tx=""
        else
            dbguart_tx=${dbguart_tx:-11}
        fi

        if [ "$ATH_ACSVAP_TEST" = "yes" ]; then
            # enable special ACS test mode
            $IMAGEPATH/bmiloader -i $NETIF --set --address=$(($host_intr_addr + 0xc0)) --param=7
        fi

        if [ "$ATH_ENABLE_CLI" = "yes" ]; then
            # enable console 
            $IMAGEPATH/bmiloader -i $NETIF --set --address=$(($host_intr_addr + 0xc4)) --param=0x80000000
        fi
	# Configure UART
	if [ "$dbguart_tx" != "" ]; then
   		echo Set debug UART to pin $dbguart_tx
                $IMAGEPATH/bmiloader -i $NETIF --set --address=$(($host_intr_addr + 0x80)) --param=$dbguart_tx
    	fi

    fi

    # Restore System Sleep
    if [ "$ATH_BUS_TYPE" != "USB" ]; then
        $IMAGEPATH/bmiloader -i $NETIF --set --address=$slp_addr --param=$old_sleep > /dev/null
    fi
    $IMAGEPATH/bmiloader -i $NETIF --set --address=$option_addr --param=$old_options > /dev/null

    # } end platform-specific setup 

    if [ "$PARAMETERS" != "" ]; then
        # Restore Host debug state.
        echo $save_dbg > ${PARAMETERS}/debugdriver
    fi

    if [ \( $exit_value -eq 0 \) -a \( $START_WLAN = TRUE \) ]; then
        # Leave BMI now and start the WLAN driver.
        $IMAGEPATH/bmiloader -i $NETIF --done
    fi
fi # }

exit $exit_value
