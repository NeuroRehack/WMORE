import serial.tools.list_ports
from PyQt5 import QtCore, QtGui, QtWidgets, QtSerialPort
from PyQt5.QtCore import QCoreApplication
from PyQt5.QtGui import QTextCursor


import time
from datetime import datetime, date
import subprocess
import os



rtc_commands = [
    b'm\n\r',
    b'2\n\r',
    b'4\n\r',
    "year",
    "month",
    "day",
    b'6\n\r',
    "hour",
    "minute",
    "second",
    b'x\n\r'
    ]
# ,
#     b'q\n\r',
#     b'y\n\r'
#     ]
format_commands = [
    b'm\n\r',
    b's\n\r',
    b'fmt\n\r',
    b'x\n\r',
    b'x\n\r'
]

def getDate(mode):
    if mode == "year":
        dt = datetime.now().year-2000
        return b'%d\n\r'%dt
    elif mode == "month":
        dt = datetime.now().month
        return b'%d\n\r'%dt
    elif mode == "day":
        dt = datetime.now().day
        return b'%d\n\r'%dt
    elif mode == "hour":
        dt = datetime.now().hour
        return b'%d\n\r'%dt
    elif mode == "minute":
        dt = datetime.now().minute
        return b'%d\n\r'%dt
    elif mode == "second":
        dt = datetime.now().second
        return b'%d\n\r'%dt
    else:
        return mode
        
class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("WMORE")
        self.resize(900, 600)

        # Create widgets
        self.format_button = QtWidgets.QPushButton("Format")
        self.rtc_button = QtWidgets.QPushButton("Set RTC")
        self.refresh_button = QtWidgets.QPushButton("Refresh")
        self.send_button = QtWidgets.QPushButton("Send")
        self.download_button = QtWidgets.QPushButton("Download Data")
        # create consol interaction widget
        self.command_line_edit = QtWidgets.QLineEdit()
        self.serial_output = QtWidgets.QTextEdit()
        # Create a list box widget
        self.list_widget = QtWidgets.QListWidget(self)
        
        # Create layouts
        central_widget = QtWidgets.QWidget()
        main_layout = QtWidgets.QVBoxLayout(central_widget)
        consol_input_layout = QtWidgets.QHBoxLayout()
        consol_interaction_layout = QtWidgets.QVBoxLayout()
        com_consol_input_layout = QtWidgets.QHBoxLayout()
        button_layout = QtWidgets.QHBoxLayout()
        
        #add buttons to button layout
        button_layout.addWidget(self.refresh_button)
        button_layout.addWidget(self.rtc_button)
        button_layout.addWidget(self.download_button)
        button_layout.addWidget(self.format_button)
        
        # Add command input and send button to consol input
        consol_input_layout.addWidget(self.command_line_edit)
        consol_input_layout.addWidget(self.send_button)
        
        # Add serial output and consol input layout to consol interation layout
        consol_interaction_layout.addWidget(self.serial_output)
        consol_interaction_layout.addLayout(consol_input_layout)
        
        com_consol_input_layout.addWidget(self.list_widget)
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
        
        self.refreshDevices()

        # Connect signals and slots
        self.format_button.clicked.connect(lambda: self.sendCmds(format_commands))
        self.rtc_button.clicked.connect(lambda: self.sendCmds(rtc_commands))
        self.refresh_button.clicked.connect(self.refreshDevices)
        self.send_button.clicked.connect(lambda: self.send_command(self.command_line_edit.text()+"\n\r"))
        self.download_button.clicked.connect(self.download)
        self.command_line_edit.returnPressed.connect(lambda: self.send_command(self.command_line_edit.text()+"\n\r"))

        
    def download(self):
        try:
            portn = self.list_widget.selectedItems()[0].text().split("COM")[1]
            print(portn)
            self.disconnect_serial()
            self.list_widget.clearSelection()
            QCoreApplication.processEvents()
            macro_full_path = os.path.abspath("Teraterm_files\\script.ttl")
            subprocess.run(f'"C:\\Program Files (x86)\\teraterm\\ttermpro.exe" /C={portn} /BAUD=115200 /M="{macro_full_path}"')
        except:
            QtWidgets.QMessageBox.critical(self,"No COM Port", "No selected device found.\nPlease select a device first.")

    def sendCmds(self,commands):
        for cmd in commands:
            cmd = getDate(cmd)
            self.serial.write(cmd)
            QCoreApplication.processEvents()  # allow the event loop to run
            time.sleep(0.5)
            print(cmd)
 

    
    def refreshDevices(self):
        
        self.list_widget.clear()
        # Populate com_port_combo_box with available COM ports
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if self.connect_serial(port.device):
                self.disconnect_serial()
                self.list_widget.addItem(port.device)
                # self.com_port_combo_box.addItem(port.device)

    def update_selected_text(self):
        # Get the selected text from the list box and display it in the window's title bar
        selected_items = self.list_widget.selectedItems()
        if selected_items:
            selected_text = selected_items[0].text()
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
        self.serial.close()
        self.serial_output.append("Disconnected\n")
        

    def send_command(self,command):
        print("--")
        print(command.encode())
        print("--")
        self.serial.write(command.encode())
        # if self.serial.waitForReadyRead(10000): 
            # wait up to 1 second for data to be available for reading
        data = self.serial.readAll().data().decode()
        self.serial_output.insertPlainText(data)
        # else:
        #     QtWidgets.QMessageBox.warning(self, "Error", "Timeout waiting for response from device.")
            # break
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
