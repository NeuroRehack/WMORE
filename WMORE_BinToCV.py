import time
import struct
import os
from tqdm import tqdm
import numpy as np
import glob
import multiprocessing
import pandas as pd


def binToCSV(file_path):
    num_imu_vars = 10  # 10 x int16 variables from IMU
    num_uint8_vars = 16  # 16 x various uint8 variables
    num_uint8_line = 40  # 40 x uint8 per line encoding 27 human-readable variables

    # Create a row vector for extracted variables
    formatted_data = [0] * 27  # 27 human-readable variables per line

    # set the fprintf format of the 27 human-readable variables per line
    line_format = "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"

    # Create headings for the output file
    line_heading_1 = "ax,ay,az,gx,gy,gz,mx,my,mz,temp,"
    line_heading_2 = "valid,"
    line_heading_3 = "g_year,g_month,g_day,g_hour,g_minute,g_second,g_hund,"
    line_heading_4 = "l_year,l_month,l_day,l_hour,l_minute,l_second,l_hund,"
    line_heading_5 = "battery,period"
    
    # in_file = file_prefix + ".bin";
    in_file = file_path
    out_file = os.path.join(os.path.split(file_path)[0], os.path.split(file_path)[1][:16] + ".csv")
    with open(in_file, 'rb') as file_id:
        # # # # # # # # # # # # # # # # # # # # print("Processing file {}".format(in_file))
        # read binary file into vector of uint8
        raw_data = file_id.read()
        
    # determine number of lines of data
    max_index = int(len(raw_data) / num_uint8_line)


    with open(out_file, 'w') as file_id:
        # Print headings to file 
        file_id.write(line_heading_1)
        file_id.write(line_heading_2)
        file_id.write(line_heading_3)
        file_id.write(line_heading_4)
        file_id.write(line_heading_5)
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
            
if __name__ == '__main__':      

    start_time = time.time()

    # file_name = "230216_094356_05.bin"
    data_dir = "C://Users//uqskaab//OneDrive - The University of Queensland//Documents//_Programing//230221_WMORE_dataconversion//data"

    

    # file_path = os.path.join(data_dir,file_name)

    extension = ".bin"
    files = glob.glob(os.path.join(data_dir, f"*{extension}"))
    # [print(file) for file in files]
    # print(len(files))
    # binToCSV(files[0])
    with multiprocessing.Pool() as pool:
        with tqdm(total=len(files)) as pbar:
            for result in pool.imap_unordered(binToCSV, files):
                pbar.update()
                
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
