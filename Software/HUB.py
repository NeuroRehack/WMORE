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

from ConversionGUI import ConvertWindow

# Import necessary libraries
import serial.tools.list_ports  # Library to access information about available serial ports
from PyQt5 import QtCore, QtGui, QtWidgets, QtSerialPort  # Library for building GUI applications
from PyQt5.QtCore import QCoreApplication, Qt, QSize, QTimer
from PyQt5.QtGui import QTextCursor, QColor, QBrush, QFont, QIcon, QCursor
from PyQt5.QtWidgets import QListWidgetItem, QFileDialog, QApplication, QPushButton, QToolTip


import time  # Library for working with time
from datetime import datetime, date  # Library for working with dates and times
import subprocess  # Library for spawning new processes
import os  # Library for interacting with the operating system

# Define lists of commands that will be used later
rtc_commands = [
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

format_commands = [
    'm', 
    's', 
    'fmt'  
]

# Define a function to get the current date and time
def getDate(cmd):
    if cmd == "year":
        dt = datetime.now().year-2000  # Get the current year and subtract 2000 to get the last 2 digits
        return '%d' % dt  # Return the year as a string
    elif cmd == "month":
        dt = datetime.now().month  # Get the current month
        return '%d' % dt  # Return the month as a string
    elif cmd == "day":
        dt = datetime.now().day  # Get the current day
        return '%d' % dt  # Return the day as a string
    elif cmd == "hour":
        dt = datetime.now().hour  # Get the current hour
        return '%d' % dt  # Return the hour as a string
    elif cmd == "minute":
        dt = datetime.now().minute  # Get the current minute
        return '%d' % dt  # Return the minute as a string
    elif cmd == "second":
        dt = datetime.now().second  # Get the current second
        return '%d' % dt  # Return the second as a string
    else:
        return cmd  # Return the cmd if it is not a recognized date or time component

def update_changedir_path(filepath, newpath):
    # Open the file for reading and writing
    with open(filepath, 'r+') as file:
        # Read the contents of the file into memory
        contents = file.read()
        # Split the contents of the file into lines
        lines = contents.split('\n')
        # Look for the line that starts with "changedir"
        for i, line in enumerate(lines):
            if line.startswith('changedir'):
                # Replace the path in the "changedir" line with the new path
                newpath = newpath.replace("/", "\\\\")
                lines[i] = f'changedir "{newpath}"'
                break
        else:
            # If no "changedir" line was found, raise an error
            raise ValueError('No "changedir" line found in script file')
        # Join the lines back together into a single string
        updated_contents = '\n'.join(lines)
        # Move the file pointer back to the start of the file
        file.seek(0)
        # Write the updated contents back to the file
        file.write(updated_contents)
        # Truncate the file to the new length (in case the new contents are shorter than the old)
        file.truncate()


class HoverButton(QPushButton):
    def __init__(self, parent=None,hover_tip=''):
        super().__init__(parent)
        self.hover_tip = hover_tip
        self.setToolTipDuration(2000) # set tooltip duration to 2 seconds
        self.hover_timer = QTimer(self)
        self.hover_timer.timeout.connect(self.on_hover)

    def enterEvent(self, event):
        self.hover_timer.start(500) # start the timer when mouse enters
        super().enterEvent(event)

    def leaveEvent(self, event):
        self.hover_timer.stop() # stop the timer when mouse leaves
        QToolTip.hideText()
        super().leaveEvent(event)

    def on_hover(self):
        self.hover_timer.stop()
        QToolTip.showText(QCursor.pos(), self.hover_tip) # show the hint text at the current cursor position


class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("WMORE")
        icon = QIcon("Software\\images\\WMORE.png")
        self.setWindowIcon(icon)
        self.resize(900, 600)

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
        self.list_widget.itemSelectionChanged.connect(self.update_selected_text)

        # Initialize serial port
        self.serial = QtSerialPort.QSerialPort()
        self.serial.setBaudRate(QtSerialPort.QSerialPort.Baud115200)
        self.serial.readyRead.connect(self.read_serial)
        
        # Connect signals and slots
        self.disconnect_button.clicked.connect(self.disconnect_serial)
        self.format_button.clicked.connect(lambda: self.sendCmds(format_commands))
        self.rtc_button.clicked.connect(lambda: self.sendCmds(rtc_commands))
        self.refresh_button.clicked.connect(self.refreshDevices)
        self.send_button.clicked.connect(lambda: self.send_command(self.command_line_edit.text()))
        self.download_button.clicked.connect(self.download)
        self.convert_button.clicked.connect(self.callConvert)
        self.command_line_edit.returnPressed.connect(lambda: self.send_command(self.command_line_edit.text()))
        
        self.port = ""
        self.addHeader()

    def callConvert(self):
        window = ConvertWindow(app)
        window.show()
        
    def addHeader(self):
        header_labels = ["ID", "Firmware", "COM Port"]
        header_text = "\t".join(header_labels)
        header_item = QListWidgetItem(header_text) # Create a new item with the header text
        header_item.setFlags(header_item.flags() & ~Qt.ItemIsSelectable & ~Qt.ItemIsEditable) # Disable selection and editing of the header item
        font = QFont()
        font.setBold(True)
        # Set the font for the item
        header_item.setFont(font)

        self.list_widget.insertItem(0, header_item) # Insert the header item at the top of the list
        # Create the separator item
        separator_item = QListWidgetItem("")
        separator_item.setBackground(QBrush(QColor(200, 200, 200)))  # Set the background color to light gray
        separator_item.setSizeHint(QSize(2, 2))  # Set the size hint to 1x1 pixel
        separator_item.setFlags(separator_item.flags() & ~Qt.ItemIsSelectable & ~Qt.ItemIsEditable) # Disable selection and editing of the header item
        self.list_widget.addItem(separator_item)
        
    def download(self):
        if self.serial.isOpen():
            portn = self.list_widget.selectedItems()[0].text().split("COM")[1]
            print(portn)
            self.disconnect_serial()
            self.list_widget.clearSelection()
            QCoreApplication.processEvents()
            macro_full_path = os.path.abspath("Software\\teratermMacro.ttl")
            # Open a file dialog to choose a folder
            newFolderPath = QFileDialog.getExistingDirectory(self, 'Choose a folder')
            item = self.list_widget.currentItem()
            # create a folder with the date time of download and firmware and id of sensor data
            txt = item.text().split("\t")
            now = datetime.now()
            newFolderPath = f"{newFolderPath}\\\\{now.strftime('%y_%m_%d-%H_%M_%S')}_{txt[1]}_{txt[0]}"
            if not os.path.exists(newFolderPath):
                # If the folder doesn't exist, create it (including any necessary parent directories)
                os.makedirs(newFolderPath)
            update_changedir_path(macro_full_path, newFolderPath)
            subprocess.run(f'"C:\\Program Files (x86)\\teraterm\\ttermpro.exe" /C={portn} /BAUD=115200 /M="{macro_full_path}"')
        else:
            QtWidgets.QMessageBox.critical(self,"No Device", "No selected device found.\nPlease select a device first.")

    def sendCmds(self,commands):
        disconnectAfter = False
        if self.serial.isOpen():
            for cmd in commands:
                QCoreApplication.processEvents()  # allow the event loop to run
                time.sleep(0.5)
                cmd = getDate(cmd)
                self.send_command(cmd)
        else:
            QtWidgets.QMessageBox.critical(self,"No Device", "No selected device found.\nPlease select a device first.")
            


    
    def refreshDevices(self):
        self.list_widget.clear()
        self.addHeader()
        # Populate com_port_combo_box with available COM ports
        ports = serial.tools.list_ports.comports()
        num_devices = 2
        for port in ports:
            if self.connect_serial(port.device):
                self.list_widget.addItem(f"\t\t{port.device}")
                self.port = port.device
                item = self.list_widget.item(num_devices) # Get a reference to the third item in the list (index starts at 0)
                self.list_widget.setCurrentItem(item) # Select the third item in the list
                self.sendCmds(["m","1","x","x"])
                self.disconnect_serial()
                num_devices+=1
        self.list_widget.clearSelection()

    def update_selected_text(self):
        # Get the selected text from the list box and display it in the window's title bar
        selected_items = self.list_widget.selectedItems()
        if selected_items:
            selected_text = selected_items[0].text().split("\t")[-1]
            print(f"Selected port: {selected_text}")
        self.disconnect_serial()
        try:
            self.connect_serial(selected_text)
        except:
            print("Cant connect to device")
            
    def connect_serial(self,port_name):
        info = QtSerialPort.QSerialPortInfo(port_name)
        product_id = info.productIdentifier()
        if product_id == 29987:  # replace with expected product ID
            self.serial.setPortName(port_name)
            self.port = port_name
            try:
                
                self.serial.open(QtCore.QIODevice.ReadWrite)
              
                self.serial_output.append("Connected to " + port_name+"\n")
                return(1)
            except:
                QtWidgets.QMessageBox.warning(self, "Error", "Failed to connect to " + port_name)
                return(0)
        else:
            print("Error: Connected to unexpected device")
            return(0)

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
        self.process_output(data)
        self.serial_output.insertPlainText(data)
        self.serial_output.ensureCursorVisible() 
    
    def process_output(self,data):

        if "done" in data:
            QtWidgets.QMessageBox.information(self,"Formatting Done", "The SD card has been formatted, please power cycle the device.")  
            index = self.list_widget.currentRow()
            self.list_widget.takeItem(index)  
        if "Coordinator v" in data:
            item = self.list_widget.currentItem()
            txt = item.text().split("\t")
            txt[1] = "Coordinator"
            txt = "\t".join(txt)
            item.setText(txt)
        if "Sensor v" in data:
            item = self.list_widget.currentItem()
            txt = item.text().split("\t")
            txt[1] = "Logger"
            txt = "\t".join(txt)
            item.setText(txt)
        if "ID" in data:
            sensorID = int((data.split("ID:")[1]).split("\n")[0])
            item = self.list_widget.currentItem()
            txt = item.text().split("\t")
            txt[0] = str(sensorID)
            txt = "\t".join(txt)
            item.setText(txt)

    
    def closeEvent(self, event):
        self.disconnect_serial()
        print("bye")
        # Call the superclass method to continue with the default closing behavior
        super().closeEvent(event)

if __name__ == "__main__":
    app = QtWidgets.QApplication([])
    window = MainWindow()
    window.show()
    app.exec_()
