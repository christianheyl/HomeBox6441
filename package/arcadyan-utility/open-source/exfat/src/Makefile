LIBEXFAT_OBJ = libexfat/cluster.o libexfat/io.o libexfat/log.o libexfat/lookup.o libexfat/mount.o libexfat/node.o libexfat/time.o libexfat/utf.o libexfat/utils.o 

MKFSEXFAT_OBJ = mkfs/cbm.o mkfs/fat.o mkfs/main.o mkfs/mkexfat.o mkfs/rootdir.o mkfs/uct.o mkfs/uctc.o mkfs/vbr.o

EXT_CFLAGS=-Werror -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat -std=c99

#Install file: "fuse/mount.exfat-fuse" as "/sbin/mount.exfat-fuse"
#Install file: "dump/dumpexfat" as "/sbin/dumpexfat"
#Install file: "fsck/exfatfsck" as "/sbin/exfatfsck"
#Install file: "mkfs/mkexfatfs" as "/sbin/mkexfatfs"
#Install file: "label/exfatlabel" as "/sbin/exfatlabel"
#make_symlink("/sbin", "mount.exfat-fuse", "mount.exfat")
#make_symlink("/sbin", "exfatfsck", "fsck.exfat")
#make_symlink("/sbin", "mkexfatfs", "mkfs.exfat")

all: libexfat mount.exfat-fuse 
#dumpexfat exfatfsck exfatlabel mkexfatfs

libexfat: $(LIBEXFAT_OBJ)
	$(AR) rcs libexfat.a $(LIBEXFAT_OBJ)

mount.exfat-fuse: fuse/main.o
	$(CC) -o mount.exfat-fuse fuse/main.o $(LDFLAGS) -L./ -lexfat -lfuse -liconv

dumpexfat: dump/main.o
	$(CC) -o dumpexfat dump/main.o -L./ -lexfat

exfatfsck: fsck/main.o
	$(CC) -o exfatfsck fsck/main.o -L./ -lexfat

#gcc -o fsck/main.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat fsck/main.c
#gcc -o fsck/exfatfsck fsck/main.o -Llibexfat -lexfat

exfatlabel: label/main.o
	$(CC) -o exfatlabel label/main.o -L./ -lexfat

#gcc -o label/main.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat label/main.c
#gcc -o label/exfatlabel label/main.o -Llibexfat -lexfat

mkexfatfs: $(MKFSEXFAT_OBJ)
	$(CC) -o mkexfatfs $(MKFSEXFAT_OBJ) -L./ -lexfat

#gcc -o mkfs/cbm.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/cbm.c
#gcc -o mkfs/fat.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/fat.c
#gcc -o mkfs/main.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/main.c
#gcc -o mkfs/mkexfat.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/mkexfat.c
#gcc -o mkfs/rootdir.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/rootdir.c
#gcc -o mkfs/uct.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/uct.c
#gcc -o mkfs/uctc.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/uctc.c
#gcc -o mkfs/vbr.o -c -Wall -O2 -ggdb -std=c99 -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -Ilibexfat mkfs/vbr.c
#gcc -o mkfs/mkexfatfs mkfs/cbm.o mkfs/fat.o mkfs/main.o mkfs/mkexfat.o mkfs/rootdir.o mkfs/uct.o mkfs/uctc.o mkfs/vbr.o -Llibexfat -lexfat


%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(EXT_CFLAGS) -c $< -o $@

install:
	mkdir -p $(CONFIG_PREFIX)
	install -m0755 mount.exfat-fuse $(CONFIG_PREFIX)/mount.exfat-fuse
	#install -m0755 dumpexfat $(CONFIG_PREFIX)/dumpexfat
	#install -m0755 exfatfsck $(CONFIG_PREFIX)/exfatfsck
	#install -m0755 mkexfatfs $(CONFIG_PREFIX)/mkexfatfs
	#install -m0755 exfatlabel $(CONFIG_PREFIX)/exfatlabel
	
#Install file: "fuse/mount.exfat-fuse" as "/sbin/mount.exfat-fuse"
#Install file: "dump/dumpexfat" as "/sbin/dumpexfat"
#Install file: "fsck/exfatfsck" as "/sbin/exfatfsck"
#Install file: "mkfs/mkexfatfs" as "/sbin/mkexfatfs"
#Install file: "label/exfatlabel" as "/sbin/exfatlabel"
#make_symlink("/sbin", "mount.exfat-fuse", "mount.exfat")
#make_symlink("/sbin", "exfatfsck", "fsck.exfat")
#make_symlink("/sbin", "mkexfatfs", "mkfs.exfat")

clean:
	rm -rf dump/*.o libexfat/*.o fsck/*.o fuse/*.o label/*.o mkfs/*.o mount.exfat-fuse dumpexfat exfatfsck mkexfatfs exfatlabel

.PHONY : all clean
