#!/bin/sh
#
# Use webjsmin to process the files in input foler,
# then re-generate to output foler.
# Bryce Liu ++ 2013-09-06
#

PROG=$0
IN=$1
OUT=$2
OPT=
SUPP_EX="js htm html stm"

i=0

for ARGV in $@
do
  if (( "$i" >= "2" )); then
  OPT+="$ARGV ";
  fi
  echo $OPT;
  i=$((i+1));
done



LST=`cd $IN; find . -type f -printf "%T@ %p\n" | sort -nr | cut -d\  -f2-`
for f in $LST
do
	dir=$(dirname "$f")
	fn=$(basename "$f")
	ex="${fn##*.}"
	fn="${fn%.*}"

	MATCH=0
	for m in $SUPP_EX
	do
		if [ $m == $ex ]; then
			MATCH=1
			break
		fi
	done
	
	if [ $MATCH == 1 ]; then
		webjsmin $IN/$dir $fn.$ex $OUT/$dir $fn.$ex $OPT
	else
		if [ ! -e "$OUT/$dir" ]; then
			mkdir -p $OUT/$dir
		fi
		cp $IN/$f $OUT/$f
	fi
done
