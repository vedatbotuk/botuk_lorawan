/*
   botuk_lorawan
   Copyright (c) 2024 Vedat Botuk.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 3.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <EEPROM.h>
#include "LoRaWan_APP.h"
#include "Wire.h"
#include <AM2302-Sensor.h>
#include "../credentials.h"

#define EEPROM_SIZE 6  // 2 bytes for temperature (int16_t), 1 byte for humidity (uint8_t), 1 byte for battery (uint8_t)

constexpr unsigned int SENSOR_PIN{48};
AM2302::AM2302_Sensor am2302{SENSOR_PIN};

int16_t currentTemperature;
uint8_t currentHumidity;
uint8_t currentBatteryPercentage;
bool sendData = true;

/* OTAA parameters */
uint8_t devEui[] = {DEV_EUI_0, DEV_EUI_1, DEV_EUI_2, DEV_EUI_3, DEV_EUI_4, DEV_EUI_5, DEV_EUI_6, DEV_EUI_7};
uint8_t appEui[] = {APP_EUI_0, APP_EUI_1, APP_EUI_2, APP_EUI_3, APP_EUI_4, APP_EUI_5, APP_EUI_6, APP_EUI_7};
uint8_t appKey[] = {APP_KEY_0, APP_KEY_1, APP_KEY_2, APP_KEY_3, APP_KEY_4, APP_KEY_5, APP_KEY_6, APP_KEY_7,
                    APP_KEY_8, APP_KEY_9, APP_KEY_10, APP_KEY_11, APP_KEY_12, APP_KEY_13, APP_KEY_14, APP_KEY_15
                   };

/* ABP parameters */
uint8_t nwkSKey[] = {NWK_SKEY_0, NWK_SKEY_1, NWK_SKEY_2, NWK_SKEY_3, NWK_SKEY_4, NWK_SKEY_5, NWK_SKEY_6, NWK_SKEY_7,
                     NWK_SKEY_8, NWK_SKEY_9, NWK_SKEY_10, NWK_SKEY_11, NWK_SKEY_12, NWK_SKEY_13, NWK_SKEY_14, NWK_SKEY_15
                    };
uint8_t appSKey[] = {APP_SKEY_0, APP_SKEY_1, APP_SKEY_2, APP_SKEY_3, APP_SKEY_4, APP_SKEY_5, APP_SKEY_6, APP_SKEY_7,
                     APP_SKEY_8, APP_SKEY_9, APP_SKEY_10, APP_SKEY_11, APP_SKEY_12, APP_SKEY_13, APP_SKEY_14, APP_SKEY_15
                    };
uint32_t devAddr = DEV_ADDR;

/* LoRaWAN configuration */
uint16_t userChannelsMask[6] = {0x0000, 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000};
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;
uint32_t appTxDutyCycle = 30000;
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

/* Prepares the payload to send */
static void prepareTxFrame(uint8_t port)
{
  // Read sensor values
  auto status = am2302.read();

  /* TEMPERATURE */
  float temperature = (float)(am2302.get_Temperature());
  int16_t scaledTemperature = (int16_t)(temperature * 100);  // Convert temperature to int16 by scaling
  Serial.printf("Temperature value = %d\n", scaledTemperature);

  /* HUMIDITY */
  float humidity = (float)(am2302.get_Humidity());
  uint8_t scaledHumidity = (uint8_t)(humidity + 0.5);  // Round humidity to the nearest integer
  Serial.printf("Humidity value = %d\n", scaledHumidity);

  /* BATTERY */
  int16_t readValue = 0;
  for (int i = 0; i < 4; i++) {
    readValue += analogRead(3);
    delay(100);
  }
  int16_t voltage = (readValue / 4);
  uint8_t battery_percentage = calc_battery_percentage(voltage);
  Serial.printf("ADC millivolts value = %d\n", voltage);
  Serial.printf("Battery percentage = %d\n", battery_percentage);

  appDataSize = 6; // Payload size: 2 bytes for temperature (int16_t) + 1 byte for humidity (uint8_t) + 3 for battery data

  // Check if data has changed significantly before sending
  if (fabs(currentTemperature - scaledTemperature) >= 50 || fabs(currentHumidity - scaledHumidity) >= 1 || fabs(currentBatteryPercentage - battery_percentage) >= 1) {

    // Store values in the payload
    appData[0] = (currentTemperature >> 8) & 0xFF;  // Temperature High Byte
    appData[1] = currentTemperature & 0xFF;         // Temperature Low Byte
    appData[2] = currentHumidity;
    appData[3] = (voltage >> 8) & 0xFF;  // Voltage High Byte
    appData[4] = voltage & 0xFF;         // Voltage Low Byte
    appData[5] = battery_percentage;

    // Print encoded data for verification
    Serial.println("Encoded Data:");
    for (int i = 0; i < appDataSize; i++) {
      Serial.print("appData[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(appData[i], HEX);
    }

    // Update stored values
    currentTemperature = scaledTemperature;
    currentHumidity = scaledHumidity;
    currentBatteryPercentage = battery_percentage;

    // Save updated values to EEPROM
    EEPROM.put(0, currentTemperature);  // Save 2 bytes for temperature
    EEPROM.put(2, currentHumidity);     // Save 1 byte for humidity
    EEPROM.put(3, currentBatteryPercentage);  // Save 1 byte for battery
    EEPROM.commit();  // Commit changes to EEPROM

    sendData = true;
  } else {
    Serial.println("No changes detected. Data will not be sent.");
    sendData = false;
  }
}

void setup()
{
  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Load stored values from EEPROM
  EEPROM.get(0, currentTemperature);  // Load temperature
  EEPROM.get(2, currentHumidity);     // Load humidity
  EEPROM.get(3, currentBatteryPercentage);  // Load battery percentage

  // Print the loaded values
  Serial.printf("Stored Temperature: %d\n", currentTemperature);
  Serial.printf("Stored Humidity: %d\n", currentHumidity);
  Serial.printf("Stored Battery Percentage: %d\n", currentBatteryPercentage);

  analogReadResolution(12);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  // Initialize the sensor and check if it's connected
  if (am2302.begin()) {
    delay(3000);  // Delay to receive valid data on immediate read
  } else {
    while (true) {
      Serial.println("Error: Sensor not detected. Please check sensor connection.");
      delay(10000);  // Retry every 10 seconds
    }
  }
}

void loop()
{
  switch (deviceState)
  {
    case DEVICE_STATE_INIT:
      {
#if (LORAWAN_DEVEUI_AUTO)
        LoRaWAN.generateDeveuiByChipID();
#endif
        LoRaWAN.init(loraWanClass, loraWanRegion);
        LoRaWAN.setDefaultDR(3);  // Set default data rate when ADR is off
        break;
      }
    case DEVICE_STATE_JOIN:
      {
        LoRaWAN.join();
        break;
      }
    case DEVICE_STATE_SEND:
      {
        prepareTxFrame(appPort);
        if (sendData) {
          LoRaWAN.send();
        }
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
    case DEVICE_STATE_CYCLE:
      {
        // Schedule the next packet transmission with random offset
        txDutyCycleTime = appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
    case DEVICE_STATE_SLEEP:
      {
        LoRaWAN.sleep(loraWanClass);
        break;
      }
    default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }
}

#define VOLTAGE
