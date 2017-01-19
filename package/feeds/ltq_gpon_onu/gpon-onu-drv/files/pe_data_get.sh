#!/bin/sh

addr=$((0x8000))

if [ "x$tid" = "x" ]; then
	tid=0
fi

# do_memset <tid>, <pe_addr>
do_memget() {
	local tid=$1
	local addr=$2

	onu scemg $tid $addr | grep 'val=' | sed 's/.*val=0x\([0-9A-Fa-f]\+\).*/\1/'
}

echo "# packet data for tid=$tid addr=$addr"

# read header
word0=`do_memget $tid $(expr $addr + 0)`
word1=`do_memget $tid $(expr $addr + 4)`
word2=`do_memget $tid $(expr $addr + 8)`
word3=`do_memget $tid $(expr $addr + 12)`

printf %08x 0x$word0
printf %08x 0x$word1
echo ""
printf %08x 0x$word2
printf %08x 0x$word3
echo ""

if [ "x$len" = "x" ]; then
	# compute length
	word0_dec=$((0x$word0))

	len=`expr $word0_dec % 256`

	# round up
	rem=`expr $len % 8`
	if [ ! $rem -eq 0 ]; then
		len=$(($len + 8 - $rem))
	fi
fi

len=`expr $len + 16` # add the header len

# print data
if [ $len -lt 145 ]; then
	len_seq=`expr $len - 1`

	for i in `seq 16 4 $len_seq`; do
		word=`do_memget $tid $(expr $addr + $i)`

		if [ `expr $i % 8` = 0 ]; then
			printf %08x 0x$word
		else
			printf %08x 0x$word
			echo ""
		fi

	done
else
	echo "# warning; length (==$len) is greater than 144 (header + max len)!"
fi

# print packet end mark
if [ ! `expr $len % 8` = 0 ]; then
	echo ""
fi

echo ":"
