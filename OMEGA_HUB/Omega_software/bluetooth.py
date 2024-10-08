import pygatt
import time
import logging
import threading
import sys
import subprocess 
import datetime
from binascii import hexlify

# The objective of this script is to connect to BLE devices that are advertising IMU data,
# and store that information on board. Furthermore, the script needs to send the RTC time 
# that the MCU has access to, to the IMU/s to reconfigure their clocks to ensure accuracy 
# from drifts.

# Enable debug logging for pygatt
logging.basicConfig()
logging.getLogger('pygatt').setLevel(logging.INFO)  # Set to DEBUG for detailed logs

# List of MAC addresses of the BLE devices (add as many as needed)
# mac_addresses = ["C0:C3:60:27:81:51", "C0:C3:0C:25:25:51"]
mac_addresses = ["C0:C3:60:27:81:51"] 

characteristic_handle = 0x000c  # Reading characteristic handle (assuming same for all devices)
send_handle_dev0 = 0x000e      # Writing characteristic handle (assuming same for all devices)

# Initialize the adapter (ensure run_as_root=False if needed)
adapter = pygatt.GATTToolBackend()

# Global variables for data rate calculations
start_time = time.time()
total_bytes_received = 0
message_count = 0
measurement_interval = 5.0  # Measurement interval in seconds

# Lock for synchronizing access to global variables
data_lock = threading.Lock()

def poll_characteristic(device, device_id):
    """
    Polls the characteristic value manually at regular intervals for a given device.
    """
    global total_bytes_received, message_count
    try:
        while True:
            # Read the characteristic value manually using the handle
            value = device.char_read_handle(characteristic_handle)
            if value:
                text_value = value.decode('utf-8', errors='replace')
                bytes_received = len(value)

                # Synchronize access to global variables
                with data_lock:
                    total_bytes_received += bytes_received
                    message_count += 1

                #print(f"Device {device_id} - Polled value: {text_value}")

                # Write the value to log.txt
                with open(f"log_{device_id}.txt", "a") as log_file:
                    log_file.write(f"{text_value}\n")
            else:
                print(f"Device {device_id} - No value received during poll.")

            # Adjust sleep time if necessary
            time.sleep(0.005)  # Wait 5 milliseconds before polling again
    except pygatt.exceptions.BLEError as e:
        print(f"Device {device_id} - Error reading characteristic: {e}")
    except Exception as e:
        print(f"Device {device_id} - Unexpected error: {e}")
    finally:
        print(f"Device {device_id} - Disconnecting...")
        try:
            device.disconnect()
        except Exception as e:
            print(f"Device {device_id} - Error disconnecting: {e}")

def data_rate_monitor():
    """
    Monitors and prints the total data rate across all devices.
    """
    global total_bytes_received, message_count, start_time
    try:
        while True:
            time.sleep(measurement_interval)
            elapsed_time = time.time() - start_time

            with data_lock:
                # Calculate data rate
                data_rate_bps = (total_bytes_received * 8) / elapsed_time  # bits per second
                data_rate_kbps = data_rate_bps / 1000  # kilobits per second

                print(f"\nTotal Data rate: {data_rate_kbps:.2f} kbps over {elapsed_time:.2f} seconds")
                print(f"Total bytes received: {total_bytes_received} bytes")
                print(f"Total messages received: {message_count}\n")

                # Reset counters for next interval
                start_time = time.time()
                total_bytes_received = 0
                message_count = 0
    except Exception as e:
        print(f"Data rate monitor - Unexpected error: {e}")

# Just a quick and easy function converting the extracted RTC time from a string into an integer
# It is being converted to an integer to be sent to the IMU. This can be changed according to what
# ever the next person working on this wants.
def convert_time_to_int(time_str):
    try:
        parts = time_str.split(":")
        if len(parts) != 3:
            print(f"Invalid time format: {time_str}")
            return None
        hour = parts[0].zfill(2)
        minute = parts[1].zfill(2)
        second = parts[2].zfill(2)
        time_int = int(f"{hour}{minute}{second}")
        return time_int
    except Exception as e:
        print(f"Error converting time to integer: {e}")
        return None

def rtc_function():
    """
    Executes the 'rtc2' script and returns the RTC time as an integer.
    Converts the time from "HH:MM:SS" to an integer "HHMMSS" (e.g., "14:10:32" -> 141032).
    Also prints the converted integer for debugging.
    """
    try:
        # Execute the rtc2 script and capture the output
        # Ensure that 'rtc2' is in the same directory or provide the absolute path
        result = subprocess.run(['./rtc2'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
        # Get and print the stdout output
        output = result.stdout.decode('utf-8').strip()
        print(output)  # Debug print to see the raw output
        
        # Define the expected prefix in the rtc2 output
        expected_prefix = "Current time: "
        
        # Check if the expected prefix exists in the output
        if expected_prefix in output:
            # Extract the time string after the prefix
            time_str = output.split(expected_prefix, 1)[1]
            print(f"Extracted Time String: {time_str}")  # Debug print
            
            # Convert the time string to an integer
            rtc_time_int = convert_time_to_int(time_str)
            print(f"Converted RTC Time to Integer: {rtc_time_int}")  # Debug print
            return rtc_time_int
        else:
            print(f"Expected prefix '{expected_prefix}' not found in RTC output.")
            return None
    except FileNotFoundError:
        print("Error: 'rtc2' script not found. Ensure it exists and the path is correct.")
        return None
    except Exception as e:
        print(f"Unexpected error in rtc_function: {e}")
        return None

def send_time_to_device(device, write_handle, time_int):
    """
    Sends the integer RTC time to the device via the specified write handle.
    """
    try:
        # Convert the integer to bytes
        time_bytes = time_int.to_bytes(4, byteorder='little')
        device.char_write_handle(write_handle, time_bytes, wait_for_response=True)
        print(f"Sent RTC time {time_int} to device.")
    except Exception as e:
        print(f"Error writing RTC time to device: {e}")

#This is the trigger bit that Nathaneal and I have agreed upon, any future work on this
# will probably require a more secure handshake. In this case we are just waiting for a
#-1 to be send to the IMU.
def send_trigger_to_device(device, write_handle, trigger_int):
    """
    Sends an integer trigger to the device via the specified write handle.
    """
    try:
        # Convert the integer to bytes
        # Assuming the IMU expects a 4-byte integer for -1
        trigger_bytes = trigger_int.to_bytes(4, byteorder='little', signed=True)
        device.char_write_handle(write_handle, trigger_bytes, wait_for_response=True)
        print(f"Sent trigger {trigger_int} to device.")
    except Exception as e:
        print(f"Error sending trigger to device: {e}")

try:
    adapter.start()
    print("Adapter started.")

    # List to keep track of threads
    threads = []

    # Start data rate monitor thread
    monitor_thread = threading.Thread(target=data_rate_monitor, daemon=True)
    monitor_thread.start()
    threads.append(monitor_thread)

    # Connect to each device and start polling in a separate thread
    for idx, mac_address in enumerate(mac_addresses):
        device_id = mac_address.replace(":", "")
        print(f"Connecting to device {idx} with MAC address {mac_address}...")
        try:
            device = adapter.connect(mac_address, timeout=10)
            print(f"Device {device_id} - Connection successful!")

            # Optionally exchange MTU (may not increase in BLE 4.0)
            try:
                mtu = device.exchange_mtu(247)
                print(f"Device {device_id} - MTU size exchanged: {mtu}")
            except Exception as e:
                print(f"Device {device_id} - MTU exchange failed: {e}")

            # Send RTC time as integer
            rtc_time_int = rtc_function()
            if rtc_time_int is not None:
                send_time_to_device(device, send_handle_dev0, rtc_time_int)
                # Send -1 to prompt the IMU to transmit its data
                send_trigger_to_device(device, send_handle_dev0, -1)
            else:
                print(f"Device {device_id} - RTC time not sent due to conversion failure.")

            # Start polling in a separate thread
            thread = threading.Thread(target=poll_characteristic, args=(device, device_id), daemon=True)
            thread.start()
            threads.append(thread)
        except pygatt.exceptions.BLEError as e:
            print(f"Device {device_id} - BLE Error: {e}")
        except Exception as e:
            print(f"Device {device_id} - An unexpected error occurred: {e}")

    # Keep the main thread alive
    while True:
        time.sleep(1)

except KeyboardInterrupt:
    print("Script interrupted by user.")
except Exception as e:
    print(f"An unexpected error occurred: {e}")
finally:
    print("Stopping adapter...")
    adapter.stop()
       