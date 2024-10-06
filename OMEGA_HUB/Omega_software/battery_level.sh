#!/bin/sh

#THIS SHELL FILE IS FOR DEMONSTRATION PURPOSES FOR THE GIT, IF USED LATER ON PLEASE ENSURE THAT THE SCRIPT IS 
#PLACED IN THE usr/bin directory, and ensure that the etc/rc.local file is edited to include the this shell 
#script and its startup in the background - Adi 

LOG_DIR="/root/WMORE/OMEGA_HUB/Omega_software"
LOG_FILE="$LOG_DIR/battery_log.txt"
POWER_DOCK_CMD="/usr/bin/power-dock2"  # Update if power-dock2 is located elsewhere

# Ensure the logging directory exists
mkdir -p "$LOG_DIR"

while true
do
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $($POWER_DOCK_CMD | grep 'Battery Voltage Level')" >> "$LOG_FILE"
    sleep 120
done
