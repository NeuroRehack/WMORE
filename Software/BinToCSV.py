"""
File: BinToCSV.py
Author: Sami Kaab
Date: February 22, 2023
Description:    This program converts every .bin file in a given directory to .csv format. 
                This program was adapted from a Matlab script written by Nimish Panday.
To Do:
        add function to remove invalid lines in output csv
"""

import time
import struct
import os
from tqdm import tqdm
import numpy as np
import glob
import multiprocessing
import pandas as pd

from constants import *


def binToCSV(file_path):
    """
    Function to convert a binary file to CSV format.
    
    Args:
        file_path (str): Path to the binary file to be converted.
    """
    
    # set the name of the output csv file
    out_file = os.path.join(os.path.split(file_path)[0], os.path.split(file_path)[1][:17] + ".csv")
    with open(file_path, 'rb') as file_id:
        # read binary file
        raw_data = file_id.read()
        
    # determine number of lines of data
    max_index = int(len(raw_data) / NUM_UNIT8_LINE)

    # Create a row vector for extracted variables
    formatted_data = [0] * 13  # 13 human-readable variables per line
    
    # Create and open output csv file
    with open(out_file, 'w') as file_id:
        # Write heading to file 
        file_id.write(LINE_HEADING)
        file_id.write("\n")
        
        # process all lines of data
        for i in range(max_index):
            # extract a line of uint8 data 
            start = (i * NUM_UNIT8_LINE)
            end = i * NUM_UNIT8_LINE + (NUM_UNIT8_LINE)
            uint8_line = list(raw_data[start:end])
            
            # Extract 9 x int16 IMU sensor data from line in LSB, MSB pairs
            for j in range(NUM_IMU_VARS):
                temp = uint8_line[(2 * j)] + (uint8_line[2 * j + 1] * 256)
                formatted_data[j] =  np.int16(temp)

            # Extract validity byte
            formatted_data[9] = int(uint8_line[18])

            # Extract global unix time
            temp  = uint8_line[19]
            temp += uint8_line[20] * 2**8
            temp += uint8_line[21] * 2**16
            temp += uint8_line[22] * 2**24
            # Add hundredths
            temp += int(uint8_line[23]) / 100.0
            formatted_data[10] = round(temp, 2)
                
            # Extract local unix time
            temp  = uint8_line[24]
            temp += uint8_line[25] * 2**8
            temp += uint8_line[26] * 2**16
            temp += uint8_line[27] * 2**24
            # Add hundredths
            temp += int(uint8_line[28]) / 100.0
            formatted_data[11] = round(temp, 2)

            # Extract battery byte
            formatted_data[12] = int(uint8_line[29])
                
            # # Extract 1 x uint32 from line
            # temp = uint8_line[32]
            # temp += uint8_line[33] * 2**8
            # temp += uint8_line[34] * 2**16
            # temp += uint8_line[35] * 2**24
            # formatted_data[14] = temp
            
            # print the data to the output text file
            file_id.write(LINE_FORMAT % tuple(formatted_data))
            
def BatchConvert(data_dir):
    """
    Function to convert all binary files in a directory to CSV format using multiprocessing.
    
    Args:
        data_dir (str): Path to the directory containing binary files to be converted.
    """
    # Get list of file with the .bin extension in the set directory
    extension = ".bin"
    files = glob.glob(os.path.join(data_dir, f"*{extension}"))
    
    # Create a multiprocessing pool
    with multiprocessing.Pool() as pool:
        with tqdm(total=len(files)) as pbar:
            # Convert all files in the given directory
            for result in pool.imap_unordered(binToCSV, files):
                pbar.update()
                
def MergeData(data_dir):
    """
    Merges CSV files in a given directory into a single file and saves it as 'combined.csv'.

    Args:
        data_dir (str): Path to the directory containing the CSV files.

    Returns:
        None
    """
    # Get a list of all CSV files in the directory
    csv_files = glob.glob(os.path.join(data_dir, f"*.csv"))
    # Combine all CSV files into a single dataframe
    df = pd.concat([pd.read_csv(os.path.join(data_dir, f)) for f in tqdm(csv_files)])
    df.to_csv(os.path.join(data_dir,"%s_combined.csv"%os.path.split(data_dir)[-1]), index=False)
    
if __name__ == '__main__':      

    start_time = time.time()

    # Set input directory containing all the bin files
    data_dir = ""
    BatchConvert(data_dir)

    
                
    print("Merging data")
    # Get a list of all CSV files in the directory
    csv_files = glob.glob(os.path.join(data_dir, f"*.csv"))
    # Combine all CSV files into a single dataframe
    df = pd.concat([pd.read_csv(os.path.join(data_dir, f)) for f in tqdm(csv_files)])

    print("Writing merged data to file...")
    # Write the combined dataframe to a new CSV file
    df.to_csv(os.path.join(data_dir,"combined.csv"), index=False)

    end_time = time.time()
    elapsed_time = time.localtime(end_time - start_time)
    # Format the struct_time object as a string in the desired format
    time_str = time.strftime("%M:%S", elapsed_time)
    print("Done in: {}".format(time_str))
