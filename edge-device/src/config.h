#include <Arduino.h>

#ifndef __CONFIG_H__
#define __CONFIG_H__

//tiny gps library settings
#define TINY_GSM_MODEM_SIM808
//#define TINY_GSM_DEBUG Serial

// Log Settings
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_NONE  0

#define LOG_LEVEL LOG_LEVEL_DEBUG

#define LOG_BUFFER_SIZE 256
#define LOG_SERIAL_BUS_FREQUENCY 9600

// GPRS and Mqtt Configuration
extern const char* const APN;
extern const char* const GPRS_USER;
extern const char* const GPRS_PASS;

extern const char* const MQTT_SERVER;
extern const uint16_t MQTT_PORT;
extern const char* const MQTT_TOPIC;
extern const char* const MQTT_CLIENT_ID;

extern const char* const EMERGENCY_PHONE_NUMBER;

// SIM808 Pins
constexpr int SIM808_RX_PIN = 13;
constexpr int SIM808_TX_PIN = 12;
constexpr unsigned long SIM808_BAUD_RATE = 9600;

// Timing Periods (in milliseconds)
constexpr unsigned long GPS_UPDATE_INTERVAL = 1000;
constexpr unsigned long MPU_UPDATE_INTERVAL = 20;
constexpr unsigned long MODEM_UPDATE_INTERVAL = 500;

// Mpu consts
constexpr float GRAVITY_ACCELERATION = 9.80665f;

// MQTT Transmission Settings
constexpr unsigned long MQTT_SEND_INTERVALS[3] = {30000, 150000, 300000};
constexpr bool MQTT_ENABLE_SMS[3] = {false, false, true};
constexpr int MQTT_WORST_STABILITY_STATUS = 3;
constexpr unsigned long MQTT_RECONNECTION_INTERVAL = 5000;
constexpr unsigned long NETWORK_RECONNECTION_INTERVAL = 30000;

#endif 
