#include "MpuSensor.h"

#include "config.h"
#include "utilities.h"
#include <Wire.h>



bool MpuSensor::setup() {

Wire.begin(); // SDA = 20, SCL = 21 for Arduino Mega
if (!mpu.setup(0x68)) return false;
mpu.setMagneticDeclination(4.73f); // Adjust as needed
mpu.selectFilter(QuatFilterSel::MADGWICK);
mpu.setFilterIterations(15);
mpu.ahrs(true);
mpu.calibrateAccelGyro();
delay(15000);
calibrate();


return true;

}

bool MpuSensor::update(unsigned long now) {

float dt = now - lastUpdate;

mpu.update();

if(dt < MPU_UPDATE_INTERVAL)
    return false;

dt /= 1000.0f; // Convert milliseconds to seconds

lastUpdate = now;

instantaneousAcceleration = {
    (mpu.getLinearAccX() - accelerationOffset.x) * GRAVITY_ACCELERATION,
    (mpu.getLinearAccY() - accelerationOffset.y) * GRAVITY_ACCELERATION,
    (mpu.getLinearAccZ() - accelerationOffset.z) * GRAVITY_ACCELERATION
};

smoothedAcceleration.x = alpha * instantaneousAcceleration.x + (1 - alpha) * smoothedAcceleration.x;
smoothedAcceleration.y = alpha * instantaneousAcceleration.y + (1 - alpha) * smoothedAcceleration.y;
smoothedAcceleration.z = alpha * instantaneousAcceleration.z + (1 - alpha) * smoothedAcceleration.z;


angularVelocity.x = mpu.getGyroX() - gyroOffset.x;
angularVelocity.y = mpu.getGyroY() - gyroOffset.y;
angularVelocity.z = mpu.getGyroZ() - gyroOffset.z;

orientation.x = mpu.getRoll() - orientationOffset.x;
orientation.y = mpu.getPitch() - orientationOffset.y;
orientation.z = mpu.getYaw() - orientationOffset.z;

displacement.x += (smoothedAcceleration.x * dt + velocity.x) / 2 * dt;
displacement.y += (smoothedAcceleration.y * dt + velocity.y) / 2 * dt;
displacement.z += (smoothedAcceleration.z * dt + velocity.z) / 2 * dt;

velocity.x += smoothedAcceleration.x * dt;
velocity.y += smoothedAcceleration.y * dt;
velocity.z += smoothedAcceleration.z * dt;

updateStationaryState();
return true;

}

void MpuSensor::updateStationaryState() {

float absoluteAcceleration = sqrt(
    instantaneousAcceleration.x * instantaneousAcceleration.x +
    instantaneousAcceleration.y * instantaneousAcceleration.y +
    instantaneousAcceleration.z * instantaneousAcceleration.z
);
float absoluteVelocity = sqrt(
    velocity.x * velocity.x +
    velocity.y * velocity.y +
    velocity.z * velocity.z
);
if(absoluteVelocity < 0.5 || absoluteAcceleration < 0.5){
    successiveStationaryState++;
}else{
    successiveStationaryState = 0;
}
if (successiveStationaryState > 150) 
    velocity = {0, 0, 0};

}



void MpuSensor::resetDisplacement(){
displacement = {0.0,0.0,0.0};

}

Vector MpuSensor::getNewLocation(float longitude, float latitude, float altitude) {
const float R = 6371000.0f; // Earth's radius in meters
float latRad = radians(latitude);
float deltaLat = (displacement.y / R) * (180.0f / PI);
float deltaLon = (displacement.x / (R * cos(latRad))) * (180.0f / PI);
float deltaAlt = displacement.z; // already in meters
return {
    longitude + deltaLon, // x
    latitude + deltaLat, // y
    altitude + deltaAlt  // z
};

}

Vector MpuSensor::getVelocity() { return velocity; }

Vector MpuSensor::getAcceleration() { return smoothedAcceleration; }

Vector MpuSensor::getOrientation() { return orientation; }

Vector MpuSensor::getAngularVelocity() { return angularVelocity; }



void MpuSensor::calibrate(){
unsigned long sampleCount = 0;

accelerationOffset = {0.0f, 0.0f, 0.0f};
gyroOffset = {0.0f, 0.0f, 0.0f};
orientationOffset = {0.0f, 0.0f, 0.0f};


unsigned long now = millis();
unsigned long start = now;
unsigned long last_update = now;
do {

    now = millis();
    mpu.update();

    if((now - last_update) < MPU_UPDATE_INTERVAL)
    continue;

    last_update = now;
    mpu.update();

    if((now - start) < 15000)
    continue;

    accelerationOffset.x += mpu.getLinearAccX();
    accelerationOffset.y += mpu.getLinearAccY();
    accelerationOffset.z += mpu.getLinearAccZ();
    gyroOffset.x += mpu.getGyroX();
    gyroOffset.y += mpu.getGyroY();
    gyroOffset.z += mpu.getGyroZ();

    sampleCount += 1;
} while((now - start) <= 30000);

float accScale = 1;
float gyroScale = 1;

accelerationOffset.x *= accScale / sampleCount;
accelerationOffset.y *= accScale / sampleCount;
accelerationOffset.z *= accScale / sampleCount;
gyroOffset.x *= gyroScale / sampleCount;
gyroOffset.y *= gyroScale / sampleCount;
gyroOffset.z *= gyroScale / sampleCount;
orientationOffset.x = - atan2(accelerationOffset.y, accelerationOffset.z) * RAD_TO_DEG + mpu.getRoll(); // Roll
orientationOffset.y = - atan2(-accelerationOffset.x, sqrt(accelerationOffset.y * accelerationOffset.y + accelerationOffset.z * accelerationOffset.z)) * RAD_TO_DEG + mpu.getPitch(); // Pitch
orientationOffset.z =  0;

Logger::vector("orientation offset", orientationOffset);
Logger::vector("accelation offset", accelerationOffset);
Logger::vector("gero offset", gyroOffset);
}

