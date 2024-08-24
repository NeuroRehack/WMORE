import smbus2
import time

# Constants for the DS3231 I2C address and registers
DS3231_I2C_ADDR = 0x68

# Function to convert BCD to decimal
def bcd_to_dec(bcd):
    return (bcd // 16) * 10 + (bcd % 16)

# Function to read time from RTC
def read_time(bus):
    # Read time data as BCD from RTC registers
    # 0x00: Seconds, 0x01: Minutes, 0x02: Hours
    data = bus.read_i2c_block_data(DS3231_I2C_ADDR, 0x00, 3)
    seconds = bcd_to_dec(data[0])
    minutes = bcd_to_dec(data[1])
    hours = bcd_to_dec(data[2])
    return hours, minutes, seconds

def main():
    # Initialize the I2C bus
    bus = smbus2.SMBus(1)  # 1 is usually the bus number for Omega 2

    while True:
        hours, minutes, seconds = read_time(bus)
        print(f"Current time: {hours:02d}:{minutes:02d}:{seconds:02d}")
        time.sleep(10)

if __name__ == "__main__":
    main()

