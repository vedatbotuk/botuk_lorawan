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

#include <AM2302-Sensor.h>
#include <EEPROM.h>
#include <LoRaWan_APP.h>
#include <Wire.h>

#include "config.h"
#include "credentials.h"

AM2302::AM2302_Sensor am2302{SENSOR_PIN};

constexpr uint8_t EEPROM_SIZE = 5;

enum EEPROMAddress : uint8_t {
  EEPROM_ADDR_TEMPERATURE = 0,
  EEPROM_ADDR_HUMIDITY = 1,
  EEPROM_ADDR_BATTERY_PERCENT = 2,
  EEPROM_ADDR_CYCLE_COUNT = 3,
  EEPROM_ADDR_SEND_BATTERY = 4,
};

uint8_t currentTemperature = 0;
uint8_t currentHumidity = 0;
uint8_t currentBatteryPercentage = 100;
uint8_t cycleCount = 0;
bool sendData = false;
bool sendBattery = true;

static void saveMeasurementState() {
  EEPROM.put(EEPROM_ADDR_TEMPERATURE, currentTemperature);
  EEPROM.put(EEPROM_ADDR_HUMIDITY, currentHumidity);
  EEPROM.commit();
}

static void saveBatteryState() {
  EEPROM.put(EEPROM_ADDR_BATTERY_PERCENT, currentBatteryPercentage);
  EEPROM.put(EEPROM_ADDR_SEND_BATTERY, sendBattery);
  EEPROM.commit();
}

static void saveCycleCount() {
  EEPROM.put(EEPROM_ADDR_CYCLE_COUNT, cycleCount);
  EEPROM.commit();
}

/* OTAA para*/
uint8_t devEui[] = {DEV_EUI_0, DEV_EUI_1, DEV_EUI_2, DEV_EUI_3,
                    DEV_EUI_4, DEV_EUI_5, DEV_EUI_6, DEV_EUI_7};

uint8_t appEui[] = {APP_EUI_0, APP_EUI_1, APP_EUI_2, APP_EUI_3,
                    APP_EUI_4, APP_EUI_5, APP_EUI_6, APP_EUI_7};

uint8_t appKey[] = {APP_KEY_0,  APP_KEY_1,  APP_KEY_2,  APP_KEY_3,
                    APP_KEY_4,  APP_KEY_5,  APP_KEY_6,  APP_KEY_7,
                    APP_KEY_8,  APP_KEY_9,  APP_KEY_10, APP_KEY_11,
                    APP_KEY_12, APP_KEY_13, APP_KEY_14, APP_KEY_15};

/* ABP para*/
uint8_t nwkSKey[] = {NWK_SKEY_0,  NWK_SKEY_1,  NWK_SKEY_2,  NWK_SKEY_3,
                     NWK_SKEY_4,  NWK_SKEY_5,  NWK_SKEY_6,  NWK_SKEY_7,
                     NWK_SKEY_8,  NWK_SKEY_9,  NWK_SKEY_10, NWK_SKEY_11,
                     NWK_SKEY_12, NWK_SKEY_13, NWK_SKEY_14, NWK_SKEY_15};

uint8_t appSKey[] = {APP_SKEY_0,  APP_SKEY_1,  APP_SKEY_2,  APP_SKEY_3,
                     APP_SKEY_4,  APP_SKEY_5,  APP_SKEY_6,  APP_SKEY_7,
                     APP_SKEY_8,  APP_SKEY_9,  APP_SKEY_10, APP_SKEY_11,
                     APP_SKEY_12, APP_SKEY_13, APP_SKEY_14, APP_SKEY_15};

uint32_t devAddr = DEV_ADDR;

/*LoraWan channelsmask, default channels 0-7*/
uint16_t userChannelsMask[6] = {0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

/*LoraWan region, select in arduino IDE tools*/
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = APP_TX_DUTY_CYCLE;

/*OTAA or ABP*/
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 4;

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port) {
  if (!am2302.read()) {
    Serial.println("Warning: failed to read sensor. Skipping transmission.");
    sendData = false;
    return;
  }

  uint8_t scaledTemperature = readTemperature();
  uint8_t scaledHumidity = readHumidity();
  readBattery();

  bool temperatureChanged = abs(currentTemperature - scaledTemperature) >= THRESH_TEMPERATURE;
  bool humidityChanged = abs(currentHumidity - scaledHumidity) >= THRESH_HUMIDITY;

  if (!temperatureChanged && !humidityChanged) {
    Serial.println("No changes detected. Data will not be sent.");
    sendData = false;
    return;
  }

  sendData = true;
  currentTemperature = scaledTemperature;
  currentHumidity = scaledHumidity;
  saveMeasurementState();

  appData[0] = currentTemperature;
  appData[1] = currentHumidity;
  appDataSize = 2;

  if (sendBattery) {
    appData[2] = currentBatteryPercentage;
    appDataSize = 3;
    sendBattery = false;
    saveBatteryState();
    Serial.println("Battery data will be sent.");
  } else {
    Serial.println("Battery data will not be sent.");
  }

  Serial.println("Encoded Data:");
  for (int i = 0; i < appDataSize; i++) {
    Serial.printf("appData[%d] = %02X\n", i, appData[i]);
  }
}

void setup() {
  Serial.begin(115200);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(EEPROM_ADDR_TEMPERATURE, currentTemperature);
  EEPROM.get(EEPROM_ADDR_HUMIDITY, currentHumidity);
  EEPROM.get(EEPROM_ADDR_BATTERY_PERCENT, currentBatteryPercentage);
  EEPROM.get(EEPROM_ADDR_CYCLE_COUNT, cycleCount);
  EEPROM.get(EEPROM_ADDR_SEND_BATTERY, sendBattery);

  if (currentHumidity > 100) {
    currentHumidity = 0;
  }
  if (currentBatteryPercentage > 100) {
    currentBatteryPercentage = 100;
  }
  if (cycleCount >= BATTERY_READ_INTERVAL_CYCLES) {
    cycleCount %= BATTERY_READ_INTERVAL_CYCLES;
    saveCycleCount();
  }

  Serial.printf("Stored Temperature: %u\n", currentTemperature);
  Serial.printf("Stored Humidity: %u\n", currentHumidity);
  Serial.printf("Stored Battery Percentage: %u\n", currentBatteryPercentage);
  Serial.printf("Stored Cycle Count: %u\n", cycleCount);
  Serial.printf("Stored Send Battery: %u\n", sendBattery);

  analogReadResolution(12);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

  if (am2302.begin()) {
    delay(100);
  } else {
    while (true) {
      Serial.println("Error: sensor check. => Please check sensor connection!");
      delay(5000);
    }
  }
}

void loop() {
  switch (deviceState) {
    case DEVICE_STATE_INIT:
#if (LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass, loraWanRegion);
      LoRaWAN.setDefaultDR(3);
      break;

    case DEVICE_STATE_JOIN:
      LoRaWAN.join();
      break;

    case DEVICE_STATE_SEND:
      prepareTxFrame(appPort);
      if (sendData) {
        LoRaWAN.send();
      }
      deviceState = DEVICE_STATE_CYCLE;
      break;

    case DEVICE_STATE_CYCLE:
      txDutyCycleTime =
          appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;

    case DEVICE_STATE_SLEEP:
      LoRaWAN.sleep(loraWanClass);
      break;

    default:
      deviceState = DEVICE_STATE_INIT;
      break;
  }
}

uint8_t readTemperature() {
  float temperature = static_cast<float>(am2302.get_Temperature());
  uint8_t scaledTemperature = temperature_to_uint8(temperature);
  Serial.printf("Temperature value = %u\n", scaledTemperature);
  return scaledTemperature;
}

uint8_t temperature_to_uint8(float temperature) {
  if (temperature < TEMPERATURE_MIN) {
    temperature = TEMPERATURE_MIN;
  } else if (temperature > TEMPERATURE_MAX) {
    temperature = TEMPERATURE_MAX;
  }

  float normalized = (temperature - TEMPERATURE_MIN) / (TEMPERATURE_MAX - TEMPERATURE_MIN);
  return static_cast<uint8_t>(normalized * 255.0f + 0.5f);
}

uint8_t readHumidity() {
  float humidity = static_cast<float>(am2302.get_Humidity());
  uint8_t scaledHumidity = static_cast<uint8_t>(humidity + 0.5f);
  Serial.printf("Humidity value = %u\n", scaledHumidity);
  return scaledHumidity;
}

void readBattery() {
  uint8_t battery_percentage = 0;

  // Every 48 cycles, read the battery voltage. When 15 min a cycle, it will
  // be 12 hours
  if (cycleCount % BATTERY_READ_INTERVAL_CYCLES == 0) {
    int32_t readValue = 0;
    for (int i = 0; i < 4; i++) {
      readValue += analogRead(BATTERY_PIN);
      delay(100);
    }
    uint16_t voltage = static_cast<uint16_t>(readValue / 4);
    battery_percentage = calc_battery_percentage(voltage);

    Serial.printf("ADC millivolts value = %u\n", voltage);
    Serial.printf("Battery percentage = %u\n", battery_percentage);

    if (abs(static_cast<int>(currentBatteryPercentage) - static_cast<int>(battery_percentage)) >= THRESH_BATTERY) {
      currentBatteryPercentage = battery_percentage;
      sendBattery = true;
      saveBatteryState();
    }

    cycleCount = 0;
  }
  cycleCount = (cycleCount + 1) % BATTERY_READ_INTERVAL_CYCLES;
  saveCycleCount();

  Serial.printf("cycleCount value = %u\n", cycleCount);
  Serial.printf("sendBattery value = %u\n", sendBattery);
}

uint8_t calc_battery_percentage(uint16_t adc) {
  if (adc < VOLTAGE_MIN) {
    return 0;
  } else if (adc > VOLTAGE_MAX) {
    return 100;
  } else {
    int battery_percentage = 100 * (static_cast<float>(adc) - VOLTAGE_MIN) /
                             (VOLTAGE_MAX - VOLTAGE_MIN);
    return battery_percentage < 0 ? 0 : battery_percentage;
  }
}