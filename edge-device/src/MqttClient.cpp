#include "MqttClient.h"
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200

MqttClient::MqttClient(SensorManager& sensorManager, SoftwareSerial& sim808Serial)
    :   sim808Serial(sim808Serial),
        sensorManager(sensorManager),
        gsmModem(sim808Serial),
        gsmClient(gsmModem),
        mqttClient(gsmClient),
        stablityState(0),
        lastSendTime(0),
        lastGprsUpdate(0),
        lastConnectAttempt(0),
        lastSuccessfullConnectionAttempt(0),
        missCount(0),
        successCount(0) {}


void MqttClient::setup() {


  Logger::info("Initializing modem...");
  if (!gsmModem.restart()) {
    Logger::warn("Failed to restart modem");
    while (true); // Halt if modem fails to initialize
  }

  // Wait for network
  Logger::info("Waiting for network...");
  if (!gsmModem.waitForNetwork(60000L)) {
    Logger::warn("Failed to connect to network");
    while (true); // Halt if network connection fails
  }

  // Connect to GPRS
  Logger::info("Connecting to GPRS...");
  if (!gsmModem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
    Logger::warn("Failed to connect to GPRS");
    while (true); // Halt if GPRS connection fails
  }

  // Set MQTT server
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

    // Attempt to connect to MQTT server
  Logger::info("Connecting to MQTT server: ");


  int attempts = 0;
  const int maxAttempts = 5;

  while (!mqttClient.connected() && attempts < maxAttempts) {
    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Logger::info("Connected to MQTT server");
    } else {
      Logger::warn("MQTT connection failed, rc=%d Retrying in 5 seconds...", mqttClient.state());
      delay(5000);
      attempts++;
    }
  }

  if (!mqttClient.connected()) {
    Logger::warn("Failed to connect to MQTT after max attempts");

    // Attempt to reconnect GPRS
    if (!gsmModem.isGprsConnected()) {
      Logger::warn("GPRS disconnected, attempting to reconnect...");
      if (!gsmModem.gprsConnect(APN, GPRS_USER, GPRS_PASS)) {
        Logger::warn("GPRS reconnection failed, restarting modem...");
        gsmModem.restart();
      }
    }

    while(true);

}
    lastSendTime = millis();
    lastConnectAttempt = lastSendTime;
    lastSuccessfullConnectionAttempt = lastConnectAttempt;
    
    Logger::debug("gprs setup complete");

}

void MqttClient::update(unsigned long now) {

    if((now - lastGprsUpdate) < MODEM_UPDATE_INTERVAL) return;
    lastGprsUpdate = now;
    mqttClient.loop();

    if (now - lastSendTime < MQTT_SEND_INTERVALS[stablityState]) return;

    bool isMqttConnected = checkConnection(now);

    VehicleStatus data = sensorManager.getVehicleStatus();

    if (!isMqttConnected){
        sendSms(data);
        adjustStablityState(false);
        return;
    }

    sendMqttMessage(data);
    if (MQTT_ENABLE_SMS[stablityState]) sendSms(data);

    lastSendTime = now;
}

void MqttClient::sendMqttMessage(const VehicleStatus& data) {
    
     Logger::info("%4d/%2d/%2d %2d:%2d:%d",
        data.time.year,
        data.time.month,
        data.time.day,
        data.time.hour,
        data.time.minute,
        data.time.second
    );
    Logger::vector("acceleration", data.acceleration);
    Logger::vector("angularVelocity", data.angularVelocity);
    Logger::vector("orientation", data.orientation);
    Logger::vector("velocity", data.velocity);
    Logger::vector("location", data.location);
    Logger::info("isLocationDeadReckoned: %s", data.isLocationDeadReckoned ? "true" : "false");
    Logger::info("locationFreshness: %lu", data.locationFreshness);
    Logger::info("signalStrength: %d", data.signalStrength);
    Logger::info("batterydata: %d", data.batteryStatus);

    // Buffer to hold serialized data (61 bytes)
    uint8_t buffer[74];
    uint8_t offset = 0;

    // Serialize Datetime (6 bytes)
    buffer[offset++] = data.time.second;
    buffer[offset++] = data.time.minute;
    buffer[offset++] = data.time.hour;
    buffer[offset++] = data.time.day;
    buffer[offset++] = data.time.month;
    // Split int16_t year into two bytes (little-endian)
    buffer[offset++] = (uint8_t)(data.time.year & 0xFF);
    buffer[offset++] = (uint8_t)(data.time.year >> 8);

    // Serialize Vector fields (4 bytes per float, 12 bytes per Vector)
    memcpy(&buffer[offset], &data.acceleration.x, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.acceleration.y, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.acceleration.z, sizeof(float));
    offset += sizeof(float);

    memcpy(&buffer[offset], &data.velocity.x, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.velocity.y, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.velocity.z, sizeof(float));
    offset += sizeof(float);

    memcpy(&buffer[offset], &data.angularVelocity.x, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.angularVelocity.y, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.angularVelocity.z, sizeof(float));
    offset += sizeof(float);

    memcpy(&buffer[offset], &data.orientation.x, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.orientation.y, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.orientation.z, sizeof(float));
    offset += sizeof(float);

    memcpy(&buffer[offset], &data.location.x, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.location.y, sizeof(float));
    offset += sizeof(float);
    memcpy(&buffer[offset], &data.location.z, sizeof(float));
    offset += sizeof(float);

    // Serialize remaining fields
    buffer[offset++] = (uint8_t)data.isLocationDeadReckoned;
    // Split unsigned long locationFreshness into four bytes (little-endian)
    buffer[offset++] = (uint8_t)(data.locationFreshness & 0xFF);
    buffer[offset++] = (uint8_t)((data.locationFreshness >> 8) & 0xFF);
    buffer[offset++] = (uint8_t)((data.locationFreshness >> 16) & 0xFF);
    buffer[offset++] = (uint8_t)((data.locationFreshness >> 24) & 0xFF);
    buffer[offset++] = (uint8_t)data.signalStrength;
    buffer[offset++] = (uint8_t)data.batteryStatus;

    
    // Publish with QoS 1
    bool ack = mqttClient.publish(MQTT_TOPIC, buffer, sizeof(buffer));
    if (ack) {
        Logger::info("Message sent");
    } else {
        Logger::warn("Failed to send message");
    }
    adjustStablityState(ack);
    Logger::info("************************************************");

}

void MqttClient::sendSms(const VehicleStatus& data) {

    char lat[12], lon[12], speed[8], acc[8], bat[8];

    // Format float values manually
    dtostrf(data.location.x, 5, 2, lat);
    dtostrf(data.location.y, 5, 2, lon);

    float speedVal = sqrt(
        data.velocity.x * data.velocity.x +
        data.velocity.y * data.velocity.y +
        data.velocity.z * data.velocity.z);
    dtostrf(speedVal, 4, 1, speed);

    float accVal = sqrt(
        data.acceleration.x * data.acceleration.x +
        data.acceleration.y * data.acceleration.y +
        data.acceleration.z * data.acceleration.z);
    dtostrf(accVal, 4, 1, acc);

    // Battery
    if (data.batteryStatus < 0) {
        strcpy(bat, "ADAPTER");
    } else {
        snprintf(bat, sizeof(bat), "%d%%", data.batteryStatus);
    }

    char sms[160];
    snprintf(sms, sizeof(sms),
        "[%02d:%02d:%02d %02d/%02d/%04d] "
        "Lat:%s Lon:%s Spd:%sm/s Acc:%s "
        "Sig:%d%% Bat:%s",
        data.time.hour, data.time.minute, data.time.second,
        data.time.day, data.time.month, data.time.year,
        lat, lon, speed, acc,
        data.signalStrength, bat);


    gsmModem.sendSMS(EMERGENCY_PHONE_NUMBER, sms);
    Logger::info("SMS sent: %s", sms);
}

void MqttClient::adjustStablityState(bool success) {
    if (success) {
        missCount = 0;
        if (++successCount >= 20 && stablityState > 0) {
            stablityState--;
            successCount = 0;
            Logger::info("downgrading stability status");
        }
    } else {
        successCount = 0;
        if (++missCount >= 3 && stablityState < MQTT_WORST_STABILITY_STATUS) {
            stablityState++;
            missCount = 0;
            Logger::info("upgrading stability status");
        }
    }
}




bool MqttClient::checkConnection(unsigned long now) {

    if(mqttClient.connected()){
        return true;
    }

    if((now - lastConnectAttempt) < MQTT_RECONNECTION_INTERVAL)
        return false;

    lastConnectAttempt = now;

    if((now - lastSuccessfullConnectionAttempt) >= NETWORK_RECONNECTION_INTERVAL){
        
        if (!gsmModem.isNetworkConnected()) {
            Logger::warn("network disconnected");

            if (!gsmModem.waitForNetwork(15000L, true) || !gsmModem.isNetworkConnected()) {
                return false;
            }
        }

        if(!gsmModem.isGprsConnected() || !gsmModem.gprsConnect(APN, GPRS_USER, GPRS_PASS)){
            return false;
        }

    }

    if (mqttClient.connect(MQTT_CLIENT_ID)) {
      Logger::info("Connected to MQTT server");
      lastSuccessfullConnectionAttempt = now;
      return true;
    } else {
      Logger::warn("MQTT connection failed, rc=%d Retrying in 5 seconds...", mqttClient.state());
    }

    return false;
}
