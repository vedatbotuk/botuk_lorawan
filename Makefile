# Makefile for Arduino ESP32S3 (Wireless Stick Lite (V3))

# Define the board and port
PROJECT_NAME=botuk_lorawan
BOARD=Heltec-esp32:esp32:heltec_wifi_lora_32_V3
PORT=/dev/ttyUSB0
BAUD_RATE=115200

# Define the sketch file (refer to the correct sketch path)
SKETCH=main/main.ino

# Define the Arduino CLI command
ARDUINO_CLI=arduino-cli

# Compile the sketch
compile:
	@echo "Compiling the sketch..."
	$(ARDUINO_CLI) compile --fqbn $(BOARD) $(SKETCH)
	@echo "Compilation complete."

# Upload the sketch to the board
upload:
	@echo "Uploading the sketch to the board..."
	$(ARDUINO_CLI) upload -p $(PORT) --fqbn $(BOARD) $(SKETCH)
	@echo "Upload complete."

# Monitor the serial output
monitor:
	@echo "Starting serial monitor at $(BAUD_RATE) baud on port $(PORT)..."
	$(ARDUINO_CLI) monitor -c $(BAUD_RATE) -p $(PORT)

# Clean the build files
clean:
	@echo "Skipping removal of the build directory as it is not required by arduino-cli."
	rm -rf build/

.PHONY: compile upload monitor clean all

# Default target
all: compile upload
