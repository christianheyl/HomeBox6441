#!/bin/sh
# Pecostat system profiling script on target. Results to be analysed by
# pecostat.awk scripts
# Version : 1.0
# Date    : 1/1/11 - yeah!
# Author  : Ritesh Banerjee

run_pecostat()
{
	lpic0=$1
	lpic1=$2
	lflags="$3"
	lvpeid=$4
	ltcid=$5
	lperiod=$6
	ltimes=$7
	echo $lpic0
	echo $lpic1
	echo -n "##### Running pic0="$lpic0";pic1="$lpic1";mode="$lflags 
	echo ";vpeid="$lvpeid";tcid="$ltcid";for="$lperiod";#times="$ltimes
	echo -n "##### Running pic0="$lpic0";pic1="$lpic1";mode="$lflags >> $logfile 
	echo ";vpeid="$lvpeid";tcid="$ltcid";for="$lperiod";#times="$ltimes >> $logfile
	pecostat -c pic0=$lpic0,pic1=$lpic1:$lflags,VPEID=$lvpeid,TCID=$ltcid $lperiod $ltimes >> $logfile
	echo "%%%%% End of run"
	echo "%%%%% End of run" >> $logfile
}

# initialize pecostat environment. Load the .ko for example
init_pecostat()
{
	logfile=/tmp/pecolog.txt
	echo "" > $logfile
	cd /tmp
}

if [ $# -gt 2 ]
then
	period=$1
	times=$2
elif [ $# -gt 1 ]
then
	period=$1
	times=2
else
	period=5
	times=2
fi
init_pecostat
# pic0=Total instructions; pic1=Total Cycles
# pecostat -c pic0=0,pic1=1:EXL,K,S,U,IE,VPEID=0,TCID=0 $period $times
run_pecostat "0"  "1"  "EXL,K,S,U,IE"  "0"  "0"  $period  $times 
# pic0=Total Stalls; pic1=Total Cycles
run_pecostat "18"  "0"  "EXL,K,S,U,IE"  "0"  "0"  $period  $times 

# pic0=Total Cycles; pic1=D$ stalls
# pecostat -c pic0=0,pic1=37:EXL,K,S,U,IE,VPEID=0,TCID=0 $period $times
run_pecostat "0"  "37"  "EXL,K,S,U,IE"  "0"  "0"  $period  $times 
# pic1=I$ Stalls; pic1=Total cycles
# pecostat -c pic0=37,pic1=0:EXL,K,S,U,IE,VPEID=0,TCID=0 $period $times
run_pecostat 37  0  "EXL,K,S,U,IE"  "0"  "0"  $period  $times 
# pic0=Total Cycles; pic1=Data cache writebacks
# pecostat -c pic0=0,pic1=10:EXL,K,S,U,IE,VPEID=0,TCID=0 $period $times
run_pecostat 0  10  "EXL,K,S,U,IE"  "0"  "0"  $period  $times 
# pic0=Dcache Accesses; pic1=Dcache Misses
run_pecostat 10  11  "EXL,K,S,U,IE"  "0"  "0"  $period  $times 
# pic0=Icache Accesses; pic1=Icache Misses
run_pecostat 9  9  "EXL,K,S,U,IE"  "0"  "0"  $period  $times 
# pic0=branch Instrs; pic1=branch mispredictions
run_pecostat 2 2 "EXL,K,S,U,IE" "0" "0" $period $times
## Add or remove pecostats here below

# pic0=ITLB accesses; pic1=ITLB misses
run_pecostat 5 5 "EXL,K,S,U,IE" "0" "0" $period $times
# pic0=DTLB accesses; pic1=DTLB misses
run_pecostat 6 6 "EXL,K,S,U,IE" "0" "0" $period $times
# pic0=JTLB instr accesses; pic1=JTLB instr misses
run_pecostat 7 7 "EXL,K,S,U,IE" "0" "0" $period $times
# pic0=JTLB data accesses; pic1=JTLB data misses
run_pecostat 8 8 "EXL,K,S,U,IE" "0" "0" $period $times
