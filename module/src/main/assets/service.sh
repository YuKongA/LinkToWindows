#!/system/bin/sh
MODDIR=${0%/*}

cp -af $MODDIR/injectrc /data/local/tmp
cd /data/local/tmp
chmod +x ./injectrc
./injectrc /system_ext/etc/init/virtual_keyboard.rc
rm ./injectrc

until pm list packages >/dev/null 2>&1; do sleep 1; done

for pkg in com.microsoft.appmanager com.microsoft.deviceintegrationservice com.microsoftsdk.crossdeviceservicebroker; do
    pm list packages | grep -q "^package:${pkg}$" || continue
    for op in SYSTEM_ALERT_WINDOW POST_NOTIFICATION RUN_ANY_IN_BACKGROUND START_FOREGROUND; do
        cmd appops set "$pkg" "$op" allow >/dev/null 2>&1
    done
done

for p in \
    android.permission.NEARBY_WIFI_DEVICES \
    android.permission.BLUETOOTH_SCAN \
    android.permission.RECORD_AUDIO \
    android.permission.GET_ACCOUNTS \
    android.permission.READ_EXTERNAL_STORAGE \
    android.permission.READ_MEDIA_IMAGES \
    android.permission.READ_MEDIA_VIDEO \
    android.permission.WRITE_EXTERNAL_STORAGE; do
    pm grant com.microsoft.appmanager "$p" >/dev/null 2>&1
done

for p in \
    android.permission.POST_NOTIFICATIONS \
    android.permission.BLUETOOTH_CONNECT; do
    pm grant com.microsoft.deviceintegrationservice "$p" >/dev/null 2>&1
done
