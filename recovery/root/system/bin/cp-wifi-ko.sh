#!/system/bin/sh

# Do not copy in fastbootd mode
FASTBOOTD_PROP=$(getprop ro.twrp.fastbootd)
if [ "$FASTBOOTD_PROP" = "1" ]; then
    echo "I:cp-wifi-ko.sh: Detected fastbootd (ro.twrp.fastbootd=1), exit script." >> /tmp/recovery.log
    exit 0
fi

mount /persist
cp -f /persist/wlan/wlan_mac.bin /vendor/etc/wifi/qca6390
cp -f /persist/wlan/wlan_mac.bin /vendor/etc/wifi/qca6490
umount /persist
mkdir -p /vendor/firmware/wlan/qca_cld/qca6390
ln -s /vendor/etc/wifi/qca6390/wlan_mac.bin /vendor/firmware/wlan/qca_cld/qca6390/wlan_mac.bin
mkdir -p /vendor/firmware/wlan/qca_cld/qca6490
ln -s /vendor/etc/wifi/qca6490/wlan_mac.bin /vendor/firmware/wlan/qca_cld/qca6490/wlan_mac.bin

ln -s /system/bin/netconnect.sh /sbin/wlan_connect
ln -s /system/bin/netscan.sh /sbin/wlan_scan
ln -s /system/bin/netinfo.sh /sbin/wlan_info

resetprop twrp.cpko "true"

exit 0
