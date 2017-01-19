#!/bin/sh /etc/rc.common

START=95

boot() {
   . /etc/profile
	cp /opt/lantiq/bin/prj_server /tmp/
	cp /opt/lantiq/bin/prj_disploc /tmp/
	 /tmp/prj_server $SYSLOGIP  &
	 /tmp/prj_disploc -s &
}

start() {
   . /etc/profile
	/opt/lantiq/bin/prj_server $SYSLOGIP &
	/opt/lantiq/bin/prj_disploc -s &
}

restart() {
	return 0
}

stop() {
	killall prj_server
	killall prj_disploc
}

