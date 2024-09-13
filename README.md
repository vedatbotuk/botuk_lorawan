# Wireless Stick Lite (V3) Project
This project involves development using the Wireless Stick Lite (V3) SoC, utilizing the Arduino IDE. Below you will find instructions for setting up the project, hardware information, and how to edit the configuration files.

## Hardware Overview
Below is the pinout diagram for the device:
![Wireless Stick Lite V3 Pinout](HTIT-WSL_V3.png)

### Sensors and Pin Configurations

- **GPIO3** is used to measure the battery level.
- **GPIO48**: Connected to the **DHT22 sensor** for measuring **temperature** and **humidity**.

## Software Setup

### Arduino IDE Configuration

1. Download and install the Arduino IDE from the official [Arduino website](https://www.arduino.cc/en/software).
2. Configure your Arduino IDE for the Wireless Stick Lite (V3) board.
   - Add the necessary board package through the **Boards Manager**.
   - Select the correct board and port.

### Setting Up Credentials

In order to connect to wireless networks or other external systems, you need to provide credentials. These credentials are stored in a separate file called `credentials.h`.

To set this up:

1. Create a copy of the `credentials_sample.h` file.
2. Rename the new file to `credentials.h`.
3. Edit the file and replace the `0xXX` placeholders with the correct values for your system.
