#!/system/bin/sh

for pkg in \
    com.microsoft.deviceintegrationservice \
    com.microsoft.appmanager \
    com.microsoftsdk.crossdeviceservicebroker; do
    if pm list packages | grep -q "^package:${pkg}$"; then
        pm uninstall "$pkg"
    fi
done
