import serial.tools.list_ports
from PyQt5 import QtCore, QtGui, QtWidgets, QtSerialPort

class MainWindow(QtWidgets.QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Serial Communication")
        self.resize(600, 400)

        # Create widgets
        self.com_port_combo_box = QtWidgets.QComboBox()
        self.range_combo_box = QtWidgets.QComboBox()
        self.connect_button = QtWidgets.QPushButton("Connect")
        self.disconnect_button = QtWidgets.QPushButton("Disconnect")
        self.send_button = QtWidgets.QPushButton("Send")
        self.command_line_edit = QtWidgets.QLineEdit()
        self.serial_output = QtWidgets.QTextEdit()

        # Create layouts
        central_widget = QtWidgets.QWidget()
        main_layout = QtWidgets.QVBoxLayout(central_widget)
        button_layout = QtWidgets.QHBoxLayout()
        button_layout.addWidget(self.com_port_combo_box)
        button_layout.addWidget(self.range_combo_box)
        button_layout.addWidget(self.connect_button)
        button_layout.addWidget(self.disconnect_button)
        button_layout.addWidget(self.command_line_edit)
        button_layout.addWidget(self.send_button)
        main_layout.addLayout(button_layout)
        main_layout.addWidget(self.serial_output)
        self.setCentralWidget(central_widget)

        # Populate com_port_combo_box with available COM ports
        ports = serial.tools.list_ports.comports()
        for port in ports:
            # self.connect_serial
            self.com_port_combo_box.addItem(port.device)

        # Populate range_combo_box with available ranges
        self.range_combo_box.addItems(["+/- 2g", "+/- 4g", "+/- 8g", "+/- 16g"])

        # Connect signals and slots
        self.connect_button.clicked.connect(lambda: self.connect_serial(self.com_port_combo_box.currentText()))
        self.disconnect_button.clicked.connect(self.disconnect_serial)
        self.send_button.clicked.connect(self.send_command)

        # Initialize serial port
        self.serial = QtSerialPort.QSerialPort()
        self.serial.setBaudRate(QtSerialPort.QSerialPort.Baud115200)
        self.serial.readyRead.connect(self.read_serial)

    def connect_serial(self,port_name):
        info = QtSerialPort.QSerialPortInfo(port_name)
        product_id = info.productIdentifier()
        if product_id == 29987:  # replace with expected product ID
            self.serial.setPortName(port_name)
            if self.serial.open(QtCore.QIODevice.ReadWrite):
                self.serial_output.append("Connected to " + port_name)
            else:
                QtWidgets.QMessageBox.warning(self, "Error", "Failed to connect to " + port_name)
        else:
            QtWidgets.QMessageBox.warning(self, "Error", "Connected to unexpected device")

    def disconnect_serial(self):
        self.serial.close()
        self.serial_output.append("Disconnected")

    def send_command(self):
        range_key = self.range_combo_box.currentIndex()
        commands = ["m","3","6",str(range_key), "x","x"]
        for command in commands:
            command+="\n\r"
            self.serial.write(command.encode())
            if self.serial.waitForReadyRead(10000):  # wait up to 1 second for data to be available for reading
                data = self.serial.readAll().data().decode()
                self.serial_output.insertPlainText(data)
            else:
                QtWidgets.QMessageBox.warning(self, "Error", "Timeout waiting for response from device.")
                break
        self.command_line_edit.clear()

    def read_serial(self):
        try:
            data = self.serial.readAll().data().decode()
        except:
            data = "could not decode!"
        self.serial_output.insertPlainText(data)


if __name__ == "__main__":
    app = QtWidgets.QApplication([])
    window = MainWindow()
    window.show()
    app.exec_()
