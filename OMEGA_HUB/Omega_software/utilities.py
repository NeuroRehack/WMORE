import serial.tools.list_ports
import time
import threading
import os
import subprocess

def read_serial_data(ser):
    lines = []
    while True:
        try:
            data = ser.readline()
        except:
            break
        if not data:
            break
        line = data.decode('utf-8').strip()
        lines.append(line)
    return lines

def find_usb_serial_port():
    ports = serial.tools.list_ports.comports()
    devices = []
    for port in ports:
        if "USB" in port.description:
            devices.append(port.device)
    return devices

# check if the device is a logger or a coordinator
def is_logger(device):
    serial_connection = serial.Serial(device, baudrate=115200, timeout=1)
    time.sleep(5)
    
    # wait for response
    lines = read_serial_data(serial_connection)
    serial_connection.close()
    for line in lines:
        print(line)
        if "Coordinator" in line:
            serial_connection.close()
            return "Coordinator"
        elif "Logger" in line:
            serial_connection.close()
            return "Logger"
    return is_logger(device)

def run_zmodem_receive(device):
    # Change to the 'data' directory
    os.chdir('data')

    # Command to execute
    command = ['/root/WMORE/zmodemFileReceive', device]

    try:
        # Run the command and wait for it to complete
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error occurred: {e}")
    finally:
        # Change back to the original directory (optional, but recommended)
        os.chdir('..')

def send_command(command,serial):
        """Format input string and send it to serial device

        Args:
            command (str): the character or string to send to the device
        """
        command+="\n\r"
        serial.write(command.encode())
        

def get_file_list(device):
    serial_connection = serial.Serial(device, baudrate=115200, timeout=1)
    read_serial_data(serial_connection)
    cmd = ["m","s","dir","x","x"]
    for command in cmd:
        print(f">{command}")
        send_command(command,serial_connection)
        time.sleep(1)
        lines = read_serial_data(serial_connection)
        # [print(line) for line in lines]
        if command == "dir":
            files = [line for line in lines if ".bin" in line]
        time.sleep(1)
    serial_connection.close()
    return files

def format_device(device):
    serial_connection = serial.Serial(device, baudrate=115200, timeout=1)
    read_serial_data(serial_connection)
    cmd = ["m","s","fmt"]
    for command in cmd:
        print(f">{command}")
        send_command(command,serial_connection)
        time.sleep(1)
        read_serial_data(serial_connection)
        # [print(line) for line in lines]
        time.sleep(1)
    serial_connection.close()

if __name__ == "__main__":
    devices_dict = {}
    new_device_dict = {}
    while True:
        new_devices_list = find_usb_serial_port()
        print(devices_dict)
        #check for new connection
        for device in new_devices_list:
            if device not in devices_dict.keys():
                print("\nnew device connected !\n check if logger\n")
                device_type = is_logger(device)
                print(f"device is {device_type}\n")
                devices_dict[device] = [device_type]
        
        #check for disconnection
        for device in devices_dict.keys():
            if device not in new_devices_list:
                print("\ndevice disconnected !\n")
                new_device_dict = {k: v for k,v in devices_dict.items() if k != device}
        devices_dict = new_device_dict
        # check for whether to download or not
        for device in devices_dict.keys():
            if devices_dict[device][0] == "Logger":
                 files = get_file_list(device)
                 print(files)
                 if len(files) > 5:
                     run_zmodem_receive(device)
                     format_device(device)
            # if devices_dict[device][0] == "":
            #     device_type = is_logger(device)
            #     print(f"device is {device_type}\n")
                
        
        time.sleep(1)
                
    

