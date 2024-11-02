
#Attempting parallel connections one last time, the bluetooth.py script was using the pygatt
#backend, which didnt allow for multiple connections. I have tried now with bleak, which is a 
#different library. As far as I can tell 3 IMU's work.
import asyncio
from bleak import BleakClient
import subprocess
import argparse
import logging
from binascii import hexlify
import sys
import time
from collections import defaultdict  # Added import

# Enable debug logging for bleak and the script
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

#The UUID's for read and write are the same for both IMU's, and i belive
#it should be the same for any extra IMU's connected assuming they are flashed
#using the same arduino code that Nathaneal usd.
CHARACTERISTIC_UUID_READ = "18ee1516-016b-4bec-ad96-bcb96d166e9a"    # Reading characteristic UUID
CHARACTERISTIC_UUID_WRITE = "18ee1516-016b-4bec-ad96-bcb96d166e9b"   # Writing characteristic UUID

def parse_arguments():
    parser = argparse.ArgumentParser(description="BLE IMU Data Collector using Bleak")
    parser.add_argument(
        "mac_addresses",
        nargs='+',
        type=str,
        help="MAC addresses of the BLE devices (e.g., C0:C3:60:27:81:51 C0:C3:0C:25:25:51)"
    )
    return parser.parse_args()

def convert_time_to_int(time_str):
    """
    Converts a time string in "HH:MM:SS" format to an integer "HHMMSS".
    For example, "14:15:30" becomes 141530.
    """
    try:
        parts = time_str.split(":")
        if len(parts) != 3:
            logger.error(f"Invalid time format: {time_str}")
            return None
        hour = parts[0].zfill(2)
        minute = parts[1].zfill(2)
        second = parts[2].zfill(2)
        time_int = int(f"{hour}{minute}{second}")
        return time_int
    except Exception as e:
        logger.error(f"Error converting time to integer: {e}")
        return None

async def rtc_function():
    """
    Executes the 'rtc2' script and returns the RTC time as an integer.
    """
    try:
        # Execute the rtc2 script and capture the output
        result = subprocess.run(['./rtc2'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        # Check if the rtc2 script executed successfully
        if result.returncode != 0:
            logger.error(f"rtc2 script failed with error: {result.stderr.decode('utf-8').strip()}")
            return None
        
        # Get and print the stdout output
        output = result.stdout.decode('utf-8').strip()
        logger.info(f"RTC Output: {output}")  # Debug print to see the raw output
        
        # Define the expected prefix in the rtc2 output
        expected_prefix = "Current time: "
        
        # Check if the expected prefix exists in the output
        if expected_prefix in output:
            # Extract the time string after the prefix
            time_str = output.split(expected_prefix, 1)[1]
            logger.info(f"Extracted Time String: {time_str}")  # Debug print
            
            # Convert the time string to an integer
            rtc_time_int = convert_time_to_int(time_str)
            if rtc_time_int is not None:
                logger.info(f"Converted RTC Time to Integer: {rtc_time_int}")  # Debug print
                return rtc_time_int
            else:
                logger.error("Failed to convert RTC time string to integer.")
                return None
        else:
            logger.error(f"Expected prefix '{expected_prefix}' not found in RTC output.")
            return None
    except FileNotFoundError:
        logger.error("Error: 'rtc2' script not found. Ensure it exists and the path is correct.")
        return None
    except Exception as e:
        logger.error(f"Unexpected error in rtc_function: {e}")
        return None

def calculate_bit_rate(data_size_bytes, transmission_interval_sec):
    """
    Calculates the bit rate in kbps. This function is different from
    the total_bit_rate func below
    """
    data_size_bits = data_size_bytes * 8
    if transmission_interval_sec == 0:
        return 0
    transmission_frequency = 1 / transmission_interval_sec  # transmissions per second
    bit_rate_bps = data_size_bits * transmission_frequency
    bit_rate_kbps = bit_rate_bps / 1000
    return bit_rate_kbps

async def send_time_and_trigger(client, device_id):
    """
    Sends RTC time and "-1" trigger to the device. This trigger
    can and should be updated to make a more secure handshake, however
    for testing purposes the trigger has been set as -1
    """
    # Send RTC time
    rtc_time_int = await rtc_function()
    if rtc_time_int is not None:
        try:
            # Convert the integer to bytes (4 bytes, little-endian)
            time_bytes = rtc_time_int.to_bytes(4, byteorder='little', signed=False)  # Adjust as per IMU requirements
            await client.write_gatt_char(CHARACTERISTIC_UUID_WRITE, time_bytes, response=True)
            logger.info(f"Device {device_id} - Sent RTC time {rtc_time_int}.")
        except Exception as e:
            logger.error(f"Device {device_id} - Error sending RTC time: {e}")
    else:
        logger.error(f"Device {device_id} - RTC time not sent due to conversion failure.")
    
    # Send trigger -1
    try:
        trigger_int = -1
        trigger_bytes = trigger_int.to_bytes(4, byteorder='little', signed=True)
        await client.write_gatt_char(CHARACTERISTIC_UUID_WRITE, trigger_bytes, response=True)
        logger.info(f"Device {device_id} - Sent trigger {trigger_int}.")
    except Exception as e:
        logger.error(f"Device {device_id} - Error sending trigger: {e}")

async def poll_characteristic(client, device_id, total_bit_rate_dict):
    """
    Polls the characteristic value at regular intervals for the connected device.
    """
    try:
        while True:
            start_time = time.time()
            value = await client.read_gatt_char(CHARACTERISTIC_UUID_READ)
            end_time = time.time()
            transmission_interval_sec = end_time - start_time  # Time taken for one transmission
            
            if value:
                try:
                    text_value = value.decode('utf-8', errors='replace')
                except UnicodeDecodeError:
                    text_value = hexlify(value).decode('utf-8')
                
                data_size_bytes = len(value)
                bit_rate_kbps = calculate_bit_rate(data_size_bytes, transmission_interval_sec)
                
                # **Update the latest bit rate instead of accumulating**
                total_bit_rate_dict[device_id] = bit_rate_kbps
                
                # Log the received value and bit rate
                with open(f"log_{device_id}.txt", "a") as log_file:
                    log_file.write(f"{text_value}\n")
                logger.info(f"Device {device_id} - Received data: {text_value} | Bit Rate: {bit_rate_kbps:.2f} kbps")
            else:
                logger.warning(f"Device {device_id} - No value received during poll.")
            
            await asyncio.sleep(0.5)  # Polling interval
    except Exception as e:
        logger.error(f"Device {device_id} - Error during polling: {e}")
    finally:
        logger.info(f"Device {device_id} - Disconnecting...")
        await client.disconnect()

async def handle_device(mac_address, total_bit_rate_dict, max_retries=5, retry_delay=10):
    """
    Handles connection, data transmission, and polling for a single BLE device.
    Does attempt to reconnect if it can find the characteristic again.
    """
    device_id = mac_address.replace(":", "")
    logger.info(f"Device {device_id} - Attempting to connect...")
    
    for attempt in range(1, max_retries + 1):
        try:
            client = BleakClient(mac_address)
            await client.connect()
            if client.is_connected:
                logger.info(f"Device {device_id} - Connection successful!")
                break
            else:
                logger.error(f"Device {device_id} - Connection failed on attempt {attempt}.")
        except Exception as e:
            logger.error(f"Device {device_id} - Connection attempt {attempt} failed: {e}")
        
        if attempt < max_retries:
            logger.info(f"Device {device_id} - Retrying in {retry_delay} seconds...")
            await asyncio.sleep(retry_delay)
        else:
            logger.error(f"Device {device_id} - All connection attempts failed.")
            return
    
    #This cheeky if statement checks if the client is connected, otherwise returns out of the loop
    if not client.is_connected:
        return
    
    try:
        #RTC time and trigger
        await send_time_and_trigger(client, device_id)
        
        #Polling
        await poll_characteristic(client, device_id, total_bit_rate_dict)
    
    except Exception as e:
        logger.error(f"Device {device_id} - Unexpected error: {e}")
    finally:
        if client.is_connected:
            await client.disconnect()
            logger.info(f"Device {device_id} - Disconnected.")

async def log_total_bit_rate(total_bit_rate_dict):
    """
    Periodically logs the total bit rate from all devices.
    """
    while True:
        total = sum(total_bit_rate_dict.values())
        logger.info(f"Total Transmission Bit Rate: {total:.2f} kbps")
        await asyncio.sleep(60)  # Log every 60 seconds

async def main(mac_addresses):
    """
    Manages multiple BLE device connections concurrently and logs total bit rate.
    """
    tasks = []
    # Initialize as defaultdict with float to store bit rates
    total_bit_rate_dict = defaultdict(float)
    
    for mac in mac_addresses:
        logger.info(f"Starting BLE handler for MAC address {mac}...")
        task = asyncio.ensure_future(handle_device(mac, total_bit_rate_dict))  # Use ensure_future for Python 3.6+
        tasks.append(task)
        await asyncio.sleep(1)  # Small delay to stagger the connections
    
    logger.info("All BLE handlers started.")
    
    # Start a separate task to log total bit rate periodically
    bit_rate_task = asyncio.ensure_future(log_total_bit_rate(total_bit_rate_dict))
    tasks.append(bit_rate_task)
    
    # Await all tasks
    await asyncio.gather(*tasks, return_exceptions=True)

if __name__ == "__main__":
    args = parse_arguments()
    mac_addresses = [mac.upper() for mac in args.mac_addresses]
    
    try:
        # Check if asyncio.run exists (Python 3.7+) 
        #Currently the omega 2 board im using has python 3.6, but 
        # feel free to remove either the if or else statement if the next p
        # person to work on this research has access to a more modern pythoN.
        if hasattr(asyncio, 'run'):
            asyncio.run(main(mac_addresses))
        else:
            # For Python 3.6
            loop = asyncio.get_event_loop()
            loop.run_until_complete(main(mac_addresses))
            loop.close()
    except KeyboardInterrupt:
        logger.info("Script interrupted by user.")
    except Exception as e:
        logger.error(f"An unexpected error occurred in main: {e}")