#!/system/bin/sh
MODDIR=${0%/*}

if [ -f "$MODDIR/sepolicy.rule" ]; then
    if [ "$KSU" = "true" ]; then
        /data/adb/ksu/bin/ksud sepolicy apply "$MODDIR/sepolicy.rule"
    else
        magiskpolicy --live --apply "$MODDIR/sepolicy.rule"
    fi
fi
