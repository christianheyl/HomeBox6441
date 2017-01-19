#!/bin/sh

addr=$((0x8000))

if [ "x$file" = "x" ]; then
	file=/parser2_in.txt
fi

if [ "x$tid" = "x" ]; then
	tid=0
fi

if [ $# -eq 0 ]; then
	put_num=0
else
	put_num=$1
fi

# do_memset <tid>, <pe_addr>, <data>
do_memset() {
	local tid=$1
	local addr=$2
	local word=$3

	echo onu scems $tid $addr 0x$word
	onu scems $tid $addr 0x$word
}

# packet counter
num=0

while read line; do
	# pass unnecessary packets (placed here for optimization)
	if [ ! $num -eq $put_num ] && [ ! "x${line##:*}" = "x" ]; then
		continue
	fi

	# count packet number
	if [ "x${line##:*}" = "x" ]; then
		if [ $num -eq $put_num ]; then
			exit
		fi

		num=$(($num + 1))

		continue
	fi

	# pass comments
	if [ "x${line###*}" = "x" ]; then
		continue
	fi

	# pass unnecessary packets
	if [ ! $num -eq $put_num ]; then
		continue
	fi

	# as we have 32-bit access to the PE memory, extract two 32-bit words
	# from the 64-bit one
	hi=`echo $line | sed 's/\([0-9A-Fa-f]\{8\}\)[0-9A-Fa-f]\{8\}/\1/'`
	lo=`echo $line | sed 's/[0-9A-Fa-f]\{8\}\([0-9A-Fa-f]\{8\}\)/\1/'`

	# store the values
	do_memset $tid $addr $hi
	do_memset $tid $(($addr + 4)) $lo

	# go to the next 64-bit word
	addr=$(($addr + 8))
done < $file
