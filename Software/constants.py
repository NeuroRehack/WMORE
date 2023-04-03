"""
File: WMORE_HUB.py
Author: Sami Kaab
Date: March 28, 2023
Description:   Global constants to be used by HUB.py and helpers.py
"""
TERATERM = "ttermpro.exe"
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

ICON_PATH = "images\\WMORE_Icon.png"
SPLASH_PATH = "images\\WMORE.png"
CONFIG_PATH = "config.ini"