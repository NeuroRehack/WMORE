import pygatt
import time
import logging
import threading
import sys
import subprocess 
import datetime
from binascii import hexlify

# Enable debug logging for pygatt
logging.basicConfig()
logging.getLogger('pygatt').setLevel(logging.INFO)  # Set to DEBUG for detailed logs

# List of MAC addresses of the BLE devices (add as many as needed)
#mac_addresses = ["C0:C3:60:27:81:51", "C0:C3:0C:25:25:51"]
mac_addresses = ["C0:C3:0C:25:25:51"]  # Replace with your devices' MAC addresses

characteristic_handle = 0x000c  # Reading characteristic handle (assuming same for all devices)
send_handle_dev0 = 0x000e  # Writing characteristic handle (assuming same for all devices)
characteristic_handle_dev1 = 0x000c
#send_handle_dev1

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

                print(f"Device {device_id} - Polled value: {text_value}")

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

def rtc_function():
    """
    Executes the 'rtc2' -o file file and returns the RTC time as a string.
    """
    try:
        # Execute the shell script and capture the output
        result = subprocess.run(['./rtc2'], stdout=subprocess.PIPE)
        output = result.stdout.decode('utf-8').strip()
        print(f"RTC Output: {output}")

        # Extract the time from the output (assuming format "Current Time: HH:MM:SS")
        time_str = output.split(": ", 1)[1]
        return time_str
    except Exception as e:
        print(f"Error executing rtc2 script: {e}")
        return None




# def send_time_to_device(device, write_handle, time_str):
#     try:
#         # Convert time_str to a datetime object
#         time_obj = datetime.datetime.strptime(time_str, '%H:%M:%S')
#         # Convert to UNIX timestamp
#         timestamp = int(time_obj.timestamp())
#         # Convert to bytes (assuming 4-byte integer)
#         time_bytes = timestamp.to_bytes(4, byteorder='little')
#         device.char_write_handle(write_handle, time_bytes)
#         print(f"Sent timestamp {timestamp} to device.")
#     except Exception as e:
#         print(f"Error writing time to device: {e}")

try:
    adapter.start()
    print("Adapter started.")

    # List to keep track of threads
    threads = []

    # Start data rate monitor thread
    monitor_thread = threading.Thread(target=data_rate_monitor, daemon=True)
    monitor_thread.start()
    threads.append(monitor_thread)
    rtc_function()
    # Connect to each device and start polling in a separate thread
    for idx, mac_address in enumerate(mac_addresses):
        print(f"Connecting to device {idx} with MAC address {mac_address}...")
        try:
            device = adapter.connect(mac_address, timeout=10)
            print(f"Device {idx} - Connection successful!")

            # Optionally exchange MTU (may not increase in BLE 4.0)
            try:
                mtu = device.exchange_mtu(247)
                print(f"Device {idx} - MTU size exchanged: {mtu}")
            except Exception as e:
                print(f"Device {idx} - MTU exchange failed: {e}")

            # Start polling in a separate thread
            thread = threading.Thread(target=poll_characteristic, args=(device, idx), daemon=True)
            thread.start()
            threads.append(thread)
        except pygatt.exceptions.BLEError as e:
            print(f"Device {idx} - BLE Error: {e}")
        except Exception as e:
            print(f"Device {idx} - An unexpected error occurred: {e}")

    # Keep the main thread alive
    while True:
        time.sleep(1)

except KeyboardInterrupt:
    print("Script interrupted by user.")
except Exception as e:
    print(f"An unexpected error occurred: {e}")
finally:
    print("Stopping adapter...")
    try:
        adapter.stop()
    except Exception as e:
        print(f"Error stopping adapter: {e}")

