#!/bin/sh

fw=default.fw
break_in=0x2f8
break_out=0x2d00

tid=0
export tid

# start from this packet
num=$(($1 - 1))

# and end with that packet
num_max=$2

if [ "x$num" = "x" ] || [ "x$num_max" = "x" ]; then
	echo "Please provide min & max packet numbers"
	exit
fi

do_put_packet() {
	local num=$1

	echo "--- PUT PACKET #$num"
	pe_data_put.sh $num

	echo "--- READOUT PACKET #$num"
	len=128 pe_data_get.sh
}

do_get_packet() {
	local num=$1

	echo "--- GET PROCESSED PACKET #$num"
	pe_data_get.sh
}

# download FW and set breakpoints
onu sced $tid $fw
onu scebs $tid $break_in
onu scebs $tid $break_out

#onu scebs $tid 0x28 # exit

onu scer $tid

while true; do
	# exit when handled enough packets
	if [ $num -gt $num_max ]; then
		exit
	fi

	# break check
	mask=`onu scebc | sed 's/.*mask=0x\([0-9A-Fa-f]\+\).*/\1/'`

	if [ "$mask" = "0" ] || [ "x$mask" = "x" ]; then
		echo "breakpoint not reached; exit"
		exit
	fi

	# get the PC
	pc=`onu sceb $tid | sed 's/.*addr=\([0-9a-fx]\+\).*/\1/'`

	if [ "$pc" = "$break_in" ]; then
		num=$((num + 1))

		# exit when handled enough packets
		if [ $num -gt $num_max ]; then
			exit
		fi

		do_put_packet $num
	elif [ "$pc" = "$break_out" ]; then
		do_get_packet $num
	else
		echo "unknown breakpoint ($pc); exit"
		exit
	fi

	# run till the next break
	onu scer $tid
done
