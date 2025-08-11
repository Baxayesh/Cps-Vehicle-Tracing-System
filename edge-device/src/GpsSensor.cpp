#include "GpsSensor.h"
#include <Arduino.h>
#include "utilities.h"

const unsigned int SIM808_RESPONSE_TIMEOUT = 5000;

void cleanResonse(char *str, int len) {
    
    // Step 1: Remove trailing "OK\r\n" if present
    str[len - 4] = '\0';
    
    // Step 2: Remove all occurrences of "\r\n"
    char *src = str, *dst = str;
    while (*src) {
        if (*src == '\r' && *(src + 1) == '\n') {
            src += 2; // skip "\r\n"
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

GpsSensor::GpsSensor(SoftwareSerial& sim808Serial): sim808Serial(sim808Serial) {
    // Initialize gpsData
    gpsData = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0, 0};
}

bool GpsSensor::setup() {

    return sendControllCommand("AT") &&
        sendControllCommand("AT+CFUN=1") &&
        sendControllCommand("AT+CGNSPWR=1") &&
        sendControllCommand("AT+CGPSRST=1") &&
        sendControllCommand("AT+CLTS=1");

}

Datetime GpsSensor::getDatetime() {
    Datetime dt = {0, 0, 0, 0, 0, 0
    }; // Default invalid

    char response[64];
    if (!sendDataCommand("AT+CCLK?", response, sizeof(response))) {
        return dt;
    }

    char* start = strstr(response, "+CCLK: \"");
    if (!start) return dt;
    start += 8; // Move past "+CCLK: \""

    // Expected format: "yy/MM/dd,hh:mm:ss±zz"
    int yy, MM, dd, hh, mm, ss;
    if (sscanf(start, "%2d/%2d/%2d,%2d:%2d:%2d", &yy, &MM, &dd, &hh, &mm, &ss) == 6) {
        dt.year = 2000 + yy;
        dt.month = MM;
        dt.day = dd;
        dt.hour = hh;
        dt.minute = mm;
        dt.second = ss;
    }

    return dt;
}


bool GpsSensor::updateGps() {

    char response[256];
    if (!sendDataCommand("AT+CGNSINF", response, sizeof(response), true)) {
        return false;
    }

    char* start = strstr(response, "+CGNSINF:");
    if (!start) return false;
    start += 9;

    GpsData tempData = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, millis(), 0};

    char* token = strtok(start, ",");
    int field = 0;

    while (token != nullptr && field <= 14) {
        switch (field) {
            case 0: // GNSS run status (1 = running)
                break;
            case 1: // Fix status: 1 = fix, 0 = no fix
                if(atoi(token) != 1){
                    return false;
                }
                break;
            case 3: tempData.latitude = atof(token); break;
            case 4: tempData.longitude = atof(token); break;
            case 5: tempData.altitude = atof(token); break;
            case 6: tempData.speed = atof(token) / 3.6f; break;  // km/h to m/s
            case 7: tempData.heading = atof(token); break;
            case 13: tempData.satellites = atoi(token); break;
        }
        token = strtok(nullptr, ",");
        field++;
    }

    gpsData = tempData;
    return true;
}

int8_t GpsSensor::getSignalStrength() {
    char response[64];

    if(!sendDataCommand("AT+CSQ", response, sizeof(response))){
        return -100;
    }
    
    char* start = strstr(response, "+CSQ:");
    if (!start) return -101;
    
    start += 6;
    char* token = strtok(start, ",");
    if (!token) return -102;
    
    int signal = atoi(token);
    if (signal == 99) return 0; // No signal
    return (signal * 100) / 31; // Convert to percentage
}

int8_t GpsSensor::getBatteryStatus() {
    char response[64];

    if(!sendDataCommand("AT+CBC", response, sizeof(response))){
        return -100;
    }

    char* start = strstr(response, "+CBC:");
    if (!start) return -101;
    
    start += 6;
    char* token = strtok(start, ",");
    if (!token) return -102;
    
    int bcs = atoi(token); // Battery connection status
    token = strtok(nullptr, ",");
    if (!token) return -103;
    
    int bcl = atoi(token); // Battery charge level
    return (bcs == 0) ? -1 : bcl;
}


bool GpsSensor::sendDataCommand(const char* command, char* response, size_t responseSize, bool verbose = false){
    
    if (response && responseSize > 0) {
        memset(response, 0, responseSize);
    }

    sim808Serial.println(command);
    
    unsigned long start = millis();
    size_t index = 0;
    char c;

    while (millis() - start < SIM808_RESPONSE_TIMEOUT) {
        while (sim808Serial.available()) {

            c = sim808Serial.read();

            if (index < responseSize - 1) {
                response[index++] = c;
            }
            
            if (c == '\n') {
                if(index >=4 && strcmp(response + index - 4, "OK\r\n") == 0){
                    cleanResonse(response, index);

                    return true; 
                }
                if(index >=7 && strcmp(response + index - 7, "ERROR\r\n") == 0){

                    if(verbose)
                        Logger::warn(response);

                    return false;
                }
            }
        }
        
    }

    if(verbose) {
        Logger::warn("invalid message");
        Logger::info(response);
        Logger::warn("invalid message end");
    }

    return false;
}

bool GpsSensor::sendControllCommand(const char* command){
    
    char response[8];
    memset(response, 0, 8);
    sim808Serial.println(command);
    
    unsigned long start = millis();
    size_t index = 0;
    
    while (millis() - start < SIM808_RESPONSE_TIMEOUT) {
        while (sim808Serial.available()) {

            char c = sim808Serial.read();
        
            if (index < 8) {
                response[index++] = c;
            }
            
            if(c == '\n'){
                if(index < 3){
                    memset(response, 0, 8);
                    index = 0;
                } else {

                    return strcmp(response, "OK\r\n") == 0;
                }
                
            }
        }
    }

    return false;
}
