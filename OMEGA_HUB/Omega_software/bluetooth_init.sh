#!/bin/bash

# Script to initialize Bluetooth on the Omega 2 Pro 
# THIS SCRIPT IS FOR GIT REPOSITORY DEMONSTRATION. Place this file inside /etc/init.d/ to run on 
# boot. 

# Bring up the Bluetooth device
hciconfig hci0 up

# Enable Simple Secure Pairing (SSP) mode
hciconfig hci0 sspmode enable

# Enable page scan to allow device discovery
hciconfig hci0 piscan

# Set Bluetooth to be discoverable and advertise
bluetoothctl << EOF
power on
agent on
discoverable on
pairable on
advertise on
scan on
EOF
