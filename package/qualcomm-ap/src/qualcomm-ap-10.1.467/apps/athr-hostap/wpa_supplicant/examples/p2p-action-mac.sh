#!/bin/sh

IFNAME=$1
CMD=$2

kill_daemon() {
    NAME=$1
    PF=$2

    if [ ! -r $PF ]; then
	return
    fi

    PID=`cat $PF`
    if [ $PID -gt 0 ]; then
	if ps $PID | grep -q $NAME; then
	    kill $PID
	fi
    fi
    rm $PF
}

if [ "$CMD" = "P2P-GROUP-STARTED" ]; then
    GIFNAME=$3
    if [ "$4" = "GO" ]; then
	kill_daemon dhcpd /var/run/dhcpd.pid-$GIFNAME
	ipconfig set $GIFNAME MANUAL 192.168.42.1 255.255.255.0
	ifconfig $GIFNAME 192.168.42.1 up
	touch /var/run/dhcpd.leases-$GIFNAME
	dhcpd -pf /var/run/dhcpd.pid-$GIFNAME \
	    -lf /var/run/dhcpd.leases-$GIFNAME \
	    -cf /etc/dhcpd-go.conf \
	    $GIFNAME
    fi
    if [ "$4" = "client" ]; then
	kill_daemon dhcpd /var/run/dhcpd.pid-$GIFNAME
	ifconfig $GIFNAME remove 192.168.42.1
	ipconfig set $GIFNAME DHCP
    fi
fi

if [ "$CMD" = "P2P-GROUP-REMOVED" ]; then
    GIFNAME=$3
    if [ "$4" = "GO" ]; then
	kill_daemon dhcpd /var/run/dhcpd.pid-$GIFNAME
	ifconfig $GIFNAME remove 192.168.42.1
    fi
    if [ "$4" = "client" ]; then
	ipconfig set $GIFNAME NONE
    fi
fi
