import pygatt
import time
import logging
import subprocess
from binascii import hexlify

# Enable debug logging for pygatt
logging.basicConfig()
logging.getLogger('pygatt').setLevel(logging.DEBUG)

# MAC address of the OpenLog Artemis (for future users just put in whatever your MAC address is)
mac_address = "C0:C3:60:27:81:51"

characteristic_handle = 0x000c  # Reading characteristic handle
send_handle = 0x000e #The writing characteristic handle

adapter = pygatt.GATTToolBackend()

def poll_characteristic(device, characteristic_handle):
    """
    Polls the characteristic value manually at regular intervals.
    """
    try:
        # Read the characteristic value manually using the handle
        value = device.char_read_handle(characteristic_handle)
        if value:
            hex_value = hexlify(value).decode('utf-8')
            print(f"Polled value: {hex_value}")

            # Write the value to log.txt
            with open("log.txt","a") as log_file:
                log_file.write(f"{hex_value}\n")
        else:
            print("No value received during poll.")
    except pygatt.exceptions.BLEError as e:
        print(f"Error reading characteristic: {e}")
    except Exception as e:
        print(f"Unexpected error: {e}")

def get_and_print_rtc_time():
    """
    Executes the 'rtc2' shell script and prints the RTC time.
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

#sending time to a ble device 
def send_time(device, write_handle, time_str):
    try:
        # Convert the time string to bytes (ensure the format matches device expectations)
        time_bytes = time_str.encode('utf-8')
        device.char_write_handle(write_handle, time_bytes)
        print(f"Sent time {time_str} to device.")
    except Exception as e:
        print(f"Error writing time to device: {e}")

try:
    adapter.start()
    print(f"Connecting to device with MAC address {mac_address}...")
    device = adapter.connect(mac_address, timeout=10)
    print("Connection successful!")

    # Manually poll the characteristic every 5 seconds
    while True:
        poll_characteristic(device, characteristic_handle)
        time_str = get_and_print_rtc_time()
        if time_str:
            send_time(device, send_handle, time_str)
        time.sleep(5)  # Wait 5 seconds before polling again

except pygatt.exceptions.BLEError as e:
    print(f"BLE Error: {e}")
except Exception as e:
    print(f"An unexpected error occurred: {e}")
finally:
    print("Stopping adapter...")
    adapter.stop()

