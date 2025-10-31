"""
File: constants.py
Author: Sami Kaab
Date: March 28, 2023
Description:   Global constants to be used by WMORE_HUB.pyw, helpers.py and 
"""
TERATERM = "ttermpro.exe"
TERATERM_MACRO = "teratermMacro.ttl"
ROOT = "\\"
OPENLOG_PRODUCT_ID = 29987
# Define lists of commands to send to the openlogs
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
GET_ID_COMMANDS = [
    "m",
    "1",
    "x",
    "x"
]


MESSAGE_PATTERNS = {
    "M_FORMAT": "Format done",
    "M_FIRMWARE": "WMORE",
    "M_ID": "Sensor ID"
}
ICON_PATH = "images\\WMORE_Icon.png"
SPLASH_PATH = "images\\WMORE.png"
CONFIG_PATH = "config.ini"

# USED IN BinToCSV.py
# set the fprintf format of the 215 human-readable variables per line
LINE_FORMAT = (
    "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%d,%d\n"
)

# Create headings for the output file
LINE_HEADING = "ax,ay,az,gx,gy,gz,mx,my,mz,temp,valid,g_unix,l_unix,battery,period"

NUM_IMU_VARS = 10  # 10 x int16 variables from IMU
NUM_UNIT8_LINE = 36  # 36 x uint8 per line encoding 15 human-readable variables
