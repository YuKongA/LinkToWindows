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
  ui_print "- APatch version: $APATCH_VER ($APATCH_VER_CODE)"
else
  ui_print "- Magisk version: $MAGISK_VER ($MAGISK_VER_CODE)"
fi

if [ ! -f /system_ext/etc/selinux/system_ext_service_contexts ]; then
  ui_print "-----------------------------------------------------------"
  ui_print "! /system_ext/etc/selinux/system_ext_service_contexts not found"
  ui_print "! This device's SEPolicy layout is incompatible with the module"
  abort "-----------------------------------------------------------"
fi

ui_print "- Setting permissions…"
set_perm_recursive $MODPATH 0 0 0755 0644
set_perm_recursive $MODPATH/system/system_ext/bin 0 2000 0751 0755
chmod u+x $MODPATH/uninstall.sh

ui_print "- Installing apps…"
for entry in \
    "com.microsoft.deviceintegrationservice:system/product/priv-app/DeviceIntegrationService/DeviceIntegrationService.apk" \
    "com.microsoft.appmanager:system/product/priv-app/LinkToWindows/LinkToWindows.apk" \
    "com.microsoftsdk.crossdeviceservicebroker:system/system_ext/app/CrossDeviceServiceBroker/CrossDeviceServiceBroker.apk"; do
    pkg="${entry%%:*}"
    apk="${entry#*:}"
    if pm list packages | grep -q "^package:${pkg}$"; then
        ui_print "- $pkg already installed, skipping…"
    else
        ui_print "- Installing $pkg…"
        size=$(stat -c%s "$MODPATH/$apk")
        result=$(pm install -r -d -S "$size" < "$MODPATH/$apk" 2>&1) || ui_print "  ! $result"
    fi
done

ui_print "- Installation is complete"
ui_print "- Please reboot now"
