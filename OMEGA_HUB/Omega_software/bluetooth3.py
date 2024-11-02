#!/usr/bin/env python3

import asyncio
from bleak import BleakClient
import subprocess
import argparse
import logging
from binascii import hexlify
import sys
import time
from collections import defaultdict
from bluezero import peripheral

# Enable debug logging for bleak and the script
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# Define UUIDs
SERVICE_UUID = "18ee1516-016b-4bec-ad96-bcb96d166e999"
CHARACTERISTIC_UUID_WRITE = "18ee1516-016b-4bec-ad96-bcb96d166e9b"  # Writable characteristic
CHARACTERISTIC_UUID_READ = "18ee1516-016b-4bec-ad96-bcb96d166e9a"   # Readable characteristic

# Initialize BLE Peripheral
ble_service = peripheral.Service(uuid=SERVICE_UUID, primary=True)

# Define Write Characteristic
write_characteristic = peripheral.Characteristic(
    uuid=CHARACTERISTIC_UUID_WRITE,
    flags=["write"],
    write_callback=None  # We'll define it later
)

# Define Read Characteristic (Optional, if needed)
read_characteristic = peripheral.Characteristic(
    uuid=CHARACTERISTIC_UUID_READ,
    flags=["read", "notify"],
    read_callback=None  # Define if needed
)

# Add characteristics to the service
ble_service.add_characteristic(write_characteristic)
ble_service.add_characteristic(read_characteristic)

# Create Peripheral
ble_peripheral = peripheral.Peripheral(
    adapter_addr=None,  # Default adapter
    device_addr=None,   # Default device address
    local_name="Omega_BLE_Server",
    services=[ble_service]
)

# Define write callback
def write_callback(value, options):
    received_data = value.decode('utf-8', errors='replace')
    logger.info(f"Received data: {received_data}")
    # Process the received data as needed

# Assign the write callback
write_characteristic.write_callback = write_callback

# Define read callback (Optional)
def read_callback(options):
    # Provide data to read if needed
    return "Readable data"

# Assign the read callback (Optional)
read_characteristic.read_callback = read_callback

async def main():
    logger.info("Starting BLE GATT server on Omega...")
    ble_peripheral.publish()
    logger.info("BLE GATT server is now running and advertising.")

    try:
        while True:
            await asyncio.sleep(1)  # Keep the server running
    except KeyboardInterrupt:
        logger.info("Stopping BLE GATT server.")
        ble_peripheral.unpublish()

if __name__ == "__main__":
    asyncio.run(main())

