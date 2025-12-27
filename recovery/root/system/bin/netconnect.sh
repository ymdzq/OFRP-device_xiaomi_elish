#!/system/bin/sh

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Error: Incorrect parameter!"
    echo "Open network usage: $0 <SSID>"
    echo "eg: $0 myopenwifi"
    echo "Encrypt network usage: $0 <SSID> <PSK>"
    echo "eg: $0 myencryptwifi 12345678"
    echo " "
    echo " "
    echo " "
    exit 1
fi

SSID="$1"
PASSWORD="$2"
ENCRY_FILE="/tmp/wlan/list/$SSID"

echo ""
echo ""
echo "Begging connect wlan..."

if [ ! -f "$ENCRY_FILE" ]; then
    echo "Error: cannot find SSID '$SSID'"
    echo "Scan first please!"
    echo " "
    echo " "
    echo " "
    exit 1
fi

ENCRY_TYPE=$(cat "$ENCRY_FILE")

case "$ENCRY_TYPE" in
    "WPA3")
        KEY_MGMT="SAE"
        ;;
    "WPA2")
        KEY_MGMT="WPA-PSK"
        ;;
    "WPA")
        KEY_MGMT="WPA-PSK"
        ;;
    "OPEN")
        KEY_MGMT="NONE"
        ;;
    *)
        KEY_MGMT="WPA-PSK"
        ;;
esac

if [ "$KEY_MGMT" = "NONE" ]; then
    echo "Open network detected!"
    if [ $# -gt 1 ]; then
        echo "Ignoring the provided password parameter"
    fi
else
    echo "Encrypted network detected!"
    if [ $# -lt 2 ]; then
        echo "Error: Encrypted network requires psk parameters!"
        echo "Usage: $0 <SSID> <PSK>"
        echo " "
        echo " "
        echo " "
        exit 1
    fi
fi

echo "Connecting to wlan: $SSID"
echo "Encryption: $ENCRY_TYPE (key_mgmt: $KEY_MGMT)"
echo " "
echo "Remove old network config..."
wpa_cli -iwlan0 -p/tmp/recovery/sockets remove_network 0
echo " "
echo "Add new network config..."
wpa_cli -iwlan0 -p/tmp/recovery/sockets add_network
echo " "
echo "Add SSID to new config..."
wpa_cli -iwlan0 -p/tmp/recovery/sockets set_network 0 ssid '"'"$SSID"'"'
echo " "
echo "Add encrytion to new config..."
wpa_cli -iwlan0 -p/tmp/recovery/sockets set_network 0 key_mgmt "$KEY_MGMT"
echo " "
if [ "$KEY_MGMT" != "NONE" ]; then
    echo "Add password to new config..."
    wpa_cli -iwlan0 -p/tmp/recovery/sockets set_network 0 psk '"'"$PASSWORD"'"'
    echo " "
else
    echo "Open network, skipping password..."
    echo " "
fi

echo "Enable new config for network..."
wpa_cli -iwlan0 -p/tmp/recovery/sockets enable_network 0
echo " "
echo "Connect to network with new config..."
echo " "
sleep 3

echo "Checking connection..."
echo " "
STATUS=$(wpa_cli -iwlan0 -p/tmp/recovery/sockets status)
WPA_STATE=$(echo "$STATUS" | grep "wpa_state=" | cut -d= -f2)

if [ "$WPA_STATE" = "COMPLETED" ]; then
    echo "Wlan connect successfully!"
    echo " "
    echo "Getting dhcp ip..."
    echo " "
    DHCP_OUTPUT=$(dhcpcd wlan0 2>&1)
    IP_ADDR=$(echo "$DHCP_OUTPUT" | grep "leased" | head -1 | awk '{print $3}')
    if [ -n "$IP_ADDR" ]; then
		echo "Your ip address:"
        echo "$IP_ADDR"
        echo " "
        echo " "
        echo " "
    else
        echo "UNKNOW"
        echo " "
        echo " "
        echo " "
    fi
else
    echo "Wlan connect failed!"
    echo " "
    echo "Check SSID and password please!"
    echo " "
    echo " "
    echo " "
    exit 1
fi
