"""
File: WMORE_BinToCSV.py
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

num_imu_vars = 10  # 10 x int16 variables from IMU
num_uint8_vars = 16  # 16 x various uint8 variables
num_uint8_line = 40  # 40 x uint8 per line encoding 27 human-readable variables

# set the fprintf format of the 27 human-readable variables per line
line_format = "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"

# Create headings for the output file
line_heading = "ax,ay,az,gx,gy,gz,mx,my,mz,temp,valid,g_year,g_month,g_day,g_hour,g_minute,g_second,g_hund,l_year,l_month,l_day,l_hour,l_minute,l_second,l_hund,battery,period"

def binToCSV(file_path):
    """
    Function to convert a binary file to CSV format.
    
    Args:
        file_path (str): Path to the binary file to be converted.
    """
    
    # set the name of the output csv file
    out_file = os.path.join(os.path.split(file_path)[0], os.path.split(file_path)[1][:16] + ".csv")
    with open(file_path, 'rb') as file_id:
        # read binary file
        raw_data = file_id.read()
        
    # determine number of lines of data
    max_index = int(len(raw_data) / num_uint8_line)

    # Create a row vector for extracted variables
    formatted_data = [0] * 27  # 27 human-readable variables per line
    
    # Create and open output csv file
    with open(out_file, 'w') as file_id:
        # Write heading to file 
        file_id.write(line_heading)
        file_id.write("\n")
        
        # process all lines of data
        for i in range(max_index):
            # extract a line of uint8 data 
            start = (i * num_uint8_line)
            end = i * num_uint8_line + (num_uint8_line)
            uint8_line = list(raw_data[start:end])
            
            # Extract 10 x int16 IMU sensor data from line in LSB, MSB pairs
            for j in range(num_imu_vars):
                temp = uint8_line[(2 * j)] + (uint8_line[2 * j + 1] * 256)
                formatted_data[j] =  np.int16(temp)
                
            # Extract 16 x uint8 from line
            for j in range(num_uint8_vars):
                formatted_data[j + 10] = int(uint8_line[j + 20])
                
            # Extract 1 x uint32 from line
            temp = uint8_line[36]
            temp += uint8_line[37] * 2**8
            temp += uint8_line[38] * 2**16
            temp += uint8_line[39] * 2**24
            formatted_data[26] = temp
            
            # print the data to the output text file
            file_id.write(line_format % tuple(formatted_data))
            
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
    data_dir = "C://Users//uqskaab//OneDrive - The University of Queensland//Documents//_Programing//230221_WMORE_dataconversion//data"
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
