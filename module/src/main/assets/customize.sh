#!/system/bin/sh

SKIPUNZIP=0
AUTOMOUNT=false
PROPFILE=false
POSTFSDATA=false
LATESTARTSERVICE=true

if [ "$ARCH" != arm64 ]; then
  ui_print "-----------------------------------------------------------"
  ui_print "! This module is only available for arm64 devices"
  abort "-----------------------------------------------------------"
fi

if [ "$KSU" = "true" ]; then
  ui_print "- KernelSU version: $KSU_VER ($KSU_VER_CODE)"
elif [ "$APATCH" = "true" ]; then
  APATCH_VER=$(cat "/data/adb/ap/version")
  ui_print "- APatch version: $APATCH_VER"
else
  ui_print "- Magisk version: $MAGISK_VER ($MAGISK_VER_CODE)"
fi

if [ ! -f /system_ext/etc/selinux/system_ext_service_contexts ]; then
  ui_print "-----------------------------------------------------------"
  ui_print "! /system_ext/etc/selinux/system_ext_service_contexts not found"
  ui_print "! this device's SEPolicy layout is incompatible with the module"
  abort "-----------------------------------------------------------"
fi

ui_print "- Setting permissions"
set_perm_recursive $MODDIR 0 0 0755 0644
set_perm_recursive $MODDIR/system/system_ext/bin 0 2000 0751 0755
set_perm_recursive $MODDIR/system/system_ext/bin/virtual_keyboard 0 2000 0755 0755 u:object_r:virtual_keyboard_exec:s0
chmod u+x $MODDIR/uninstall.sh

ui_print "- Installing apps…"
for entry in \
    "com.microsoft.deviceintegrationservice:system/product/priv-app/DeviceIntegrationService/DeviceIntegrationService.apk" \
    "com.microsoft.appmanager:system/product/priv-app/LinkToWindows/LinkToWindows.apk" \
    "com.microsoftsdk.crossdeviceservicebroker:system/system_ext/app/CrossDeviceServiceBroker/CrossDeviceServiceBroker.apk"; do
    pkg="${entry%%:*}"
    apk="${entry#*:}"
    if pm list packages | grep -q "^package:${pkg}$"; then
        ui_print "- $pkg installed. Skip."
    else
        cp "$MODDIR/$apk" /data/local/tmp/
        pm install "/data/local/tmp/$(basename "$apk")"
        rm -f "/data/local/tmp/$(basename "$apk")"
    fi
done

ui_print "- Installation is complete. Please reboot now."
