import time
import serial
import subprocess

def receive_zmodem_files(port, baudrate):
    # Open the serial connection
    ser = serial.Serial(port, baudrate, timeout=1)

    # Wait for a moment to establish the connection
    time.sleep(2)

    # Send the commands
    commands = ["s\r", "s\r", "dir\r", "sz *\r", "x\r", "x\r", "\r"]
    for cmd in commands:
        ser.write(cmd.encode())
        time.sleep(1)  # Wait for the command to be processed

    # Close the serial connection
    ser.close()

    # Receive the Zmodem files using rz (lrzsz package should be installed)
    subprocess.run(["rz"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)

#
receive_zmodem_files(port="/dev/ttyUSB0", baudrate=9600)
