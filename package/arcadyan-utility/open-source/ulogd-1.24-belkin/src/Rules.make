#

#prefix=/usr/local
prefix=$(INSTALLDIR)
exec_prefix=${prefix}
ETCDIR=${prefix}/usr/share/ulogd/etc
BINDIR=${exec_prefix}/sbin

#ULOGD_CONFIGFILE=${prefix}/etc/ulogd.conf
ULOGD_CONFIGFILE=/etc/ulogd.conf

ULOGD_LIB_PATH=${exec_prefix}/lib/ulogd
ULOGD_LIB_PATH_REAL=/lib/ulogd

# Path of libipulog (from iptables)
LIBIPULOG=./libipulog
INCIPULOG=-I./libipulog/include
INCCONFFILE=-I./conffile

#CC=gcc
#LD=ld
#INSTALL=/usr/bin/install -c
INSTALL=install -c

CFLAGS=-g -O2  -Wall
CFLAGS+=-DULOGD_CONFIGFILE=\"$(ULOGD_CONFIGFILE)\"
# doesn't work for subdirs
#CFLAGS+=$(INCIPULOG) $(INCCONFFILE)
#CFLAGS+=-I/lib/modules/`uname -r`/build/include
#CFLAGS+=-DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DHAVE_LIBDL=1 -DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1 -DHAVE_DIRENT_H=1 -DSTDC_HEADERS=1 -DHAVE_FCNTL_H=1 -DHAVE_UNISTD_H=1 -DHAVE_VPRINTF=1 -DHAVE_SOCKET=1 -DHAVE_STRERROR=1
#CFLAGS+=-g -DDEBUG -DDEBUG_MYSQL -DDEBUG_PGSQL

LIBS=-ldl 


# Names of the plugins to be compiled
ULOGD_SL:=BASE OPRINT PWSNIFF LOGEMU LOCAL SYSLOG

# mysql output support
#ULOGD_SL+=MYSQL
MYSQL_CFLAGS=-I 
MYSQL_LDFLAGS= 

# postgreSQL output support
#ULOGD_SL+=PGSQL
PGSQL_CFLAGS=-I 
PGSQL_LDFLAGS= 

# mysql output support
#ULOGD_SL+=SQLITE3
SQLITE3_CFLAGS=-I 
SQLITE3_LDFLAGS= 

