#!/bin/sh

for i in $*; do
	version=`strings $i | grep "@(#)"`
	name=`basename $i`
	echo $name : $version
done
