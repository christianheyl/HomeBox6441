#!/bin/bash

ROOTFS_DIR=$1

usage() {
  echo "usage: $0 <rootfs directory>"
}

dbg() {
  echo $1
}

if [ "x$ROOTFS_DIR" = "x" ] ; then
  usage
  exit -1
fi

if [ ! -d $ROOTFS_DIR ] ; then
  echo "$ROOTFS_DIR does not exist!"
  exit -1
fi

#copy /etc/dbus-1 to /usr/share/config/dbus-1
mkdir -p $ROOTFS_DIR/usr/share/config
#cp -af $ROOTFS_DIR/etc/dbus-1 $ROOTFS_DIR/usr/share/config/
#cp -af $ROOTFS_DIR/etc/config/glbcfg.* $ROOTFS_DIR/usr/share/config/
#cp -af $ROOTFS_DIR/etc/config/.glbcfg $ROOTFS_DIR/usr/share/config/
#cp -af $ROOTFS_DIR/etc/passwd  $ROOTFS_DIR/usr/share/config/
#cp -af $ROOTFS_DIR/etc/config/arc-middle $ROOTFS_DIR/usr/share/config/
#cp -af $ROOTFS_DIR/etc/services $ROOTFS_DIR/usr/share/services
#cp -af $ROOTFS_DIR/etc/l7-protocols $ROOTFS_DIR/usr/share/l7-protocols
# cp -af $ROOTFS_DIR/etc/ $ROOTFS_DIR/usr/share/config/
# symbolic link /etc
# rm -rf $ROOTFS_DIR/etc
# ln -sf /tmp/etc $ROOTFS_DIR/etc
tar czvf $ROOTFS_DIR/etc/dftconfig.tgz -C $ROOTFS_DIR/etc ./config
rm -rf $ROOTFS_DIR/etc/config
ln -sf /tmp/etc/config $ROOTFS_DIR/etc/config

# symbolic link /var
rm -rf $ROOTFS_DIR/var
ln -sf /tmp/var $ROOTFS_DIR/var

# symbolic link /etc
#rm -rf $ROOTFS_DIR/mnt
#ln -sf /tmp/media $ROOTFS_DIR/mnt

# dbus message /usr/share/msg
mkdir -p $ROOTFS_DIR/usr/share/msg

# symbolic link for mtlk firmware
(cd $ROOTFS_DIR; for f in root/mtlk/images/*; do ln -sf /tmp/mtlk_images/$(basename $f) lib/firmware/; done)
