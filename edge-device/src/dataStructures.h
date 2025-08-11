#ifndef __DATA_STRUCTURES_H__
    #define __DATA_STRUCTURES_H__

#include <stdint.h>

struct Vector {
    float x;
    float y;
    float z;
};

struct Datetime {
    uint8_t second;       // 0–59
    uint8_t minute;       // 0–59
    uint8_t hour;         // 0–23
    uint8_t day;          // 1–31
    uint8_t month;        // 1–12
    int16_t year;         // e.g., 2025
};

struct GpsData {
    float latitude;
    float longitude;      // Degrees
    float altitude;       // Meters
    float speed;          // m/s
    float heading;        // Degrees
    unsigned long measureTime; // millis()
    uint8_t satellites;   // Number of satellites for validity check
};

struct VehicleStatus {
    Datetime time;
    Vector acceleration;     // m/s^2, global frame
    Vector velocity;         // m/s, global frame
    Vector angularVelocity;  // rad/s
    Vector orientation;      // Euler angles (roll, pitch, yaw in rad)
    Vector location;         // lat/lon
    bool isLocationDeadReckoned;
    unsigned long locationFreshness;
    int8_t signalStrength;   // 0 - 100 
    int8_t batteryStatus;    // Percentage (0–100) or negative value means adapter power supply
};

#endif
