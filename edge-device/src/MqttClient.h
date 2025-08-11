#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#include "config.h"
#include "SensorManager.h"
#include <TinyGsmClient.h>
#include <PubSubClient.h>

class MqttClient {
public:
    MqttClient(SensorManager& sensorManager, SoftwareSerial& sim808Serial);
    void setup();
    void update(unsigned long now);

private:
    void sendMqttMessage(const VehicleStatus& data);
    void sendSms(const VehicleStatus& data);
    void adjustStablityState(bool success);
    bool checkConnection(unsigned long now);

    SoftwareSerial& sim808Serial;
    SensorManager& sensorManager;
    TinyGsm gsmModem;
    TinyGsmClient gsmClient;
    PubSubClient mqttClient;

    int8_t stablityState;
    unsigned long lastSendTime;
    unsigned long lastGprsUpdate;
    unsigned long lastConnectAttempt;
    unsigned long lastSuccessfullConnectionAttempt;
    uint8_t missCount;
    uint16_t successCount;
    
};

#endif