#!/system/bin/sh
MODDIR=${0%/*}

cp -af $MODDIR/injectrc /data/local/tmp
cd /data/local/tmp
chmod +x ./injectrc
./injectrc /system_ext/etc/init/virtual_keyboard.rc
rm ./injectrc
