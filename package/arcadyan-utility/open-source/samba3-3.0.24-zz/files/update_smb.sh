#!/bin/sh

# Include model information
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

lan_ip=$lan_main_0_ipAddr

SMB_CONF="/ramdisk/flash/samba/smb.conf"
SMB_PASSWD_FILE="/ramdisk/flash/samba/smbpasswd"
TMP_SMB_LOCK="/tmp/tmp_smb_lock"

# Functions
check_smb_locked()
{
    if [ -f ${TMP_SMB_LOCK} ]; then
        return 1
    else
        return 0
    fi
}

update_user() {
	rm -f "$SMB_PASSWD_FILE"
	touch "$SMB_PASSWD_FILE"
#	echo > "$SMB_PASSWD_FILE"

	# add other users to access samba share
	idx=0; #FIXME : need to be 0
#	echo "start file user"
	while [ $idx -lt $smbPassFileLineCount ]; do
		eval smbPassVar='$smbPassFileLineCount'$idx
		echo $smbPassVar >> $SMB_PASSWD_FILE

		idx=$(( $idx + 1 ))
	done

	if [ $idx -eq 0 ]; then
		# add root/admin users to access samba share
		smbpasswd root "$Password"
		smbpasswd admin "$Password"
	fi
}

smb_header() {
#	[ -L $SMB_CONF ] || ln -nsf /tmp/smb.conf $SMB_CONF

	[ -z "$smb_name" ] && return
	[ -z "$smb_workgroup" ] && return
	[ -z "$smb_descr" ] && return

	echo "[global]" > $SMB_CONF
#	echo "       netbios name = $smb_name" >> $SMB_CONF
	echo "       workgroup = $smb_workgroup" >> $SMB_CONF
	echo "       server string = $smb_descr" >> $SMB_CONF
	echo "       map to guest = Bad User" >> $SMB_CONF
	echo "       log file = /var/log/samba.%m" >> $SMB_CONF
	echo "       log level = 1" >> $SMB_CONF
	echo "       socket options = TCP_NODELAY IPTOS_LOWDELAY SO_SNDBUF=8192 SO_RCVBUF=8192" >> $SMB_CONF
	echo "       read raw = yes" >> $SMB_CONF
	echo "       write raw = yes" >> $SMB_CONF
	echo "       oplocks = yes" >> $SMB_CONF
	echo "       max xmit = 65535" >> $SMB_CONF
	echo "       dead time = 15" >> $SMB_CONF
	echo "       getwd cache = yes" >> $SMB_CONF
	echo "       lpq cache = 30" >> $SMB_CONF
	echo "       max log size = 50" >> $SMB_CONF
	echo "       dns proxy = No" >> $SMB_CONF
	[ -n "$lan_ip" ] && {
		echo "       interfaces = $lan_ip/24" >> $SMB_CONF
        } || {
		echo "       interfaces = 192.168.1.1/24" >> $SMB_CONF
	}
	echo "       bind interfaces only = yes" >> $SMB_CONF
	echo "       guest account = admin" >> $SMB_CONF
	echo "       use sendfile = yes" >> $SMB_CONF
}

smb_add_share() {

#	echo "fs Count = $file_share_Count"
	[ -z "$file_share_Count" ] && return

	idx_list=""
	idx=0
#	echo "start file share"
	while [ $idx -lt $file_share_Count ]; do
#		echo "index = $idx"
		# get rid of duplicate share entry
		idx_found=0
		for adv_idx in $idx_list; do
			if [ $idx -eq $adv_idx ]; then
				idx_found=1
				break
			fi
		done
		if [ $idx_found -eq 1 ]; then
			idx=$(( $idx + 1 ))
			continue
		fi

		# handle share entry
		ro_users=""
		rw_users=""
		eval name='$fs_'$idx'_name'
		eval path='$fs_'$idx'_path'
		eval rw_lvl='$fs_'$idx'_rwLvl'
		eval users='$fs_'$idx'_users'
		[ -z "$name" ] && continue
		[ -z "$path" ] && continue
		echo "" >> $SMB_CONF
		echo "[$name]" >> $SMB_CONF
		echo "       path = $path" >> $SMB_CONF
		echo "       read only = yes" >> $SMB_CONF
		echo "       browsable = yes" >> $SMB_CONF
		if [ $rw_lvl -eq 1 ]; then
			rw_users=${users}
		else
			ro_users=${users}
		fi

		# handle all next entries for the same share name
		tmp_idx=$(( $idx + 1 ))
		while [ $tmp_idx -lt $file_share_Count ]; do
			eval tmp_name='$fs_'$tmp_idx'_name'
			eval rw_lvl='$fs_'$tmp_idx'_rwLvl'
			eval users='$fs_'$tmp_idx'_users'
			if [ "$tmp_name" = "$name" ]; then
				idx_list="$idx_list $tmp_idx"
#				echo "$idx_list"
				if [ $rw_lvl -eq 1 ]; then
					if [ "$rw_users" != "" ]; then
						rw_users=${rw_users},${users}
					else
						rw_users=${users}
					fi
				else
					if [ "$ro_users" != "" ]; then
						ro_users=${ro_users},${users}
					else
						ro_users=${users}
					fi
				fi
			fi

			tmp_idx=$(( $tmp_idx + 1 ))
		done
		if [ "$rw_users" != "" -a "$ro_users" != "" ]; then
#			echo "       writable = Yes" >> $SMB_CONF
			echo "       valid users = $rw_users,$ro_users" >> $SMB_CONF
			echo "       write list = $rw_users" >> $SMB_CONF
			echo "       read list = $ro_users" >> $SMB_CONF
		elif [ "$rw_users" != "" ]; then
#			echo "       writable = Yes" >> $SMB_CONF
			echo "       valid users = $rw_users" >> $SMB_CONF
			echo "       write list = $rw_users" >> $SMB_CONF
		else
#			echo "       writable = No" >> $SMB_CONF
			echo "       valid users = $ro_users" >> $SMB_CONF
			echo "       read list = $ro_users" >> $SMB_CONF
		fi
		echo "       guest ok = no" >> $SMB_CONF

		idx=$(( $idx + 1 ))
	done
}

###########################################################
#                        MAIN                             #
###########################################################

# check lock file
currenttime=`date +%s`
check_smb_locked
while [ $? -ne 0 ]; do
    newtime=`date +%s`
    # the longest waiting time is 30s, avoid endless waiting
    if [ $(( ${newtime} - ${currenttime} )) -gt 30 ]; then
        exit 1
    fi
    sleep 1
    check_smb_locked
done

# create lock file
touch ${TMP_SMB_LOCK}

# load rc.conf
if [ ! "$ENVLOADED" ]; then
	if [ -r /etc/rc.conf ]; then
		. /etc/rc.conf 2> /dev/null
		ENVLOADED="1"
	fi
fi

# update user
update_user

# add header in smb.conf
smb_header

# add share in smb.conf
smb_add_share

# release lock file
rm -f ${TMP_SMB_LOCK}

exit 0
