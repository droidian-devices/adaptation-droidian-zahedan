#!/bin/bash

modules=(
trace_mmstat
kheaders
lcd
met
conninfra
wmt_chrdev_wifi
wlan_drv_gen4m
bt_drv_6877
gps_drv
fmradio_drv_connac2x
connfem
focaltech_tp
mt6357-accdet
mt6359-accdet
tcp_westwood
tcp_htcp
)

for name in "${modules[@]}"; do
    modprobe -f "$name"
done

echo 1 > /dev/wmtWifi

exit 0
