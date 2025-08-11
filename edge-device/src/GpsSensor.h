#ifndef __GPS_SENSOR_H__
    #define __GPS_SENSOR_H__

#include <SoftwareSerial.h>
#include "dataStructures.h"

// GpsSensor class
class GpsSensor {
public:
    GpsData gpsData;
    
    GpsSensor(SoftwareSerial& sim808Serial);
    bool setup();
    bool updateGps();
    int8_t getSignalStrength();
    int8_t getBatteryStatus();
    Datetime getDatetime();

private:
    SoftwareSerial& sim808Serial;
    bool sendDataCommand(const char* command, char* response, size_t responseSiz, bool verbose = false);
    bool sendControllCommand(const char* command);

   
};

#endif