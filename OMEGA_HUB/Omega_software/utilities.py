import serial.tools.list_ports
import time
import threading
import os
import subprocess
import json
import requests
import pickle
from tqdm import tqdm
import queue
import logging
import datetime
from backup_to_drive import upload, is_internet_available

CONNECTED = 0   # Device has only just been connected
BUSY = 1        # Files being downloaded or rtc being set
CHARGING = 2    # Files downloaded rtc set
ERROR = 3

COORDINATOR = 0 # device is a coordinator
LOGGER = 1 # device is a logger
DEBUG = 0 # set to 1 to print serial data
ROOT_PATH = "/root/WMORE"
DATA_PATH = os.path.join(ROOT_PATH, "data")
LOG_FILE = os.path.join(ROOT_PATH, "logs" ,"log.txt")


TIME_ZONE = 10 #Australia/Sydney

RTC_COMMANDS = [
    'm',  
    '2',  
    '4',  
    "year",  # will be replaced by the year
    "month",  # will be replaced by the month
    "day",  # will be replaced by the day
    '6',  
    "hour",  # will be replaced by the hour
    "minute",  # will be replaced by the minute
    "second",  # will be replaced by the second
    'x'  
]
FORMAT_COMMANDS = [
    'm', 
    's', 
    'fmt'  
]
ID_COMMANDS = [
    'x',
    '1',
    'x'
]
TYPE_COMMANDS = [
    'i',
    'i',
    'i'
]

FILE_LIST_COMMANDS = [
    'x',
    's',
    'dir',
    'x',
    'x'
]

class WMOREFILE():
    def __init__(self, date= None, time= None, size = None, filename = None, path = None):
        self.filename = filename
        self.path = path
        self.size = int(size) if size is not None else None
        self.date = date
        self.time = time
    def print_file(self):
        print(f"filename: {self.filename}, size: {self.size}, date: {self.date}, time: {self.time}")

class DeviceObject:
    def __init__(self, comport, type, status, id):
        self.comport = comport
        self.type = type
        self.status = status
        self.id = id
        self.ser = None
        self.files = None
        logging.info(f"new device connected: {self.comport}")

        self.type_dict = {COORDINATOR : "Coordinator",LOGGER : "Logger", None : None}
        
        
    def set_download_path(self):
        type_dict = {COORDINATOR : "Coordinator",LOGGER : "Logger", None : None}
        if self.id is None:
            self.get_id()
        # set the download path
        currTime = time.strftime("%y%m%d")
        folderName = os.path.join(f"{type_dict[self.type]}_{self.id}", str(currTime))#f"{currTime}_{self.id}"
        self.download_path = os.path.join(DATA_PATH, folderName)
        path = ""
        for folder in os.path.split(self.download_path):
            path = os.path.join(path, folder)
            if not os.path.exists(path):
                os.mkdir(path)

    def get_info(self):
        """Get the device type, id and download path
        """
        self.status = BUSY
        logging.info(f"{self.comport} device status: {self.status}")
        self.get_type()
        logging.info(f"{self.comport} device type: {self.type_dict[self.type]}")
        self.get_id()
        logging.info(f"{self.comport} device id: {self.id}")
        self.set_download_path()
        if self.type == LOGGER:
            self.download_files()
        elif self.type == COORDINATOR:
            self.set_rtc()
        self.status = CHARGING
        logging.info(f"{self.comport} device status: {self.status}")
        return
        
    def connect(self):
        """Connect to the device
        """
        try:
            self.ser = serial.Serial(self.comport, baudrate=115200, timeout=1)
            print(f"connected to {self.comport}")
        except:
            print("error connecting to device")
            self.ser = None
    
    def disconnect(self):
        """Disconnect from the device
        """
        try:
            self.ser.close()
            self.ser = None
        except:
            print("error disconnecting from device")
            self.ser = None
        
    def format_device(self):
        """Format the sd card on the device
        """
        if self.ser is None:
            self.connect()
        read_serial_data(self.ser)
        for command in FORMAT_COMMANDS:
            send_command(command,self.ser)
            time.sleep(1)
            read_serial_data(self.ser)
            time.sleep(1)
        time.sleep(5)
        self.disconnect()
        logging.info(f"Logger {self.id} formatted")
    
    def get_id(self):
        """Get the device id

        Returns:
            deviceID (int): the device id
        """
        print("Get device id\n")
        type = None
        if self.ser is None:
            self.connect()
        i = 0
        while i < len(ID_COMMANDS):
            command = ID_COMMANDS[i]
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
    
    def get_type(self):
        """Get the device type (logger or coordinator)
        """
        print("Get device type\n")
        type = None
        if self.ser is None:
            self.connect()
        
        for command in TYPE_COMMANDS:
            send_command(command,self.ser)
            lines = read_serial_data(self.ser)
            for line in lines:
                if "Coordinator" in line:
                    self.type = COORDINATOR
                    return
                elif "Logger" in line:
                    self.type = LOGGER
                    return
    def set_rtc(self):
        """Set the rtc on the device
        """
        date_time_components = {
            'year': '%y',
            'month': '%m',
            'day': '%d',
            'hour': '%H',
            'minute': '%M',
            'second': '%S'
        }
        print("Set RTC\n")
        if self.ser is None:
            self.connect()
        read_serial_data(self.ser)
        for command in RTC_COMMANDS:
            if command in date_time_components.keys():
                dt = datetime.datetime.now() + datetime.timedelta(hours=TIME_ZONE)
                command = dt.strftime(date_time_components[command])
                
            send_command(command,self.ser)
            time.sleep(1)
            read_serial_data(self.ser)
            time.sleep(1)
        time.sleep(5)
        self.disconnect()
        logging.info(f"Coordinator {self.id} rtc set to {dt}")
        
    def get_file_list(self):
        """Get the list of files on the device
        """
        print("get logger file list")
        if self.ser is None:
            self.connect()
        read_serial_data(self.ser)
        i = 0
        while i < len(FILE_LIST_COMMANDS):
            command = FILE_LIST_COMMANDS[i]
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
                # ignore files not ending in .bin or .txt
                files = [line for line in lines if (line.endswith(".bin") or line.endswith(".txt"))]
                # get the files to delete (those that have 0 bytes)
                filesToDelete = [file.split(" ")[-1] for file in files if not int(file.split(" ")[-2])]
                # get the files to download
                files = [file for file in files if int(file.split(" ")[-2])]

                # # delete empty files
                # for file in tqdm(filesToDelete): 
                #     command = f"del {file}"
                #     send_command(command, self.ser)
                #     lines = read_serial_data(self.ser)
            i+=1
        self.files = [WMOREFILE(date=file.split(" ")[0], time=file.split(" ")[1], size=file.split(" ")[-2], filename=file.split(" ")[-1]) for file in files]
        logging.info(f"logger {self.id} files: {[file.filename for file in self.files]}")
        
    def download_files(self):
        """Checks if the files on the device are the same as the files on the HUB and downloads the files that are not on the HUB or are corrupted
        """
        if self.files is None:
            self.get_file_list()
            self.disconnect()
        if len(self.files) > 1:# there's alway the OLA_settings.txt file
            # get downloaded files
            downloaded_files = get_downloaded_files(self.download_path)
            # remove empty files
            downloaded_files = clean_files(downloaded_files)
            
            device_file_dict = {file.filename:file.size for file in self.files}
            downloaded_files_dict = {file.filename:file.size for file in downloaded_files}
            redownload_files = []
            delete_files = []
            # add files that have not been downloaded or have been corrupted to the list of files to redownload
            [redownload_files.append(file) for file in device_file_dict.keys() if (file in downloaded_files_dict.keys() and device_file_dict[file] != downloaded_files_dict[file]) or file not in downloaded_files_dict.keys()]
            # add corrupted files and files that don't exist on the device to the list of files to delete
            [delete_files.append(file) for file in downloaded_files_dict.keys() if (file in device_file_dict.keys() and device_file_dict[file] != downloaded_files_dict[file])]# or file not in device_file_dict.keys()]
            
            print(f"delete files: {delete_files}")
            print(f"redownload files: {redownload_files}")
            # log the files to delete and redownload
            if len(delete_files) > 0 or len(redownload_files) > 0:
                logging.info(f"logger {self.id} delete files: {delete_files}")
                logging.info(f"logger {self.id} download files: {redownload_files}")
            # get rid of the corrupted files
            [os.remove(os.path.join(self.download_path, file)) for file in delete_files if os.path.exists(os.path.join(self.download_path, file))]
            
            if len(redownload_files) > 0:
                if sorted(redownload_files) == sorted(list(device_file_dict.keys())):
                    redownload_files = None
                run_zmodem_receive(deviceObj=self,maxRetries=3, filesToDownload=redownload_files)
                self.download_files()
            else:
                self.format_device()
                

    
    
    def to_dict(self):
        """Convert the device object to a dictionary

        Returns:
            dict: dictionary containing the device object attributes
        """
        status_dict = {CONNECTED:"connected", BUSY:"busy", CHARGING:"charging", None:None}
        type_dict = {COORDINATOR : "Coordinator",LOGGER : "Logger", None : None}
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
 
def run_zmodem_receive(deviceObj = None, maxRetries=3, filesToDownload=None):
    folderPath = deviceObj.download_path
    # create a directory for the device if it doesn't exist
    if not os.path.exists(folderPath):
        os.mkdir(folderPath)
    # Change to the directory containing the files to download
    os.chdir(folderPath)
    
    # Command to execute
    command = [os.path.join(ROOT_PATH, "zmodemFileReceive"), deviceObj.comport]
    if filesToDownload is not None:
        for file in filesToDownload:
            command.append(file)
    # log the command and the files to download
    logging.info(f"{deviceObj.id} zmodem command: {command}")
    

    try:
        # Run the command and wait for it to complete
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error occurred: {e}")
        if maxRetries > 0:
            time.sleep(10)
            run_zmodem_receive(deviceObj, maxRetries-1, filesToDownload)
    finally:
        # Change back to the original directory (optional, but recommended)
        os.chdir(ROOT_PATH)
        return folderPath

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
            
            deviceObj = DeviceObject(comport=device, type=None,status=BUSY, id=None)
            get_info_thread = threading.Thread(target=deviceObj.get_info)
            get_info_thread.start()
            print(f"\nnew device connected !: {device}\n")
            # typeToPrint = "Logger" if deviceObj.type == LOGGER else "Coordinator" 
            # print(f"\ndevice is {typeToPrint}\n")
            devices_dict[device] = deviceObj
    #         connectionChanged = True
    # if connectionChanged:
    #     send_update(devices_dict)
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

def get_downloaded_files(path):
    """ get the files in the folder specified by path
    Args:
        path (str): path to the folder containing the files to download
    Return:
        files_list (list): list of WMOREFILE objects
    """
    
    if not os.path.exists(path):
        os.mkdir(path)
        
    files = os.listdir(path)
    files_list = []
    for file in files:
        file_path = os.path.join(path, file)
        file_size = os.path.getsize(file_path)
        files_list.append(WMOREFILE(size = file_size, filename = file, path = file_path))
    return files_list

def clean_files(files):
    """ Remove empty downloaded files

    Args:
        files (list): list of WMOREFILE objects

    Returns:
        clean_files (list):  list of WMOREFILE objects
    """
    clean_files = [file for file in files if (file.filename.endswith(".bin") or file.filename.endswith(".txt")) and file.size != 0]
    [os.remove(file.path) for file in files if file not in clean_files]
    return clean_files

def main():
    # create file if it doesn't exist
    if not os.path.exists(LOG_FILE):
        with open(LOG_FILE, 'w') as f:
            pass
    #setup logging debug
    logging.basicConfig(filename=LOG_FILE, level=logging.INFO, format='%(asctime)s %(levelname)s %(name)s %(message)s')
    
    devices_dict = {}
    lastupload = 0
    # create a queue to store the devices
    wmore_queue = queue.Queue()
    full_download = False
    last_dict = {}
    while True:
        # print('\033c', end='')
        now = time.time()
        connected_devices_list = get_usb_device_list()
        #check for new connection
        devices_dict = check_for_new_device(connected_devices_list, devices_dict)
        # send_update(devices_dict)
        #check for disconnection
        devices_dict = check_for_device_disconnection(connected_devices_list, devices_dict)
        [print(deviceObj.to_dict()) for deviceObj in devices_dict.values() if last_dict != devices_dict]
        last_dict = devices_dict
        try_download = True
        for deviceObj in devices_dict.values():
            if deviceObj.status == CHARGING:
                try_download = False
        if is_internet_available():
            if now - lastupload > 60:
                print("uploading to drive")
                logging.info("uploading to drive")
                try:
                    upload()
                    lastupload = time.time()
                except:
                    print("error uploading to drive")
                    logging.debug("error uploading to drive")   
        # print("_"*50+"\n")

        time.sleep(1)

if __name__ == "__main__":
    main()