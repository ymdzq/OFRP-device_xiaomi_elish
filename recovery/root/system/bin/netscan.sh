#!/system/bin/sh
mkdir -p /tmp/wlan/list
rm /tmp/wlan/list/*
echo "Scanning..."
wpa_cli -iwlan0 -p/tmp/recovery/sockets scan >/dev/null 2>&1
wpa_cli -i wlan0 -p /tmp/recovery/sockets scan_results > /tmp/wlan/raw_list.txt
if [ $(wc -l < /tmp/wlan/raw_list.txt) -le 1 ]; then
    rm -f /tmp/wlan/list.txt
    echo "No networks found or scan error, exiting..."
    rm -f /tmp/wlan/raw_list.txt
    echo " "
    exit 0
fi
{
    tail -n +2 /tmp/wlan/raw_list.txt | while IFS= read -r line; do
        flags=$(echo "$line" | awk -F'[[:space:]]+' '{print $4}')
        ssid=$(echo "$line" | awk -F'[[:space:]]+' '{
            ssid_start = 0
            for (i=1; i<=NF; i++) {
                if (i <= 3) continue
                if (i > 4) {
                    ssid_start = i
                    break
                }
            }
            result = ""
            for (i=ssid_start; i<=NF; i++) {
                if (result == "") {
                    result = $i
                } else {
                    result = result " " $i
                }
            }
            print result
        }')
        
        if echo "$ssid" | grep -q '\\x'; then
            decoded_ssid=$(printf "%b" "$ssid" 2>/dev/null || echo "$ssid")
        else
            decoded_ssid="$ssid"
        fi
        
        if [ -n "$decoded_ssid" ]; then
            encryption=""
            if echo "$flags" | grep -q "SAE"; then
                encryption="WPA3"
            elif echo "$flags" | grep -q "WPA2"; then
                encryption="WPA2"
            elif echo "$flags" | grep -q "WPA-PSK"; then
                encryption="WPA"
            else
                encryption="OPEN"
            fi
            
            safe_filename=$(echo "$decoded_ssid" | tr -d '/\\:*?"<>|' | sed 's/^[ \t]*//;s/[ \t]*$//')
            
            if [ -z "$safe_filename" ]; then
                continue
            fi
            
            file_path="/tmp/wlan/list/${safe_filename}"
            echo "$encryption" > "$file_path"
        fi
    done
} > /dev/null

rm -f /tmp/wlan/raw_list.txt
echo "==============WLAN-LIST=============="
ls /tmp/wlan/list
echo "=============Scan Finish!============"
ls /tmp/wlan/list > /tmp/wlan/list.txt
echo " "
echo " "
echo " "