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

#include "../credentials.h"
#include "LoRaWan_APP.h"
#include "Wire.h"

#define EEPROM_SIZE \
  7  // 2 bytes for temperature (int16_t), 1 byte for humidity (uint8_t), 1
     // byte
     // for battery (uint8_t), 1 byte for cycleCount (uint8_t)

// Treshold for sending data
uint8_t treshBattery = 1;
uint8_t treshTemperature = 100;
uint8_t treshHumidity = 2;

constexpr unsigned int SENSOR_PIN{48};
AM2302::AM2302_Sensor am2302{SENSOR_PIN};

int16_t currentTemperature;
uint8_t currentHumidity;
int16_t voltage;
uint8_t currentBatteryPercentage;
uint8_t battery_percentage;
uint8_t cycleCount;
bool sendData = true;
bool sendBattery;

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
uint16_t userChannelsMask[6] = {0x0000, 0x0000, 0x00FF, 0x0000, 0x0000, 0x0000};

/*LoraWan region, select in arduino IDE tools*/

LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = CLASS_A;

/*the application data transmission duty cycle.  value in [ms].*/
uint32_t appTxDutyCycle = 900000;  // 900000 -> 15 minutes

/*OTAA or ABP*/
bool overTheAirActivation = true;
bool loraWanAdr = true;
bool isTxConfirmed = true;
uint8_t appPort = 2;

uint8_t confirmedNbTrials = 2;

/* Prepares the payload of the frame */
static void prepareTxFrame(uint8_t port) {
  auto status = am2302.read();

  /* TEMPERATURE */
  float temperature = (float)(am2302.get_Temperature());
  // Temperatur in int16 umwandeln, indem wir sie mit 100 multiplizieren und
  // runden
  int16_t scaledTemperature = (int16_t)(temperature * 100);

  Serial.printf("Temperature value = %d\n", scaledTemperature);

  /* HUMIDITY */
  float humidity = (float)(am2302.get_Humidity());
  uint8_t scaledHumidity =
      (uint8_t)(humidity + 0.5);  // Runden auf nächstgelegene Ganzzahl

  Serial.printf("Humidity value = %d\n", scaledHumidity);

  /* BATTERY */

  // Every 48 cycles, read the battery voltage. When 15 min a cycle, it will
  // be 12 hours
  // check sendBattery if true only for device start
  if (cycleCount % 48 == 0) {
    int16_t readValue = 0;
    for (int i = 0; i < 4; i++) {
      readValue += analogRead(3);
      delay(100);
    }
    voltage = (readValue / 4);
    battery_percentage = calc_battery_percentage(voltage);

    Serial.printf("ADC millivolts value = %d\n", voltage);
    Serial.printf("Battery percentage = %d\n", battery_percentage);

    if (fabs(currentBatteryPercentage - battery_percentage) >= treshBattery) {
      currentBatteryPercentage = battery_percentage;
      sendBattery = true;
      EEPROM.put(3, voltage);  // Write 2 bytes starting from address 3
      EEPROM.put(5, currentBatteryPercentage);  // Write 1 byte at address 5
      EEPROM.commit();
    }

    cycleCount = 0;  // Reset cycleCount to 0 after reading the voltage
  }
  cycleCount++;
  EEPROM.put(6, cycleCount);  // Write cycleCount to EEPROM address 6
  EEPROM.commit();

  Serial.printf("cycleCount value = %d\n", cycleCount);
  Serial.printf("sendBattery value = %d\n", sendBattery);

  if (fabs(currentTemperature - scaledTemperature) >= treshTemperature ||
      fabs(currentHumidity - scaledHumidity) >= treshHumidity) {
    sendData = true;

    // First refresh new values
    currentTemperature = scaledTemperature;
    currentHumidity = scaledHumidity;
    // Write the updated values back to EEPROM
    EEPROM.put(0, currentTemperature);  // Write 2 bytes starting from address 0
    EEPROM.put(2, currentHumidity);     // Write 1 byte at address 2
    EEPROM.put(7, sendBattery);         // Write 1 byte at address 7
    EEPROM.commit();

    if (sendBattery == true) {
      appDataSize = 6;
      Serial.println("Battery data will be sent.");
    } else {
      appDataSize = 3;
      Serial.println("Battery data will not be sent.");
    }

    appData[0] = (currentTemperature >> 8) & 0xFF;  // High Byte
    appData[1] = currentTemperature & 0xFF;         // Low Byte
    appData[2] = currentHumidity;

    if (sendBattery == true) {  // Only send battery percentage if it was read
      appData[3] = (voltage >> 8) & 0xFF;  // High Byte
      appData[4] = voltage & 0xFF;         // Low Byte
      appData[5] = battery_percentage;
    }
    sendBattery = false;
    EEPROM.put(7, sendBattery);  // Write 1 byte at address 7
    EEPROM.commit();

    // Print encoded data for verification
    Serial.println("Encoded Data:");
    for (int i = 0; i < appDataSize; i++) {
      Serial.print("appData[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(appData[i], HEX);
    }
  } else {
    Serial.println("No changes detected. Data will not be sent.");
    sendData = false;
  }
}

/*if true, next uplink will add MOTE_MAC_DEVICE_TIME_REQ */
void VextON(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
}

void VextOFF(void) {
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, HIGH);
}

void setup() {
  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  // Read the stored current values from EEPROM
  EEPROM.get(0, currentTemperature);  // Read 2 bytes starting from address 0
  EEPROM.get(2, currentHumidity);     // Read 1 byte at address 2
  EEPROM.get(3, voltage);             // Read 2 bytes starting from address 3
  EEPROM.get(5, currentBatteryPercentage);  // Read 1 byte at address 5
  EEPROM.get(6, cycleCount);   // Read cycleCount from EEPROM address 6
  EEPROM.get(7, sendBattery);  // Read cycleCount from EEPROM address 7

  // Print the retrieved current values
  Serial.println("Stored Temperature: " + String(currentTemperature));
  Serial.println("Stored Humidity: " + String(currentHumidity));
  Serial.println("Stored Battery Percentage: " +
                 String(currentBatteryPercentage));

  analogReadResolution(12);
  Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);
  // set pin and check for sensor
  if (am2302.begin()) {
    // this delay is needed to receive valid data,
    // when the loop directly read again
    // Serial.println("Info: sensor check.");
    delay(500);
  } else {
    while (true) {
      Serial.println("Error: sensor check. => Please check sensor connection!");
      delay(5000);
    }
  }
}

void loop() {
  switch (deviceState) {
    case DEVICE_STATE_INIT: {
#if (LORAWAN_DEVEUI_AUTO)
      LoRaWAN.generateDeveuiByChipID();
#endif
      LoRaWAN.init(loraWanClass, loraWanRegion);
      // both set join DR and DR when ADR off
      LoRaWAN.setDefaultDR(3);
      break;
    }
    case DEVICE_STATE_JOIN: {
      LoRaWAN.join();
      break;
    }
    case DEVICE_STATE_SEND: {
      prepareTxFrame(appPort);
      if (sendData == true) {
        LoRaWAN.send();
      }
      deviceState = DEVICE_STATE_CYCLE;
      break;
    }
    case DEVICE_STATE_CYCLE: {
      // Schedule next packet transmission
      txDutyCycleTime =
          appTxDutyCycle + randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
      LoRaWAN.cycle(txDutyCycleTime);
      deviceState = DEVICE_STATE_SLEEP;
      break;
    }
    case DEVICE_STATE_SLEEP: {
      LoRaWAN.sleep(loraWanClass);
      break;
    }
    default: {
      deviceState = DEVICE_STATE_INIT;
      break;
    }
  }
}

#define VOLTAGE_MAX 3700
// TODO find minimum voltage
#define VOLTAGE_MIN 2900
uint8_t calc_battery_percentage(int adc) {
  /*For 3V no calculating is necassary*/
  int battery_percentage =
      100 * ((float)adc - VOLTAGE_MIN) / (VOLTAGE_MAX - VOLTAGE_MIN);

  if (battery_percentage < 0) battery_percentage = 0;

  return battery_percentage;