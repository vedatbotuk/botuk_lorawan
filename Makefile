# Makefile for Arduino ESP32S3 (Wireless Stick Lite (V3))

# Define the board and port
PROJECT_NAME=botuk_lorawan
BOARD=Heltec-esp32:esp32:heltec_wifi_lora_32_V3
PORT=/dev/ttyUSB0

# Define the sketch file (refer to the correct sketch path)
SKETCH=main/main.ino

# Define the Arduino CLI command
ARDUINO_CLI=arduino-cli

# Compile the sketch
compile:
	$(ARDUINO_CLI) compile --fqbn $(BOARD) $(SKETCH)

# Upload the sketch to the board
upload:
	$(ARDUINO_CLI) upload -p $(PORT) --fqbn $(BOARD) $(SKETCH)

# Monitor the serial output
monitor:
	$(ARDUINO_CLI) monitor -p $(PORT)

# Clean the build files
clean:
	rm -rf build/

.PHONY: compile upload monitor clean all

# Default target
all: compile upload
