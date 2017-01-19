#!/bin/sh

if [ "$#" != "2" ]; then
    echo Syntax: $0 interval count
    exit 0
fi
pecostat -c pic0=0,pic1=1,pic2=37,pic3=37:EXL,K,S,U,IE \
         -c pic0=0,pic1=1,pic2=41,pic3=41:EXL,K,S,U,IE \
         -c pic0=0,pic1=1,pic2=45,pic3=45:EXL,K,S,U,IE \
         -c pic0=0,pic1=1,pic2=18,pic3=55:EXL,K,S,U,IE \
         -c pic0=0,pic1=1,pic2=46,pic3=46:EXL,K,S,U,IE \
         -c pic0=0,pic1=51,pic2=1,pic3=53:EXL,K,S,U,IE \
$1 $2

# Example to run
#
#    pecostat.sh 0.8 12 > pecostat.out &
#    time test_app > test_app.out
#    echo DONE
#    echo Wait for pecostat to complete ...
#
#    or
#
#    pecostat.sh 0.8 100 > pecostat.out &
#    pid=$!
#    time test_app > test_app.out
#    pkill pecostat
#    echo DONE
