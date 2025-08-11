#include <Arduino.h>
#include "SensorManager.h"
#include "MqttClient.h"
#include "utilities.h"

// Example usage
SoftwareSerial sim808Serial(SIM808_RX_PIN, SIM808_TX_PIN);
SensorManager sensorManager(sim808Serial);
MqttClient mqttClient(sensorManager, sim808Serial);


void setup() {
    Logger::setup();    
    Logger::info("setup started");
    sim808Serial.begin(SIM808_BAUD_RATE);
    mqttClient.setup();
    sensorManager.setup();
    Logger::info("setup finished");
}

void loop() {

    unsigned long now = millis();

    sensorManager.update(now);

    mqttClient.update(now);

}