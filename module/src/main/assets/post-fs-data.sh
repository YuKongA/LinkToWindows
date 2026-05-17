#!/system/bin/sh

MODDIR=${0%/*}

mkdir -p $MODDIR/mount_point
cp -a /system_ext/etc/selinux/system_ext_service_contexts $MODDIR/mount_point/

grep -q "^cross_device_service[[:space:]]" $MODDIR/mount_point/system_ext_service_contexts || \
    echo "cross_device_service                             u:object_r:cross_device_service:s0" >> $MODDIR/mount_point/system_ext_service_contexts

grep -q "^vendor.virtual_keyboard[[:space:]]" $MODDIR/mount_point/system_ext_service_contexts || \
    echo "vendor.virtual_keyboard                          u:object_r:virtual_keyboard_service:s0" >> $MODDIR/mount_point/system_ext_service_contexts

mount --bind $MODDIR/mount_point/system_ext_service_contexts /system_ext/etc/selinux/system_ext_service_contexts

chcon u:object_r:virtual_keyboard_exec:s0 $MODDIR/system/system_ext/bin/virtual_keyboard

mount --bind $MODDIR/system/system_ext/bin/virtual_keyboard /system_ext/bin/virtual_keyboard

