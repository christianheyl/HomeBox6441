#!/bin/sh

oper=$1

###########################################################
#                        MAIN                             #
###########################################################

if [ "$oper" = "add" ]; then
	# verification of the args
	if [ $# -ne 4 ]; then
		echo "Usage: sh update_user.sh add <user_name> <password> <uid>"
		exit 1
	fi

	user_name=$2
	password=$3
	uid=$4

	# add user; 'root' or 'admin' already added
	if [ "$user_name" != "root" -a "$user_name" != "admin" ]; then
		(sleep 1; echo "$password"; sleep 1; echo "$password") | adduser -h /tmp -s /bin/sh -G root -H -u $uid $user_name > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "user '$user_name' not created in /etc/passwd"
			exit 1
		fi

		# add user for samba share access
		smbpasswd $user_name $password
		if [ $? -ne 0 ]; then
			echo "user $user_name not created in smbpasswd"
			exit 1
		fi
	fi

elif [ "$oper" = "mod" ]; then
	# verification of the args
	if [ $# -ne 3 ]; then
		echo "Usage: sh update_user.sh mod <user_name> <password>"
		exit 1
	fi

	user_name=$2
	password=$3

	# add user; 'root' or 'admin' already added
#	if [ "$user_name" != "root" -a "$user_name" != "admin" ]; then
	(sleep 1; echo "$password"; sleep 1; echo "$password") | passwd $user_name > /dev/null 2>&1
	if [ $? -ne 0 ]; then
		echo "user '$user_name' not updated in /etc/passwd"
		exit 1
	fi
#	fi

	# add user for samba share access
	smbpasswd -del $user_name > /dev/null 2>&1
	smbpasswd $user_name $password
	if [ $? -ne 0 ]; then
		echo "user $user_name not updated in smbpasswd"
		exit 1
	fi

elif [ "$oper" = "del" ]; then
	# verification of the args
	if [ $# -ne 2 ]; then
		echo "Usage: sh update_user.sh del <user_name>"
		exit 1
	fi

	user_name=$2

	# delete user; 'root' or 'admin' can not be deleted
	if [ "$user_name" != "root" -a "$user_name" != "admin" ]; then
		# delete user from samba share access
		smbpasswd -del $user_name > /dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "user $user_name not deleted"
			exit 1
		fi

		deluser $user_name > /dev/null 2>&1
	fi
else
	echo "unsupported operation '$oper'"
	exit 1
fi

exit 0
