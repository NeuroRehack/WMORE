"""
File: WMORE_HUB.py
Author: Sami Kaab
Date: March 28, 2023
Description:   This program allows the user to interact with the WMOREs.
Feature List:
                List available devices
                Serial communication
                Set RTC
                Format SD card
                Download data
                Convert Data
To Do:  
                implement all feature for one button multiple devices simultaneously
                allow user to enable 
                disable set rtc button for loggers
                store sensor info in sensor object class
"""


import os
import subprocess
import time
from datetime import datetime
import re
import os
import configparser
from multiprocessing import Pool


from PySide2 import QtCore, QtGui, QtWidgets, QtSerialPort

from ConversionGUI import ConvertWindow


# Define lists of commands that will be used later
RTC_COMMANDS = [
    'm',  
    '2',  
    '4',  
    "year",  
    "month",  
    "day",  
    '6',  
    "hour",  
    "minute",  
    "second",  
    'x'  
]

FORMAT_COMMANDS = [
    'm', 
    's', 
    'fmt'  
]

TERATERM = "ttermpro.exe"

ROOT = "\\"

def is_date_component_request(cmd):
    """
    Get the current date and time component specified in the command.

    Args:
        cmd (str): A string indicating which date or time component to return.
            Valid options are 'year', 'month', 'day', 'hour', 'minute', 'second'.

    Returns:
        str: A string representation of the current value of the specified date or time component.
        If the input command is not recognized, the function returns the input command as a string.
    """
    date_time_components = {
        'year': '%y',
        'month': '%m',
        'day': '%d',
        'hour': '%H',
        'minute': '%M',
        'second': '%S'
    }

    if cmd in date_time_components:
        dt = datetime.now()
        return dt.strftime(date_time_components[cmd])  # format the datetime object to the specified component
    else:
        return cmd  # Return the cmd if it is not a recognized date or time component



def change_output_path_in_ttl(filepath, newpath):
    """
    Update the "changedir" line in the ttl script file with a new path.

    Args:
        filepath (str): The path to the ttl script file.
        newpath (str): The new path to use in the "changedir" line.

    Raises:
        ValueError: If no "changedir" line is found in the script file.

    """
    # Replace forward slashes with backslashes if needed
    newpath = newpath.replace('/', os.path.sep)

    # Read the contents of the file into memory
    with open(filepath, 'r') as file:
        contents = file.read()

    # Look for the line that starts with "changedir"
    lines = contents.split('\n')
    for i, line in enumerate(lines):
        if line.startswith('changedir'):
            # Replace the path in the "changedir" line with the new path
            lines[i] = f'changedir "{newpath}"'
            break
    else:
        # If no "changedir" line was found, raise an error
        raise ValueError('No "changedir" line found in script file')

    # Write the updated contents back to the file
    with open(filepath, 'w') as file:
        file.write('\n'.join(lines))

def get_config(iniFilePath,section,var):
    config = configparser.ConfigParser()
    config.read(iniFilePath)
    value = config.get(section,var)
    return value
    

def set_config(iniFilePath,section,var,val):
    config = configparser.ConfigParser()
    config.read(iniFilePath)
    config.set(section,var,val)
    with open(iniFilePath, 'w') as config_file:
        config.write(config_file)
        
def search_directory(root):
    for dirpath, dirnames, filenames in os.walk(root):
        if TERATERM in filenames:
            return os.path.join(dirpath, TERATERM)
        
        
    
class HoverButton(QtWidgets.QPushButton):
    """
    Define a new QPushButton subclass with hover behavior
    """
    def __init__(self, parent=None,hover_tip=''):
        super().__init__(parent)
        self.hover_tip = hover_tip
        self.setToolTipDuration(2000) # set tooltip duration to 2 seconds
        self.hover_timer = QtCore.QTimer(self)
        self.hover_timer.timeout.connect(self.on_hover)

    def enterEvent(self, event):
        self.hover_timer.start(500) # start the timer when mouse enters
        super().enterEvent(event)

    def leaveEvent(self, event):
        self.hover_timer.stop() # stop the timer when mouse leaves
        QtWidgets.QToolTip.hideText()
        super().leaveEvent(event)

    def on_hover(self):
        self.hover_timer.stop()
        QtWidgets.QToolTip.showText(QtGui.QCursor.pos(), self.hover_tip) # show the hint text at the current cursor position


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("WMORE")
        icon = QtGui.QIcon("Software\\images\\WMORE.png")
        self.setWindowIcon(icon)
        self.resize(900, 600)
        
        # Create and show splash screen
        splash_pix = QtGui.QPixmap('Software\images\WMORE.png')
        splash_pix = splash_pix.scaled(800, 600, QtCore.Qt.KeepAspectRatio)
        splash = QtWidgets.QSplashScreen(splash_pix, QtCore.Qt.WindowStaysOnTopHint)
        splash.show()

        # Create widgets
        self.disconnect_button = HoverButton("Disconnect",hover_tip = "Closes serial conenction")
        self.format_button = HoverButton("Format",hover_tip="Wipes the SD Card of the selected device")
        self.rtc_button = QtWidgets.QPushButton("Set RTC")
        self.refresh_button = QtWidgets.QPushButton("Refresh")
        self.send_button = QtWidgets.QPushButton("Send")
        self.download_button = QtWidgets.QPushButton("Download Data")
        self.convert_button = QtWidgets.QPushButton("Convert Data")
        # create consol interaction widget
        self.command_line_edit = QtWidgets.QLineEdit()
        self.serial_output = QtWidgets.QTextEdit()
        self.serial_output.setStyleSheet("background-color: #D6EBEB;")
        # Create a list box widget
        self.list_widget = QtWidgets.QListWidget(self)
        
        # Create layouts
        central_widget = QtWidgets.QWidget()
        main_layout = QtWidgets.QVBoxLayout(central_widget)
        device_info_layout = QtWidgets.QVBoxLayout()
        consol_input_layout = QtWidgets.QHBoxLayout()
        consol_interaction_layout = QtWidgets.QVBoxLayout()
        com_consol_input_layout = QtWidgets.QHBoxLayout()
        button_layout = QtWidgets.QHBoxLayout()
        
        #add buttons to button layout
        button_layout.addWidget(self.refresh_button)
        button_layout.addWidget(self.disconnect_button)
        button_layout.addWidget(self.rtc_button)
        button_layout.addWidget(self.download_button)
        button_layout.addWidget(self.convert_button)
        button_layout.addWidget(self.format_button)
        
        # Add command input and send button to consol input
        consol_input_layout.addWidget(self.command_line_edit)
        consol_input_layout.addWidget(self.send_button)
        
        # Add serial output and consol input layout to consol interation layout
        device_info_layout.addWidget(self.list_widget)
        consol_interaction_layout.addWidget(self.serial_output)
        consol_interaction_layout.addLayout(consol_input_layout)
        
        com_consol_input_layout.addLayout(device_info_layout)
        com_consol_input_layout.addLayout(consol_interaction_layout)
        
        main_layout.addLayout(button_layout)
        main_layout.addLayout(com_consol_input_layout)
        self.setCentralWidget(central_widget)
        
        # Connect the list box's selectionChanged signal to a method that updates the selected text
        self.list_widget.itemSelectionChanged.connect(self.switch_device)

        # Initialize serial port
        self.serial = QtSerialPort.QSerialPort()
        self.serial.setBaudRate(QtSerialPort.QSerialPort.Baud115200)
        self.serial.readyRead.connect(self.read_serial)
        
        # Connect signals and slots
        self.disconnect_button.clicked.connect(self.disconnect_btn_pressed)
        self.format_button.clicked.connect(lambda: self.send_commands(FORMAT_COMMANDS))
        self.rtc_button.clicked.connect(lambda: self.send_commands(RTC_COMMANDS))
        self.refresh_button.clicked.connect(self.refresh_devices)
        self.send_button.clicked.connect(lambda: self.send_command(self.command_line_edit.text()))
        self.download_button.clicked.connect(self.download)
        self.convert_button.clicked.connect(self.call_convert)
        self.command_line_edit.returnPressed.connect(lambda: self.send_command(self.command_line_edit.text()))
        
        splash.showMessage("checking Tera Term...", QtCore.Qt.AlignBottom | QtCore.Qt.AlignCenter, QtGui.QColor("black"))
        self.teratermPath = self.check_tera_term()
        self.port = ""
        self.add_header()
        splash.showMessage("Getting devices information", QtCore.Qt.AlignBottom | QtCore.Qt.AlignCenter, QtGui.QColor("black"))
        self.refresh_devices()
        # Close splash screen
        splash.finish(self)

    def disconnect_btn_pressed(self):
        self.list_widget.clearSelection()
        self.disconnect_serial()
        
    def check_tera_term(self):
        teratermPath = get_config('Software\\config.ini','paths', 'teraterm_location')
        search_path = ROOT
        if os.path.split(teratermPath)[-1] != 'ttermpro.exe':
            with Pool() as pool:
                results = pool.map(search_directory, [os.path.join(search_path, d) for d in os.listdir(search_path)])
            for r in results:
                if r:
                    teratermPath = r
                    set_config('Software\\config.ini','paths', 'teraterm_location', teratermPath)
                    return teratermPath
        if os.path.split(teratermPath)[-1] != 'ttermpro.exe':
            answer = QtWidgets.QMessageBox.question(self, 
                                                    "Tera Term",
                                                    f"Tera Term seem to be missing.\nYou will not be able to download data from the WMORE until you set the path.\nDo you wan to set the path to Tera Term now?",
                                                    QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                                                    QtWidgets.QMessageBox.Yes)
            if answer == QtWidgets.QMessageBox.Yes:
                
                teratermPath = self.find_tera_term()
                
            else:
                QtWidgets.QMessageBox.warning(self,"Download Feature","You will not be able to download data from the WMORE until you set the path.")        
        if os.path.split(teratermPath)[-1] != 'ttermpro.exe':
            self.download_button.setEnabled(False)
        return teratermPath
    
    def find_tera_term(self):
        teratermPath=""
        # Prompt user to select tera term exe file location
        teratermPath, _ = QtWidgets.QFileDialog.getOpenFileName(caption='Select teraterm location',
                                                                        filter='Executable Files (*.exe)',
                                                                        options=QtWidgets.QFileDialog.Options())
        if os.path.split(teratermPath)[-1]=="ttermpro.exe":
            answer = QtWidgets.QMessageBox.question(self, 
                                                    "Confirmation",
                                                    f"Set teraterm exe file path to:\n{teratermPath}",
                                                    QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No | QtWidgets.QMessageBox.Cancel,
                                                    QtWidgets.QMessageBox.Yes)
            if answer == QtWidgets.QMessageBox.Yes:
                set_config('Software\\config.ini','paths', 'teraterm_location', teratermPath)
            elif answer == QtWidgets.QMessageBox.No:
                self.find_tera_term()
            else:
                QtWidgets.QMessageBox.warning(self,"Download Feature","You will not be able to download data from the WMORE until you set the path.")        
        else:
            answer = QtWidgets.QMessageBox.critical(self,"Wrong Path","The file you have chosen seems wrong.\nThe executable for Tera Term should be called ttermpro.exe\nTry again?",QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No)  
            if answer == QtWidgets.QMessageBox.Yes:
                self.find_tera_term()
            else:
                QtWidgets.QMessageBox.warning(self,"Download Feature","You will not be able to download data from the WMORE until you set the path.")        
                return teratermPath
                
        return teratermPath
        
    
    def call_convert(self):
        """
        Opens the ConvertWindow and displays it to the user.
        """
        window = ConvertWindow(app)
        window.show()

        
    def add_header(self):
        """
        Adds a header to a Device QListWidget
        """
        header_labels = ["ID", "Firmware", "COM Port"]
        header_text = "\t".join(header_labels)
        header_item = QtWidgets.QListWidgetItem(header_text) 
        header_item.setFlags(header_item.flags() & ~QtCore.Qt.ItemIsSelectable & ~QtCore.Qt.ItemIsEditable) # Disable selection and editing of the header item
        font = QtGui.QFont()
        font.setBold(True)
        header_item.setFont(font)
        self.list_widget.insertItem(0, header_item) # Insert the header item at the top of the list
        # Create the separator item
        separator_item = QtWidgets.QListWidgetItem("")
        separator_item.setBackground(QtGui.QBrush(QtGui.QColor(200, 200, 200)))  # Set the background color to light gray
        separator_item.setSizeHint(QtCore.QSize(2, 2))  # Set the size hint to 1x1 pixel
        separator_item.setFlags(separator_item.flags() & ~QtCore.Qt.ItemIsSelectable & ~QtCore.Qt.ItemIsEditable) # Disable selection and editing of the header item
        self.list_widget.addItem(separator_item)
        
    def download(self):
        if self.serial.isOpen():
            portn = self.list_widget.selectedItems()[0].text().split("COM")[1]
            print(portn)
            self.disconnect_serial()
            self.list_widget.clearSelection()
            QtCore.QCoreApplication.processEvents()
            macro_full_path = os.path.abspath("Software\\teratermMacro.ttl")
            # Open a file dialog to choose a folder
            newFolderPath = QtWidgets.QFileDialog.getExistingDirectory(self, 'Choose output folder')
            item = self.list_widget.currentItem()
            # create a folder with the date time of download and firmware and id of sensor data
            txt = item.text().split("\t")
            now = datetime.now()
            newFolderPath = f"{newFolderPath}\\\\{now.strftime('%y_%m_%d-%H_%M_%S')}_{txt[1]}_{txt[0]}"
            if not os.path.exists(newFolderPath):
                # If the folder doesn't exist, create it (including any necessary parent directories)
                os.makedirs(newFolderPath)
            change_output_path_in_ttl(macro_full_path, newFolderPath)
            if self.teratermPath != "":
                print(self.teratermPath)
                try:
                    subprocess.run(f'{self.teratermPath} /C={portn} /BAUD=115200 /M="{macro_full_path}"')
                except Exception as e:
                    QtWidgets.QMessageBox.critical(self,"Tera Term Error", f"An error occured when trying to run Tera Term:\n{str(e)}")
                                
        else:
            QtWidgets.QMessageBox.critical(self,"No Device", "No selected device found.\nPlease select a device first.")

    def send_commands(self,commands):
        if self.serial.isOpen():
            for cmd in commands:
                QtCore.QCoreApplication.processEvents()  # allow the event loop to run
                time.sleep(0.5)
                cmd = is_date_component_request(cmd)
                self.send_command(cmd)
        else:
            QtWidgets.QMessageBox.critical(self,"No Device", "No selected device found.\nPlease select a device first.")

    def refresh_devices(self):
        self.list_widget.clear()
        self.add_header()
        num_devices = 2
        for port_info in QtSerialPort.QSerialPortInfo.availablePorts():
            port = port_info.portName()
            status = self.connect_serial(port)
            if status == 0:
                self.list_widget.addItem(f"\t\t{port}")
                self.port = port
                item = self.list_widget.item(num_devices) # Get a reference to the third item in the list (index starts at 0)
                self.list_widget.setCurrentItem(item) # Select the third item in the list
                self.send_commands(["m","1","x","x"])
                self.disconnect_serial()
                num_devices+=1
        self.list_widget.clearSelection()


    def switch_device(self):
        selected_items = self.list_widget.selectedItems()
        if selected_items:
            port_name = selected_items[0].text().split("\t")[-1]
            self.disconnect_serial()
            status = self.connect_serial(port_name)
            if status == 2:
                QtWidgets.QMessageBox.critical(self, "Error", f"Failed to connect to {port_name}\nCheck device connection, and refresh." )
                index = self.list_widget.currentRow()
                self.list_widget.takeItem(index)
                
            
            
    def connect_serial(self,port_name):
        info = QtSerialPort.QSerialPortInfo(port_name)
        product_id = info.productIdentifier()
        print(f"{port_name}:{product_id}")
        if product_id == 29987:  # replace with expected product ID
            self.serial.setPortName(port_name)
            self.port = port_name
            try:
                self.serial.open(QtCore.QIODevice.ReadWrite)
                self.serial_output.append("Connected to " + port_name+"\n")
                return(0)
            except:
                QtWidgets.QMessageBox.critical(self, "Error", "Failed to connect to " + port_name)
                return(1)
        else:
            print("Error: Connected to unexpected device")
            return(2)

    def disconnect_serial(self):
        if self.serial.isOpen():
            self.serial.close()
            self.serial_output.append("Disconnected\n")
        
    def send_command(self,command):
        # self.serial_output.insertPlainText(f"\nsent: {command}\n")
        command+="\n\r"
        self.serial.write(command.encode())
        self.command_line_edit.clear()

    def read_serial(self):
        try:
            data = self.serial.readAll().data().decode()
        except:
            data = "could not decode!"
        self.process_serial_output(data)
        self.serial_output.insertPlainText(data)
        self.serial_output.ensureCursorVisible() 
            
    def process_serial_output(self, data):
        if "done" in data:
            self.format_done()
        elif "Coordinator v" in data:
            self.update_item_text(1, "Coordinator")
        elif "Sensor v" in data:
            self.update_item_text(1, "Logger")
        elif "ID" in data:
            sensorID = int(re.search(r'ID: (\d+)', data).group(1))
            self.update_item_text(0, str(sensorID))
            
    def format_done(self):
        QtWidgets.QMessageBox.information(self, "Formatting Done", "The SD card has been formatted, please power cycle the device.")
        index = self.list_widget.currentRow()
        self.list_widget.takeItem(index)
        
    def update_item_text(self, idx, new_text):
        item = self.list_widget.currentItem()
        txt = item.text().split("\t")
        txt[idx] = new_text
        txt = "\t".join(txt)
        item.setText(txt)
    
    def closeEvent(self, event):
        self.disconnect_serial()
        # Call the superclass method to continue with the default closing behavior
        super().closeEvent(event)

if __name__ == "__main__":
    app = QtWidgets.QApplication([])
    window = MainWindow()
    window.show()
    app.exec_()
