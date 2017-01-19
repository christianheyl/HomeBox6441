#!/bin/sh

#
# Script to program Target flash.
#
# This script acts as a front end for the flashloader program.  It handles
# the most common activities in a safe way.  "Experts" can use the flashloader
# tool directly, but there is a larger risk that something will get messed up.
#
# This is used in the following circumstances:
# 1) You want to upgrade standard firmware and data on flash
# 2) You are installing standard firmware and data for the first time
#    on blank flash (or on flash that just contains Board Data)
# 3) You want to backup/restore the entire flash image
#
# N.B.: This script can be modified for use with various flash
# parts or flash layouts.
#

Help() {
        echo "Usage: $0 [options]"
        echo
        echo "With NO options, perform a default flash update."
        echo "This updates software components, leaving data untouched."
	echo "It is equivalent to --os --wlan."
        echo
        echo "To request update of individual flash components:"
        echo "   --wlan  | --wlan=wlanBinary | -w=wlanBinary"
        echo "   --os    | --os=osBinary     | -o=osBinary"
        echo "   --dsets | --dsets=dsetDesc  | -d=dsetDesc"
        echo "   --all"
        echo
        echo "To backup/restore flash including Board-specific Data:"
        echo "   --backup=file"
        echo "   --restore=file"
        echo
        echo "To force all operations to be staged through a local file:"
        echo "   --image=file"
        echo
        echo "To avoid all contact with Target:  (Use with --image.)"
        echo "   --notarg"
        echo
        # Undocumented option
        # echo "To just load the Target-side Flash application:"
        # echo "   --targapp"
        # echo
        echo "To see this help information"
        echo "   --help"
        exit 0
}

Usage() {
        echo "Type '$0 --help' for usage information"
        exit 1
}

be_patient() {
    echo "This operation may take a minute or more."
    echo "Please be patient.  Do not interrupt this operation."
}

target_setup() {
    echo Loading Target....
    $BMILOADER -i $NETIF --write --address=$OSRAMADDR   --file=$OSRAM > $FLASHLOG
    $BMILOADER -i $NETIF --write --address=$FLASHADDR  --file=$FLASHBIN > $FLASHLOG
    if [ "$ALT_FLASHPARTBIN" ]
    then
        $BMILOADER -i $NETIF --write --address=$ALT_FLASHPARTADDR --file=$ALT_FLASHPARTBIN > $FLASHLOG
    fi

    # Set AR6K boot options:
    # AR6K_OPTION_BMI_DISABLE |
    # AR6K_OPTION_SLEEP_DISABLE |
    # AR6K_OPTION_DSET_DISABLE
    $BMILOADER -i $NETIF --set   --address=0xac0140c0  --param=0x49 > $FLASHLOG

    # hi_flash_is_present=1
    $BMILOADER -i $NETIF --write  --address=0x8000060c  --param=0x1 > $FLASHLOG

    # bank0_config_value for default flash part
    $BMILOADER -i $NETIF --set   --address=0xac004004  --param=0x920000a2 > $FLASHLOG

    $BMILOADER -i $NETIF --begin --address=$OSRAMADDRU > $FLASHLOG
}

verify_wlan() {
    if [ ! -r "$WLAN" ]; then
        echo "WLAN binary file, '$WLAN', not found"
        exit 1
    fi
}

update_wlan() {
    echo
    echo "Update WLAN software using '$WLAN'."
    if [ ! "$USE_IMAGE" ]
    then
        be_patient
    fi
    $FLASHLOADER $USE_IMAGE -i $NETIF $ALT_FLASHPART --write --address=$WLANADDR --file=$WLAN > $FLASHLOG
    if [ $? -ne 0 ]
    then
        echo "WLAN update failed (code=$?).  Abort."
        exit 1
    fi
}

verify_os() {
    if [ ! -r "$OS" ]; then
        echo "OS binary file, '$OS', not found."
        exit 1
    fi
}

update_os() {
    echo
    echo "Update OS software using '$OS'."
    if [ ! "$USE_IMAGE" ]
    then
        be_patient
    fi
    $FLASHLOADER $USE_IMAGE -i $NETIF $ALT_FLASHPART --write --address=$OSFLASHADDR --file=$OS > $FLASHLOG
    if [ $? -ne 0 ]
    then
        echo "OS update failed (code=$?).  Abort."
        exit 1
    fi
}

GREP_PROG=${GREP_PROG:-grep}
TMPDIR=${TMPDIR:-/tmp}
DSETBIN=${DSETBIN:-$TMPDIR/data.flash.bin.$$}

update_dsets() {
    if [ "x$DSETMAP" != "x" ]
    then
        DSETMAP_OPTION="--mapout=$DSETMAP"
    fi

    # Create new data image from dsets.txt file
    $MKDSETIMG --desc=$DSETS --out=$DSETBIN $DSETMAP_OPTION --start=$DSET_INDEX > $FLASHLOG

    # Commit the DataSet image to flash
    $FLASHLOADER $USE_IMAGE -i $NETIF $ALT_FLASHPART --write --address=$DSET_INDEX --file=$DSETBIN > $FLASHLOG
    eval $RM_PROG $DSETBIN

    # Make sure we have a valid DataSet Metadata pointer
    # Usually, this is written once by ART and does not change.
    $FLASHLOADER $USE_IMAGE -i $NETIF $ALT_FLASHPART --write --address=$DSET_ADDR --param=$DSET_INDEX > $FLASHLOG
    if [ $? -ne 0 ]
    then
        echo "DataSet index init failed (code=$?).  Abort."
        exit 1
    fi
}

# DataSet updates are handled in a local file, which is
# committed to flash in the final step.
eval FLASH_IMAGE=${FLASH_IMAGE:-$TMPDIR/flash_image.$$}

GETOPT_PROG=${GETOPT_PROG:-getopt}
RM_PROG=${RM_PROG:-rm}
SLEEP_PROG=${SLEEP_PROG:-sleep}
DIRNAME_PROG=${DIRNAME_PROG:-dirname}
NETIF=${NETIF:-eth1}
BMILOADER=${BMILOADER:-$WORKAREA/host/.output/$ATH_PLATFORM/image/bmiloader}
FLASHLOG=${FLASHLOG:-/dev/null}
FLASHLOADER=${FLASHLOADER:-$WORKAREA/host/.output/$ATH_PLATFORM/image/flashloader}
MKDSETIMG=${MKDSETIMG:-$WORKAREA/host/.output/$ATH_PLATFORM/image/mkdsetimg}
OSRAM=${OSRAM:-$WORKAREA/target/AR6001/bin/athos.ram.bin}
OSRAMADDR=${OSRAMADDR:-0x80009000}
OSRAMADDRU=${OSRAMADDRU:-0xa0009000}

FLASHBIN=${FLASHBIN:-$WORKAREA/target/AR6001/bin/flash.ram.bin}
FLASHADDR=${FLASHADDR:-0x80002000}
FLASHADDRU=${FLASHADDRU:-0xa0002000}

#
# Support for alternate flash parts that use a command set other
# than AMD16.
#
# ALT_FLASHPARTBIN=${ALT_FLASHPARTBIN:-$WORKAREA/target/AR6001/bin/flashpart.ram.bin}
# ALT_FLASHPARTADDR=${ALT_FLASHPARTADDR:-0x80008000}
# ALT_FLASHPART="--part=$ALT_FLASHPARTADDR"

if [ "$#" -eq 0 ]
then
    set -- --wlan --os
fi

args=`$GETOPT_PROG -l wlan::,os::,dsets::,targapp,backup::,restore::,image::,notarg,help,all "w:o:d:tb:r:i:hna" "$@"`
if [ $? -ne 0 ]
then
    Usage
fi
set -- $args

WLAN=""
OS=""
DSETS=""
JUST_LOAD=""
BACKUP_FILE=""
RESTORE_FILE=""
USE_IMAGE=""
RM_IMAGE=""
NOTARG=""
ACTION=""
WLANADDR=${WLANADDR:-0xa200b000}
OSFLASHADDR=${OSFLASHADDR:-0xa2000000}

MORE=TRUE
while [ "$MORE" ]
do
        case "$1" in
        -w | --wlan)
                shift
                eval WLAN="$1"
                WLAN=${WLAN:-$WORKAREA/target/AR6001/bin/athwlan.flash.bin}
                ACTION=TRUE;
                ;;
        -o | --os)
                shift
                eval OS="$1"
                OS=${OS:-$WORKAREA/target/AR6001/bin/athos.flash.bin}
                ACTION=TRUE;
                ;;
        -d | --dsets)
                shift
                eval DSETS="$1"
                DSETS=${DSETS:-$WORKAREA/host/support/dsets.txt}
                DSET_ADDR=${DSET_ADDR:-0xa207fdfc}
                DSET_INDEX=${DSET_INDEX:-0xa2040000}
                USE_IMAGE="--image=$FLASH_IMAGE"
                RM_IMAGE=${RM_IMAGE:-"yes"}
                ACTION=TRUE;
                ;;
        -t | --targapp)
                JUST_LOAD=TRUE
                ACTION=TRUE;
                ;;
        -b | --backup)
                shift
                eval BACKUP_FILE="$1"
                if [ ! "$BACKUP_FILE" ]
                then
                    Usage
                fi
                ACTION=TRUE;
                ;;
        -r | --restore)
                shift
                eval RESTORE_FILE="$1"
                if [ ! "$RESTORE_FILE" ]
                then
                    Usage
                fi
                ACTION=TRUE;
                ;;
        -i | --image)
                shift
                eval FLASH_IMAGE="$1"
                if [ ! "$FLASH_IMAGE" ]
                then
                    Usage
                fi
                RM_IMAGE="no"
                eval USE_IMAGE="--image=$FLASH_IMAGE"
                ;;
        -n | --notarg)
                NOTARG=TRUE
                ;;
        -a | --all)
                    shift
                    set -- "junk" "--wlan" "" "--os" "" "--dsets" "" "$@"
                    ;;
        -h | --help)
                Help
                ;;
        --)
                shift
                if [ "$#" -ne 0 ]
                then
                    echo "Illegal: $*"
                    Usage
                fi

                if [ ! "$ACTION" ]
                then
                    set -- "junk" "--wlan" "" "--os" "" "--"
                else
                    MORE=""
    	    	    break
                fi
                ;;
        esac
        shift
done

# Verify sane environment variables
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
        echo "Loader application, '$BMILOADER', not found"
        exit 1
fi

if [ ! -x "$FLASHLOADER" ]; then
        echo "Flash Loader application, '$FLASHLOADER', not found"
        exit 1
fi

if [ ! -r "$OSRAM" ]; then
        echo "OS RAM image, '$OSRAM', not found"
        exit 1
fi

if [ ! -r "$FLASHBIN" ]; then
        echo "Flash binary, '$FLASHBIN', not found"
        exit 1
fi


# False sharing of flash sectors on the standard flash part requires
# that we use an image file whenever we update just one of the
# firmware images.
if [ \( "$WLAN" -a ! "$OS" \) -o \( ! "$WLAN" -a "$OS" \) ]
then
        USE_IMAGE="--image=$FLASH_IMAGE"
        RM_IMAGE=${RM_IMAGE:-"yes"}
fi


# Verify that all files are accessible

if [ "$WLAN" ]
then
    verify_wlan
fi

if [ "$OS" ]
then
    verify_os
fi

# Load the Target Flash Application
if [ ! "$NOTARG" ]
then
    target_setup
fi

if [ "$JUST_LOAD" ]
then
    exit 0
fi

# If a backup was requested, do that before we modify flash.
if [ "$BACKUP_FILE" ]
then
    if [ -e "$BACKUP_FILE" ]
    then
        echo "Backup file, '$BACKUP_FILE', already exists."
        exit 1
    fi
    if [ "$NOTARG" ]
    then
        echo "Conflicting options.  Cannot create backup with --notarg."
        Usage
    fi
    $FLASHLOADER -i $NETIF $ALT_FLASHPART --read --file=$BACKUP_FILE > $FLASHLOG
    if [ $? -ne 0 ]
    then
        echo "Could not create backup file from flash (code=$?)."
        exit 1
    else
        echo "Flash backup file, '$BACKUP_FILE', created."
    fi
fi

if [ "$RESTORE_FILE" ]
then
    if [ "$NOTARG" ]
    then
        echo "Conflicting options.  Cannot restore with --notarg."
        Usage
    fi
    echo
    echo "Restoring flash from file, '$RESTORE_FILE'."
    be_patient
    $FLASHLOADER -i $NETIF $ALT_FLASHPART --commit --image=$RESTORE_FILE > $FLASHLOG
    if [ $? -ne 0 ]
    then
        echo "Could not restore flash from backup file (code=$?)."
        exit 1
    else
        echo "Flash restored from file, '$RESTORE_FILE'."
    fi
fi


# Now update all requested sections of flash


if [ "$USE_IMAGE" ]
then
    if [ ! -e "$FLASH_IMAGE" ]
    then
        if [ "$NOTARG" ]
        then
            echo "Need to fetch initial flash image, but --notarg disallows."
            Usage
        fi
        # Fetch the current flash image
        $FLASHLOADER -i $NETIF $ALT_FLASHPART --read --file=$FLASH_IMAGE  > $FLASHLOG
        if [ $? -eq 0 ]
        then
            $FLASHLOADER -i $NETIF $ALT_FLASHPART --verify --file=$FLASH_IMAGE  > $FLASHLOG
            if [ $? -eq 0 ]
            then
                echo "Created local flash image, '$FLASH_IMAGE'."
            else
                echo "Verification failed on initial flash image, '$FLASH_IMAGE'."
            exit 1
        fi
        else
            echo "Could not fetch flash image to '$FLASH_IMAGE'. (code=$?)."
            exit 1
        fi
    else
        echo "Use existing flash image, '$FLASH_IMAGE'."
    fi
fi


if [ "$OS" ]
then
    update_os
fi

if [ "$WLAN" ]
then
    update_wlan
fi

if [ "$DSETS" ]
then
    update_dsets
fi

if [ "$USE_IMAGE" -a \( ! "$NOTARG" \) ]
then
    echo
    echo "Commit '$FLASH_IMAGE' to Target flash."
    be_patient
    # Special optimization for flashes that are blank (except for Board Data).
    # If the flash is blank, then flashloader will have no need to erase
    # anything.   So this write should work without --override.  If it fails,
    # then the flash must not have been blank in the same sector as the
    # board data.  So we'll need to write the whole file.
    $FLASHLOADER -i $NETIF $ALT_FLASHPART --write --address=0xa2000000 --file=$FLASH_IMAGE --length=0x7fe00 > $FLASHLOG 2>&1
    # Catch the unusual case where we're updating the entire
    # flash and simultaneously changing board data.  This
    # verify will fail due to a board data mismatch, and
    # we'll try to commit the entire image.
    $FLASHLOADER -i $NETIF $ALT_FLASHPART --verify --file=$FLASH_IMAGE > $FLASHLOG

    if [ $? -ne 0 ]
    then
        # Optimization failed, try things the slow way.
        $FLASHLOADER -i $NETIF $ALT_FLASHPART --commit --image=$FLASH_IMAGE > $FLASHLOG
        $FLASHLOADER -i $NETIF $ALT_FLASHPART --verify --file=$FLASH_IMAGE > $FLASHLOG
    fi
    status=$?

    if [ $status -eq 0 ]
    then
        if [ "$RM_IMAGE" = "yes" ]
        then
            # If the image file was auto-created,
            # and if it was successfully committed to flash,
            # then remove it.
            eval $RM_PROG $FLASH_IMAGE
        fi
    else
        echo "Error (code=$status) while committing image, '$FLASH_IMAGE', to flash."
        exit 1
    fi
fi

if [ ! "$NOTARG" ]
then
    # Indicate completion of the flash operation to the target
    $FLASHLOADER -i $NETIF $ALT_FLASHPART --done > $FLASHLOG
fi

echo "$0 is done."
exit 0
