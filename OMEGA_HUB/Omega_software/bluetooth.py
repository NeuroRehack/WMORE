import pygatt
import time
import logging
import subprocess
from binascii import hexlify

# Enable debug logging for pygatt
logging.basicConfig()
logging.getLogger('pygatt').setLevel(logging.DEBUG)

# MAC address of the OpenLog Artemis
mac_address = "C0:C3:60:27:81:51"
# Handle of the characteristic you're reading from
characteristic_handle = 0x000c  # Replace with your characteristic handle

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
    except Exception as e:
        print(f"Error executing rtc2 script: {e}")

try:
    adapter.start()
    print(f"Connecting to device with MAC address {mac_address}...")
    device = adapter.connect(mac_address, timeout=10)
    print("Connection successful!")

    # Manually poll the characteristic every 5 seconds
    while True:
        poll_characteristic(device, characteristic_handle)
        get_and_print_rtc_time()
        time.sleep(5)  # Wait 5 seconds before polling again

except pygatt.exceptions.BLEError as e:
    print(f"BLE Error: {e}")
except Exception as e:
    print(f"An unexpected error occurred: {e}")
finally:
    print("Stopping adapter...")
    adapter.stop()

