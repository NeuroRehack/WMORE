#!/usr/bin/env python3
import asyncio
import subprocess
import sys
import os
import time
import logging
from bluetooth2 import handle_device  # Ensure bluetooth2.py is in the same directory

# Enable debug logging for the wrapper
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

# List of MAC addresses of the BLE devices
mac_addresses = [
    "C0:C3:60:27:81:51",  # Device 1
    "C0:C3:0C:25:25:51",  # Device 2
    # Add more MAC addresses as needed
]

async def main():
    tasks = []
    for mac in mac_addresses:
        logger.info(f"Starting BLE handler for MAC address {mac}...")
        task = asyncio.ensure_future(handle_device(mac))
        tasks.append(task)
        await asyncio.sleep(1)  # Small delay to stagger the connections
    
    logger.info("All BLE handlers started.")
    
    # Await all tasks
    await asyncio.gather(*tasks, return_exceptions=True)

if __name__ == "__main__":
    try:
        loop = asyncio.get_event_loop()
        loop.run_until_complete(main())
        loop.close()
    except KeyboardInterrupt:
        logger.info("Wrapper script interrupted by user.")
    except Exception as e:
        logger.error(f"An unexpected error occurred in the wrapper script: {e}")


