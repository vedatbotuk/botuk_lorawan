#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>

constexpr unsigned int SENSOR_PIN = 48;
constexpr unsigned int BATTERY_PIN = 3;

// Threshold for sending data
// If changes in humidity and temperature exceed thresholds, data will be sent
// on next wake up. Battery data will be sent only when temperature or humidity
// data will be sent.
constexpr uint8_t TRESH_BATTERY = 1;
constexpr int16_t TRESH_TEMPERATURE = 100;
constexpr uint8_t TRESH_HUMIDITY = 2;

// Maximum ADC value for battery measurement (in millivolts)
constexpr uint16_t VOLTAGE_MAX = 3700;
// Minimum ADC value for battery measurement (in millivolts)
constexpr uint16_t VOLTAGE_MIN = 2900;

constexpr uint32_t APP_TX_DUTY_CYCLE = 900000;  // 900000 -> 15 minutes
// Every 48 cycles, read the battery voltage. When 15(90000ms) min a cycle, it
// will be 12 hours
constexpr uint8_t BATTERY_READ_INTERVAL_CYCLES = 48;

#endif  // CONFIG_H