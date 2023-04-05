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
"""

import os
import subprocess
import time
from datetime import datetime
import re
import os
from multiprocessing import Pool, freeze_support

from PySide2 import QtCore, QtGui, QtWidgets, QtSerialPort

from ConversionGUI import ConvertWindow
from constants import *
import helpers 


class WMORE:
    def __init__(self, id = None, com_port = None, firmware = None):
        self.id = id
        self.com_port = com_port
        self.firmware = firmware

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
        icon = QtGui.QIcon(ICON_PATH)
        self.setWindowIcon(icon)
        self.resize(900, 600)
        
        # Create and show splash screen
        splash_pix = QtGui.QPixmap(SPLASH_PATH)
        splash_pix = splash_pix.scaled(800, 600, QtCore.Qt.KeepAspectRatio)
        splash = QtWidgets.QSplashScreen(splash_pix, QtCore.Qt.WindowStaysOnTopHint)

        splash.show()

        # Create widgets
        self.disconnect_button = HoverButton("Disconnect", hover_tip = "Close connection with device")
        self.format_button = HoverButton("Format", hover_tip="Wipe the SD Card of the selected device")
        self.rtc_button = HoverButton("Set RTC", hover_tip="Set the real time clock of the coordinator")
        self.refresh_button = HoverButton("Refresh", hover_tip="Scan computer for connected WMOREs")
        self.send_button = HoverButton("Send", hover_tip="Send serial command to WMORE (return key)")
        self.download_button = HoverButton("Download Data", hover_tip="Download data from Logger sd card")
        self.convert_button = HoverButton("Convert Data", hover_tip="Open  data conversion window")
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
        
        self.convertionApp = ConvertWindow(app)
        
        # Connect signals and slots
        self.disconnect_button.clicked.connect(self.disconnect)
        self.format_button.clicked.connect(self.format)
        self.rtc_button.clicked.connect(lambda: self.send_commands(RTC_COMMANDS))
        self.refresh_button.clicked.connect(self.refresh_devices)
        self.send_button.clicked.connect(lambda: self.send_command(self.command_line_edit.text()))
        self.download_button.clicked.connect(self.download)
        self.convert_button.clicked.connect(lambda: self.convertionApp.show())
        self.command_line_edit.returnPressed.connect(lambda: self.send_command(self.command_line_edit.text()))
        
        
        self.device_list = []
        self.selected_device = None
        splash.showMessage("Checking Tera Term...", QtCore.Qt.AlignBottom | QtCore.Qt.AlignCenter, QtGui.QColor("black"))
        self.teratermPath = helpers.check_tera_term_automate()
        if self.teratermPath == None:
            splash.finish(self)
            self.teratermPath = helpers.check_tera_term(self)
            splash.show()

        self.add_header()
        splash.showMessage("Getting devices information...", QtCore.Qt.AlignBottom | QtCore.Qt.AlignCenter, QtGui.QColor("black"))
        # Close splash screen
        self.refresh_devices()
        splash.finish(self)
        self.raise_()
        
    def set_button_state(self,state):
        """Enable or disable all buttons

        Args:
            state (bool): True for enabling buttons, False for disabling them
        """
        # assume "window" is a reference to the main window widget
        for button in self.findChildren(QtWidgets.QPushButton):
            button.setEnabled(state)

    def add_header(self):
        """
        Adds a header to a Device QListWidget
        """
        header_text = "Com Port\tID Firmware"
        header_item = QtWidgets.QListWidgetItem(header_text) 
        header_item.setFlags(header_item.flags() & ~QtCore.Qt.ItemIsSelectable & ~QtCore.Qt.ItemIsEditable) # Disable selection and editing of the header item
        header_item.setBackground(QtGui.QColor(142, 190, 203)) # Set the background color to red
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
        """_summary_
        """
        if self.serial.isOpen():
            portn = self.list_widget.selectedItems()[0].text().split("COM")[1]
            self.disconnect_serial()
            self.list_widget.clearSelection()
            QtCore.QCoreApplication.processEvents()
            macro_full_path = os.path.abspath(TERATERM_MACRO)
            # Open a file dialog to choose output folder
            newFolderPath = QtWidgets.QFileDialog.getExistingDirectory(self, 'Choose output folder')
            item = self.list_widget.currentItem()
            # create a folder with the date time of download and firmware and id of sensor data
           
            fw = self.selected_device.firmware.split(" ")[0]
            id = self.selected_device.id
            now = datetime.now()
            newFolderPath = f"{newFolderPath}\\\\{now.strftime('%y%m%d_%H%M%S')}_{fw}_{id}"
            if not os.path.exists(newFolderPath):
                # If the folder doesn't exist, create it (including any necessary parent directories)
                os.makedirs(newFolderPath)
            helpers.change_output_path_in_ttl(macro_full_path, newFolderPath)
            if self.teratermPath != None:
                try:
                    
                    # subprocess.run(f'{self.teratermPath} /C={portn} /BAUD=115200 /M="{macro_full_path}"')
                    command = f'"{self.teratermPath}" /C={portn} /BAUD=115200 /M="{macro_full_path}"'
                    subprocess.Popen(command, shell=True)
                except Exception as e:
                    QtWidgets.QMessageBox.critical(self,"Tera Term Error", f"An error occured when trying to run Tera Term:\n{str(e)}")
                                
        else:
            QtWidgets.QMessageBox.critical(self,"No Device", "No selected device found.\nPlease select a device first.")

    def send_commands(self,commands):
        """Send a list of commands to the serial device

        Args:
            commands (str): a list of character and/or strings
        """
        if self.serial.isOpen():
            for cmd in commands:
                QtCore.QCoreApplication.processEvents()  # allow the event loop to run
                time.sleep(0.5)
                cmd = helpers.is_date_component_request(cmd)
                self.send_command(cmd)
        else:
            QtWidgets.QMessageBox.critical(self,"No Device", "No selected device found.\nPlease select a device first.")

    def refresh_devices(self):
        """iterate over com port and list all the WMOREs conencted
        """
        self.list_widget.clear()
        self.add_header()
        self.device_list.clear()
        self.selected_device = None
        num_devices = 0
        for port_info in QtSerialPort.QSerialPortInfo.availablePorts():
            port = port_info.portName()
            status = self.connect_serial(port)
            if status == 0: # the device is an wmore and the connection was successful
                wmore = WMORE(com_port=port) #create WMORE object
                self.device_list.append(wmore)
                self.selected_device = wmore
                self.list_widget.addItem(f"{self.selected_device.com_port}")
                item = self.list_widget.item(num_devices + 2)# +2 to remove the header and sepeartion line
                self.list_widget.setCurrentItem(item)
                self.set_button_state(False)
                # get device id
                self.send_commands(GET_ID_COMMANDS)
                self.update_item_text()
                self.disconnect_serial()
                num_devices+=1
        self.list_widget.clearSelection()
        self.set_button_state(True)
        self.rtc_button.setEnabled(False)
        self.download_button.setEnabled(False)
        self.format_button.setEnabled(False)

    def switch_device(self):
        """Function triggered when the user selects a new device.
            Connects to selected device
        """
        selected_items = self.list_widget.selectedItems()
        if selected_items:
            port_name = selected_items[0].text().split("\t")[0]
            self.disconnect_serial()
            status = self.connect_serial(port_name)
            if status == 2:
                # In case the user tries to connect to a device that has been unplugged
                QtWidgets.QMessageBox.critical(self, "Error", f"Failed to connect to {port_name}\nCheck device connection, and refresh." )
                index = self.list_widget.currentRow()
                self.list_widget.takeItem(index)
            elif status == 1:
                QtWidgets.QMessageBox.critical(self, "Error", f"Failed to connect to {port_name}" )
            else:
                if self.serial.isOpen():
                    
                    self.serial_output.append(f"Connected to {port_name}\n\n")
                    # disable rtc button for logger and download button for coordinator 
                    devices = [device for device in self.device_list if device.com_port == port_name]
                    self.selected_device = devices[0]
                    
                    self.rtc_button.setEnabled(not any((device.firmware is not None) and ("Logger" in device.firmware)  for device in devices))
                    if self.teratermPath != None:
                        self.download_button.setEnabled(any((device.firmware is not None) and ("Logger" in device.firmware) for device in devices))
                    self.format_button.setEnabled(any((device.firmware is not None) and ("Logger" in device.firmware) for device in devices))
                   
                else:
                    QtWidgets.QMessageBox.warning(self, "Communication failure", f"Failed to connect to device\nIt may be opened in another program" )
                    self.list_widget.clearSelection()
                    
        else:
            self.disconnect_serial()
            self.set_button_state(True)
            self.rtc_button.setEnabled(False)
            self.download_button.setEnabled(False)
            self.format_button.setEnabled(False)
            
    def connect_serial(self,port_name):
        """Connect to device with specified comport

        Args:
            port_name (str): the com port to connect to (eg: COM25)
        """
        info = QtSerialPort.QSerialPortInfo(port_name)
        product_id = info.productIdentifier()
        # check that the device we are connecting to is an Openlog
        if product_id == OPENLOG_PRODUCT_ID:  # Product ID of Openlog
            self.serial.setPortName(port_name)
            try:
                self.serial.open(QtCore.QIODevice.ReadWrite)
                return(0)
            except:
                return(1)
        else:
            # In case the user tries to connect to a device that has been unplugged
            return(2)
        
    def send_command(self,command):
        """Format input string and send it to serial device

        Args:
            command (str): the character or string to send to the device
        """
        command+="\n\r"
        self.serial.write(command.encode())
        self.command_line_edit.clear()
            
    def read_serial(self):
        """Read data from the device, process the data and display in on the screen
        """
        try:
            data = self.serial.readAll().data().decode()
        except:
            data = "could not decode!"
        self.process_serial_output(data)
        self.serial_output.insertPlainText(data)
        self.serial_output.ensureCursorVisible() #automatically scroll down

    def disconnect_serial(self):
        """close serial port
        """
        if self.serial.isOpen():
            self.serial.close()
            self.serial_output.append("Disconnected\n")
    
    def disconnect(self):
        """Close serial connection
        """
        self.list_widget.clearSelection()
        self.disconnect_serial()
                    
    def process_serial_output(self, data):
        """Exectute functions based on the device output

        Args:
            data (str): data read from the serial device
        """
        if MESSAGE_PATTERNS["M_FORMAT"] in data:
            self.format_done()
        elif MESSAGE_PATTERNS["M_FIRMWARE"] in data:
            fw = re.search(r"\bWMORE\s+(.+)", data).group(1) # extract the id
            self.selected_device.firmware = fw
            self.update_item_text()
        elif MESSAGE_PATTERNS["M_ID"] in data:
            sensorID = int(re.search(r'Sensor ID: (\d+)', data).group(1)) # extract the id
            self.selected_device.id = sensorID
            self.update_item_text()
        
    def format(self):
        reply = QtWidgets.QMessageBox.warning(self, 'Warning', 'This will delete all data on the sensor.\nAre you sure you want to continue?', 
                                              QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No, QtWidgets.QMessageBox.No)

        if reply == QtWidgets.QMessageBox.Yes:
            self.send_commands(FORMAT_COMMANDS)  
              
    def format_done(self):
        """Remove device from list a tell the user to power cycle the device
        """
        QtWidgets.QMessageBox.information(self, "Formatting Done", "The SD card has been formatted, please power cycle the device.")
        index = self.list_widget.currentRow()
        self.list_widget.takeItem(index)
        
    def update_item_text(self):
        """Update the data shown on the currently selected device
        """
        item = self.list_widget.currentItem()
        item.setText(f"{self.selected_device.com_port}\t {self.selected_device.id} {self.selected_device.firmware}")
    
    def closeEvent(self, event):
        self.disconnect_serial()
        self.convertionApp.close() # in case the convertion window is still open
        # Call the superclass method to continue with the default closing behavior
        super().closeEvent(event)

if __name__ == "__main__":
    freeze_support() # necessary for when compiling the exe
    app = QtWidgets.QApplication([])
    window = MainWindow()
    window.show()
    app.exec_()
