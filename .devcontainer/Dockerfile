FROM ubuntu:22.04

# Avoid prompts from apt
ENV DEBIAN_FRONTEND=noninteractive

# Install basic tools and dependencies
RUN apt-get update && apt-get install -y \
    git \
    wget \
    curl \
    python3 \
    python3-pip \
    python3-venv \
    build-essential \
    cmake \
    ninja-build \
    gperf \
    ccache \
    dfu-util \
    device-tree-compiler \
    udev \
    usbutils \
    libusb-1.0-0 \
    libusb-1.0-0-dev \
    unzip \
    xz-utils \
    file \
    sudo \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
COPY .devcontainer/requirements.txt /tmp/requirements.txt
RUN pip3 install -r /tmp/requirements.txt

# Set up Arduino CLI
RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
RUN arduino-cli config init

# Add SparkFun board URL and update index
RUN arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/sparkfun/Arduino_Apollo3/main/package_sparkfun_apollo3_index.json
RUN arduino-cli core update-index --additional-urls https://raw.githubusercontent.com/sparkfun/Arduino_Apollo3/main/package_sparkfun_apollo3_index.json

# Install required cores
RUN arduino-cli core install arduino:avr
RUN arduino-cli core install arduino:mbed_nano

# Install SparkFun Apollo3 board support
RUN arduino-cli core install SparkFun:apollo3@2.2.1 --additional-urls https://raw.githubusercontent.com/sparkfun/Arduino_Apollo3/main/package_sparkfun_apollo3_index.json

# Install AmbiqSuiteSDK (required for Apollo3)
RUN git clone https://github.com/sparkfun/AmbiqSuiteSDK.git /root/Arduino/libraries/AmbiqSuiteSDK

# Install only essential Arduino libraries for WMORE - Exact names from README
RUN arduino-cli lib install "SparkFun 9DoF IMU Breakout - ICM 20948 - Arduino Library@1.2.12"
RUN arduino-cli lib install "SdFat@2.2.3"
RUN arduino-cli lib install "SparkFun I2C Mux Arduino Library@1.0.3"
RUN arduino-cli lib install "SparkFun CCS811 Arduino Library@2.0.3"
RUN arduino-cli lib install "SparkFun VL53L1X 4m Laser Distance Sensor@1.2.12"
RUN arduino-cli lib install "SparkFun BME280@2.0.9"
RUN arduino-cli lib install "SparkFun LPS25HB Pressure Sensor Library@1.1.1"
RUN arduino-cli lib install "SparkFun VEML6075 Arduino Library@1.1.5"
RUN arduino-cli lib install "SparkFun PHT MS8607 Arduino Library@1.0.5"
RUN arduino-cli lib install "SparkFun MCP9600 Thermocouple Library@1.0.5"
RUN arduino-cli lib install "SparkFun SGP30 Arduino Library@1.0.5"
RUN arduino-cli lib install "SparkFun VCNL4040 Proximity Sensor Library@1.0.4"
RUN arduino-cli lib install "SparkFun MS5637 Barometric Pressure Library@1.0.2"
RUN arduino-cli lib install "SparkFun High Precision Temperature Sensor TMP117 Qwiic@1.2.5"
RUN arduino-cli lib install "SparkFun u-blox GNSS Arduino Library@2.2.26"
RUN arduino-cli lib install "SparkFun Qwiic Scale NAU7802 Arduino Library@1.0.6"
RUN arduino-cli lib install "SparkFun SCD30 Arduino Library@1.0.20"
RUN arduino-cli lib install "SparkFun Qwiic Humidity AHT20@1.0.3"
RUN arduino-cli lib install "SparkFun SHTC3 Humidity and Temperature Sensor Library@1.1.4"
RUN arduino-cli lib install "SparkFun ADS122C04 ADC Arduino Library@1.0.4"
RUN arduino-cli lib install "SparkFun MicroPressure Library@1.0.1"
RUN arduino-cli lib install "SparkFun Particle Sensor Panasonic SN-GCJA5@1.0.1"
RUN arduino-cli lib install "SparkFun SGP40 Arduino Library@1.0.4"
RUN arduino-cli lib install "SparkFun SDP3x Arduino Library@1.0.4"
RUN arduino-cli lib install "BlueRobotics MS5837 Library@1.1.1"
RUN arduino-cli lib install "SparkFun Qwiic Button and Qwiic Switch Library@2.0.6"
RUN arduino-cli lib install "SparkFun Bio Sensor Hub Library@1.0.1"

# Verify SparkFun Apollo3 Artemis board is installed
RUN arduino-cli board listall | grep 'SparkFun:apollo3:sfe_artemis_atp'|| (echo 'SparkFun Artemis board not found!' && exit 1)

# Install BOSSA for flashing the Arduino Nano
RUN apt-get update && apt-get install -y \
    libwxgtk3.0-gtk3-dev \
    libreadline-dev \
    && rm -rf /var/lib/apt/lists/*
RUN git clone https://github.com/shumatech/BOSSA.git /tmp/bossa
WORKDIR /tmp/bossa
RUN make && make install
WORKDIR /

# Create a dedicated workspace directory
RUN mkdir -p /workspace
WORKDIR /workspace

# Add UART access permissions
RUN echo 'KERNEL=="ttyACM[0-9]*", GROUP="dialout", MODE="0666"' > /etc/udev/rules.d/99-serial.rules
RUN echo 'KERNEL=="ttyUSB[0-9]*", GROUP="dialout", MODE="0666"' >> /etc/udev/rules.d/99-serial.rules

# Clean up
RUN apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

WORKDIR /workspace

CMD ["/bin/bash"] 