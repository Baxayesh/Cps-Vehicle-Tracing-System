#include "SensorManager.h"
#include "utilities.h"

SensorManager::SensorManager(SoftwareSerial& sim808Serial) : 
    gpsSensor(sim808Serial),
    mpuSensor() {

    lastGpsPeriod = 0;
    isGpsUpdated = true;
}

void SensorManager::setup(){

    if (!gpsSensor.setup()) {
        Logger::warn("failed to initialize gps. program halted");
        while (1);
    }

    if(!mpuSensor.setup()){
        Logger::warn("failed to initialize mpu. program halted");
        while (1);
    }

}

void SensorManager::update(unsigned long now){

    mpuSensor.update(now);
      
    
    if((now - lastGpsPeriod) >= GPS_UPDATE_INTERVAL){
        lastGpsPeriod = now;
        isGpsUpdated = gpsSensor.updateGps();
        if(isGpsUpdated){
            mpuSensor.resetDisplacement();   
        }
    }
}





VehicleStatus SensorManager::getVehicleStatus() {

    VehicleStatus status;

    status.time = gpsSensor.getDatetime();
    status.signalStrength = gpsSensor.getSignalStrength();
    status.batteryStatus = gpsSensor.getBatteryStatus();

    status.velocity = mpuSensor.getVelocity();
    status.acceleration = mpuSensor.getAcceleration();
    status.orientation = mpuSensor.getOrientation();
    status.angularVelocity = mpuSensor.getAngularVelocity();

    status.isLocationDeadReckoned = !isGpsUpdated;
    if(status.isLocationDeadReckoned){
        status.location = mpuSensor.getNewLocation(
            gpsSensor.gpsData.longitude,
            gpsSensor.gpsData.latitude,
            gpsSensor.gpsData.altitude
        );
    }else{
        status.location = {
            gpsSensor.gpsData.longitude,
            gpsSensor.gpsData.latitude,
            gpsSensor.gpsData.altitude
        };
    }

    status.locationFreshness = millis() - gpsSensor.gpsData.measureTime;  

    return status;
}