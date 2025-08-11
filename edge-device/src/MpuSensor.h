#ifndef __MPU_SENSOR_H__
    #define __MPU_SENSOR_H__

#include "config.h"
#include "utilities.h"
#include "dataStructures.h"
#include <MPU9250.h>
#include <Wire.h>



class MpuSensor {

private:

  MPU9250 mpu;
  Vector orientation = {0.0f, 0.0f, 0.0f};
  Vector velocity = {0.0f, 0.0f, 0.0f};
  Vector displacement = {0.0f, 0.0f, 0.0f};
  Vector angularVelocity = {0.0f, 0.0f, 0.0f};
  Vector instantaneousAcceleration = {0.0f, 0.0f, 0.0f};
  Vector smoothedAcceleration = {0.0f, 0.0f, 0.0f};
  Vector accelerationOffset = {0.0f, 0.0f, 0.0f};
  Vector gyroOffset = {0.0f, 0.0f, 0.0f};
  Vector orientationOffset = {0.0f, 0.0f, 0.0f};
  unsigned long successiveStationaryState = 0;
  unsigned long lastUpdate = 0;
  const float alpha = 0.1f; // EMA coefficient

 
public:

  bool setup();
  bool update(unsigned long now);
  void resetDisplacement();
  Vector getNewLocation(float latitude, float longitude, float altitude);
  Vector getVelocity();
  Vector getAcceleration();
  Vector getOrientation();
  Vector getAngularVelocity();
  void calibrate();

private:
  void updateStationaryState();


};

#endif