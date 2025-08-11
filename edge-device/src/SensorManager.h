#ifndef __SENSOR_MANAGER_H__
    #define __SENSOR_MANAGER_H__


#include "datastructures.h"
#include "GpsSensor.h"
#include "MpuSensor.h"

class SensorManager {
public:

    SensorManager(SoftwareSerial& sim808Serial);
    void setup();
    void update(unsigned long now);
    VehicleStatus getVehicleStatus();
    

private:
    GpsSensor gpsSensor;
    MpuSensor mpuSensor;
    unsigned long lastGpsPeriod;
    bool isGpsUpdated;
};

#endif