import serial.tools.list_ports
import time
import threading
import os
import subprocess
import json
import requests
import pickle



CONNECTED = 0   # Device has only just been connected
BUSY = 1        # Files being downloaded or rtc being set
CHARGING = 2    # Files downloaded rtc set
ERROR = 3

COORDINATOR = 0
LOGGER = 1
DEBUG = 1

class DeviceObject:
    def __init__(self, comport, type, status, id):
        self.comport = comport
        self.type = type
        self.status = status
        self.id = id
        self.ser = None
        
    def connect(self):
        try:
            self.ser = serial.Serial(self.comport, baudrate=115200, timeout=1)
            print(f"connected to {self.comport}")
        except:
            print("error connecting to device")
            self.ser = None
    
    def disconnect(self):
        try:
            self.ser.close()
            self.ser = None
        except:
            print("error disconnecting from device")
            self.ser = None
        
    
    def format_device(self):
        if self.ser is None:
            self.connect()
        read_serial_data(self.ser)
        cmd = ["x","s","fmt"]
        for command in cmd:
            send_command(command,self.ser)
            time.sleep(1)
            read_serial_data(self.ser)
            time.sleep(1)
        time.sleep(5)
        self.disconnect()
    
    def get_id(self):
        print("Get device id\n")
        type = None
        if self.ser is None:
            self.connect()
        cmd = ["x","1","x"]
        i = 0
        while i < len(cmd):
            command = cmd[i]
            send_command(command,self.ser)
            lines = read_serial_data(self.ser)
            for line in lines:
                if "ID" in line:
                    deviceID = int(line.split(" ")[-1])
                    self.id = deviceID
                    return deviceID
            if i == 0:
                found = False
                for line in lines:
                    if "Menu: Main Menu" in line:
                        found = True
                        break
                if not found:
                    i-=1
            i+=1
        return deviceID
    
    # check if the device is a logger or a coordinator
    def get_type(self):
        print("Get device type\n")
        type = None
        if self.ser is None:
            self.connect()
        cmd = ["i","i","i"]
        for command in cmd:
            send_command(command,self.ser)
            lines = read_serial_data(self.ser)
            for line in lines:
                if "Coordinator" in line:
                    self.type = COORDINATOR
                    return
                elif "Logger" in line:
                    self.type = LOGGER
                    return
        
    def get_file_list(self):
        print("get logger file list")
        if self.ser is None:
            self.connect()
        read_serial_data(self.ser)
        cmd = ["x","s","dir","x","x"]
        i = 0
        while i < len(cmd):
            command = cmd[i]
            send_command(command,self.ser)
            lines = read_serial_data(self.ser)
            if i == 0:
                found = False
                for line in lines:
                    if "Menu: Main Menu" in line:
                        found = True
                        break
                if not found:
                    i-=1
                
            if command == "dir":
                # ignore .txt (settings) files
                files = [line for line in lines if ".bin" in line]
                filesToDelete = [file.split(" ")[-1] for file in files if not int(file.split(" ")[-2])]
                # ignore empty files
                files = [file for file in files if int(file.split(" ")[-2])]

                nbFiles = len(filesToDelete)
                for k,file in enumerate(filesToDelete): 
                    command = f"del {file}"
                    send_command(command, self.ser)
                    lines = read_serial_data(self.ser)
                    nbFilesLeft = nbFiles - k
                    print(f"\n\t\t\t\t{k}/{nbFiles} - eta:{nbFilesLeft}")                    
            i+=1
        return files
    
    def to_dict(self):
        status_dict = {CONNECTED:"connected", BUSY:"busy", CHARGING:"charging"}
        type_dict = {COORDINATOR : "Coordinator",LOGGER : "Logger"}
        return {
            "comport": self.comport, 
            "type": type_dict[self.type], 
            "status": status_dict[self.status], 
            "id": self.id
            }


def read_serial_data(ser):
    lines = []
    i = 0
    # while True:
    #     data = ser.read_all()
    #     if not data:
    #         i+=1
    #         if i > 1:
    #             break
    #     try:
    #         lines =  [lines.append(line) for line in data.decode().strip().split("\n") if line != ""]
    #         # data.decode().strip().split("\n")
    #     except:
    #         pass
    
    while True:
        try:
            data = ser.readline()
            if not data:
                i+=1
                if i >1:
                    break
            else:
                i = 0
                try:
                    line = data.decode('utf-8').strip()
                except:
                    line = []
                lines.append(line)
        except:
            pass
    [print(line) for line in lines if DEBUG]
    return lines

def run_zmodem_receive(deviceObj, maxRetries=3):
    # currtime YYMMDD
    currTime = time.strftime("%y%m%d")
    
    folderName = f"{currTime}_{deviceObj.id}"
    folderPath = f"/root/WMORE/data/{folderName}"
    # create a directory for the device if it doesn't exist
    if not os.path.exists(folderPath):
        os.mkdir(folderPath)
    # Change to the directory containing the files to download
    os.chdir(folderPath)
    
    # Command to execute
    command = ['/root/WMORE/zmodemFileReceive', deviceObj.comport]

    try:
        # Run the command and wait for it to complete
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error occurred: {e}")
        if maxRetries > 0:
            time.sleep(10)
            run_zmodem_receive(deviceObj, maxRetries-1)
    finally:
        # Change back to the original directory (optional, but recommended)
        os.chdir('/root/WMORE')

def send_command(command,serial):
        """Format input string and send it to serial device

        Args:
            command (str): the character or string to send to the device
        """
        if DEBUG:
            print(f"\n\t\t\t\tsending: {command}\n")
        command+="\n\r"
        serial.write(command.encode())
        time.sleep(1)
        

def check_for_new_device(connected_devices_list, devices_dict):
    connectionChanged = False
    for device in connected_devices_list:
        if device not in devices_dict.keys():
            deviceObj = DeviceObject(comport=device, type=None,status=CONNECTED, id=None)
            print(f"\nnew device connected !: {device}\n")
            deviceObj.get_type()
            typeToPrint = "Logger" if deviceObj.type == LOGGER else "Coordinator" 
            print(f"\ndevice is {typeToPrint}\n")
            deviceObj.get_id()
            devices_dict[device] = deviceObj
            connectionChanged = True
    if connectionChanged:
        send_update(devices_dict)
    return devices_dict

def check_for_device_disconnection(connected_devices_list, devices_dict):
    connectionChanged = False
    new_device_dict = devices_dict
    for device in devices_dict.keys():
        if device not in connected_devices_list:
            print("\ndevice disconnected !\n")
            # Only add connected devices to new connected devices dict
            new_device_dict = {k: v for k,v in devices_dict.items() if k != device}
            connectionChanged = True
    if connectionChanged:
        send_update(new_device_dict)
    return new_device_dict

def get_usb_device_list():
    ports = serial.tools.list_ports.comports()
    devices = []
    for port in ports:
        if "USB" in port.description:
            devices.append(port.device)
    return devices


def send_update(devices_dict):
        # serialize data of the dict.values()
    serialized_data = json.dumps([deviceObj.to_dict() for deviceObj in devices_dict.values()])

    url = "http://localhost:5000/receive_data"
    headers = {'Content-Type': 'application/json'}
    try:
        response = requests.post(url, data=serialized_data, headers=headers)
        print("info sent to server\n")
        lastupload = time.time()        
    except:
        print("server not connected\n")

if __name__ == "__main__":
    devices_dict = {}
    lastupload = time.time()
    while True:
        now = time.time()
        connected_devices_list = get_usb_device_list()
        #check for new connection
        devices_dict = check_for_new_device(connected_devices_list, devices_dict)
        #check for disconnection
        devices_dict = check_for_device_disconnection(connected_devices_list, devices_dict)
        [print(deviceObj.to_dict()) for deviceObj in devices_dict.values()]
        
            
        # check for whether to download or not
        for device in devices_dict.keys():
            deviceObj = devices_dict[device] 
            if deviceObj.type == LOGGER and deviceObj.status == CONNECTED:
                files = deviceObj.get_file_list()
                print(files)
                
                run_zmodem_receive(deviceObj,3)
                # format_device(device)
            
                deviceObj.status = CHARGING
                send_update(devices_dict)

        
        time.sleep(1)
                
    
