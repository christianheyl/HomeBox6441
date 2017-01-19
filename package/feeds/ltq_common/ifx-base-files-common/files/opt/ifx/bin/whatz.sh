#!/bin/sh

for i in $*; do
	version=`zcat $i | strings | grep "@(#)"`
	name=`basename $i`
	echo $name : $version
done

