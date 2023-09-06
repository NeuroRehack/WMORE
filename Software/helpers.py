"""
File: helpers.py
Author: Sami Kaab
Date: April 03, 2023
Description:   helper functions to be used by HUB.py
"""

from PySide2 import QtCore, QtGui, QtWidgets, QtSerialPort
from datetime import datetime
import configparser
import os
from multiprocessing import Pool


from constants import *

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
    """Retrieves the value of the specified variable in the specified config file

    Args:
        iniFilePath (str): path to the .ini file
        section (str): name of the section the variable is under
        var (str): name of the variable

    Returns:
        str: the value of the variable
    """
    config = configparser.ConfigParser()
    config.read(iniFilePath)
    value = config.get(section,var)
    return value
    
def set_config(iniFilePath,section,var,val):
    """Opens the specified .ini file and sets the value of the specified variable

    Args:
        iniFilePath (str): path to the .ini file
        section (str): name of the section the variable is under
        var (str): name of the variable
        val (str): value to set the variable to
    """
    config = configparser.ConfigParser()
    config.read(iniFilePath)
    config.set(section,var,val)
    with open(iniFilePath, 'w') as config_file:
        config.write(config_file)
        
def search_directory(root):
    """search computer for location  of Tera term exe file

    Args:
        root (str): path to the folder in which to look for

    Returns:
        str: location of the Tera Term exe file
    """
    for dirpath, dirnames, filenames in os.walk(root):
        if TERATERM in filenames:
            return os.path.join(dirpath, TERATERM)
        
def set_tera_term_location(window):
    """Prompts the user to select the location of the Tera Term exe file

    Returns:
        str: location of the Tera Term exe file            
    """
    teratermPath=""
    # Prompt user to select tera term exe file location
    teratermPath, _ = QtWidgets.QFileDialog.getOpenFileName(caption='Select teraterm location',
                                                                    filter='Executable Files (*.exe)',
                                                                    options=QtWidgets.QFileDialog.Options())
    if os.path.split(teratermPath)[-1]=="ttermpro.exe":
        answer = QtWidgets.QMessageBox.question(window, 
                                                "Confirmation",
                                                f"Set teraterm exe file path to:\n{teratermPath}",
                                                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No | QtWidgets.QMessageBox.Cancel,
                                                QtWidgets.QMessageBox.Yes)
        if answer == QtWidgets.QMessageBox.Yes:
            set_config(CONFIG_PATH,'paths', 'teraterm_location', teratermPath)
        elif answer == QtWidgets.QMessageBox.No:
            set_tera_term_location(window)
        else:
            QtWidgets.QMessageBox.warning(window,"Download Feature","You will not be able to download data from the WMORE until you set the path.")        
    else:
        answer = QtWidgets.QMessageBox.critical(window,"Wrong Path","The file you have chosen is invalid.\nThe executable for Tera Term should be called ttermpro.exe\nTry again?",QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No)  
        if answer == QtWidgets.QMessageBox.Yes:
            set_tera_term_location(window)
        else:
            QtWidgets.QMessageBox.warning(window,"Download Feature","You will not be able to download data from the WMORE until you set the path.")        
            return teratermPath
            
    return teratermPath

def check_tera_term(window):
    """Checks that the path to the Tera Term exe has been set, allows the
        user to set it if not.

    Returns:
        str: The path to the tera term exe file
    """
    teratermPath = get_config(CONFIG_PATH,'paths', 'teraterm_location')
            
    if os.path.split(teratermPath)[-1] != 'ttermpro.exe':
        # let user find where Tera Term is located
        answer = QtWidgets.QMessageBox.question(window, 
                                                "Tera Term",
                                                f"Tera Term seems to be missing.\nYou will not be able to download data from the WMORE until you install Tera Term.\nDo you wan to set the path to Tera Term now?",
                                                QtWidgets.QMessageBox.Yes | QtWidgets.QMessageBox.No,
                                                QtWidgets.QMessageBox.Yes)
        if answer == QtWidgets.QMessageBox.Yes:
            teratermPath = set_tera_term_location(window)
            
        else:
            QtWidgets.QMessageBox.warning(window,"Download Feature","You will not be able to download data from the WMORE until you set the path.")   
                 
    if os.path.split(teratermPath)[-1] != 'ttermpro.exe':
        teratermPath = None
        
    return teratermPath

def check_tera_term_automate():
    """Checks that the path to the Tera Term exe has been set, and searches for it

    Returns:
        str: The path to the tera term exe file
    """
    teratermPath = get_config(CONFIG_PATH,'paths', 'teraterm_location')
    # check if the path exists
    if not os.path.exists(teratermPath):
        teratermPath = None
    search_path = ROOT
    if teratermPath is None:
        # try to find where tera term is located
        with Pool() as pool:
            results = pool.map(search_directory, [os.path.join(search_path, d) for d in os.listdir(search_path)])
        for r in results:
            if r:
                teratermPath = r
                set_config(CONFIG_PATH,'paths', 'teraterm_location', teratermPath)
                return teratermPath
    
    return teratermPath
        