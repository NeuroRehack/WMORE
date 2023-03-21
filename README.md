<a name="readme-top"></a>

# WMORE
This project aims at providing an opensource platform for synchronised inertial measurements.
This platform consist of a list of required components, firmware to run the components and a software user interface. 

This project consist of two sensor types: a logger and a coordinator. The coordinator's role is to initiate and stop recording on hte loggers as well as keep them synchronised.
## Table of Content
- [WMORE](#wmore)
  - [Table of Content](#table-of-content)
- [Requirements](#requirements)
- [Hardware](#hardware)
      - [Required components](#required-components)
- [](#)
  - [Firmware](#firmware)
    - [Artemis Openlog (OLA)](#artemis-openlog-ola)
- [](#-1)
# Requirements
# Hardware
#### Required components
This project was designed using the following components. Only the Arduino Nano and Artemis Openlog are required to run the firmware, the battery and switches can be swapped for different models. However the CAD models for the case will need to be modified or redesigned accordingly if you choose to do so.
 * [Arduino Nano ble 33 (sense)](https://store.arduino.cc/products/arduino-nano-33-ble-sense)
 * [Artemis Openlog](https://www.sparkfun.com/products/16832)
 * Slide switch https://www.digikey.com.au/en/products/detail/c-k/OS102011MS2QN1/411602
 * Tactile Switch https://www.digikey.com.au/en/products/detail/te-connectivity-alcoswitch-switches/1825910-6/1632536
 * [LiPo Battery](https://ecocell.com.au/product/lipo-500-503035/)

Use the wiring diagram below. **NOTE** Pay attention to the wiring difference between the Coordinator and Logger
![wiring Diagram](Documentation/WMORE_wiring_diagram.png)
The wiring diagram above is included [here](Documentation/Wiring_Diagram.pdf)


<p align="right">(<a href="#readme-top">back to top</a>)</p>

#
## Firmware
### Artemis Openlog (OLA)
#
The Coordinator and Logger OLA firmware applications are based on v2.3 of the Openlog Artemis application supplied by Sparkfun [https://github.com/sparkfun/OpenLog_Artemis]. Both the OLA firmware applications were developed using the Openlog Artemis board support package installed on version 1.8.12 of the Arduino IDE. 

The Coordinator and Logger OLA firmware applications are predominantly written in Arduino C++, but include calls to the AmbiqSuiteSDK (mirrored by Sparkfun at https://github.com/sparkfun/AmbiqSuiteSDK). The original Openlog Artemis application has been significantly modified for OSCAR to enable synchronisation and to reduce processing delays. Some of the original functionality has been lost as a result. 

An outline of the Arduino IDE setup process is as follows:
- [ ] Install Arduino IDE 1.8.12 or above
- [ ] Add Sparkfun URL to preferences: https://learn.sparkfun.com/tutorials/artemis-development-with-arduino
- [ ] Install Board Support for Sparkfun Apollo3 Artemis using Board Manager
- [ ] Select Redboard Artemis ATP
- [ ] Install libraries using Library Manager
    - [ ] Required: 
        
        Sparkfun 9DoF IMU Breakout – ICM-20948
    - [ ] Required but redundant (will not be needed once dead code is removed):
      - [ ] SparkFun_I2C_Mux_Arduino_Library
      - [ ] SparkFunCCS811
      - [ ] SparkFun_VL53L1X
      - [ ] SparkFunBME280
      - [ ] SparkFun_LPS25HB_Arduino_Library  
      - [ ] SparkFun_VEML6075_Arduino_Library 
      - [ ] SparkFun_PHT_MS8607_Arduino_Library 
      - [ ] SparkFun_MCP9600
      - [ ] SparkFun_SGP30_Arduino_Library 
      - [ ] SparkFun_VCNL4040_Arduino_Library 
      - [ ] SparkFun_MS5637_Arduino_Library
      - [ ] SparkFun_TMP117 
      - [ ] SparkFun_u-blox_GNSS_Arduino_Library 
      - [ ] SparkFun_Qwiic_Scale_NAU7802_Arduino_Library 
      - [ ] SparkFun_SCD30_Arduino_Library 
      - [ ] SparkFun_Qwiic_Humidity_AHT20 
      - [ ] SparkFun_SHTC3 
      - [ ] SparkFun_ADS122C04_ADC_Arduino_Library 
      - [ ] SparkFun_MicroPressure 
      - [ ] SparkFun_Particle_Logger_SN-GCJA5_Arduino_Library 
      - [ ] SparkFun_SGP40_Arduino_Library
      - [ ] SparkFun_SDP3x_Arduino_Library
      - [ ] MS5837
      - [ ] SparkFun_Qwiic_Button
      - [ ] SparkFun_Bio_Logger_Hub_Library 
      