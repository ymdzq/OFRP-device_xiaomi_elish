#!/system/bin/sh

info=$(ifconfig wlan0 2>/dev/null || ifconfig -a wlan0 2>/dev/null)

[ -z "$info" ] && echo "Error: Cannot get wlan0 information" && exit 1

mac=$(echo "$info" | grep -oE '([0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}' | head -1)
ipv4=$(echo "$info" | grep -oE 'inet addr:[0-9.]+' | cut -d: -f2)
ipv6=$(echo "$info" | grep -oE 'inet6 addr: [0-9a-fA-F:]+' | grep -v 'fe80:' | head -1 | awk '{print $3}' | cut -d'/' -f1)

echo "==========Network information========"
echo " "
echo "ipv4: ${ipv4:-Not found}"
echo " "
echo "ipv6: ${ipv6:-Not found}"
echo " "
echo "mac : ${mac:-Not found}"
echo " "
echo "====================================="
echo " "
echo " "
echo " "