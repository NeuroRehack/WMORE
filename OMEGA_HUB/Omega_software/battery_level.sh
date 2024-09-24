#!/bin/sh
while true
do
    echo "$(date +%Y-%m-%d\ %H:%M:%S) - $(power-dock2 | grep 'Battery Voltage Level')" >> ~/WMORE/OMEGA_HUB/Omega_software/battery_log.txt
    sleep 20
done
