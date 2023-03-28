import numpy as np
num_imu_vars = 10; # 10 x int16 variables from IMU
num_uint8_vars = 16; # 16 x various uint8 variables
num_uint8_line = 40; # 40 x uint8 per line encoding 27 human-readable variables 
# Create a row vector for extracted variables
np.array
formatted_data = np.arange(27) # 27 human-readable variables per line
# set the fprintf format of the 27 human-readable variables per line
line_format = "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"
# Create headings for the output file
line_heading_1 = "ax,ay,az,gx,gy,gz,mx,my,mz,temp,"
line_heading_2 = "valid,"
line_heading_3 = "g_year,g_month,g_day,g_hour,g_minute,g_second,g_hund,"
line_heading_4 = "l_year,l_month,l_day,l_hour,l_minute,l_second,l_hund,"
line_heading_5 = "battery,period"


fileName = "230215_175353_05.bin"

# open file
raw_data = open(fileName, 'rb')
print(f.read())
# create out file
# write to out file
