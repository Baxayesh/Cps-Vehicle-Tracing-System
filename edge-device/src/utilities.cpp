#include "utilities.h"
#include <Arduino.h>
#include <stdarg.h>

static char logBuffer[LOG_BUFFER_SIZE];

void Logger::setup(){
    #if LOG_LEVEL > LOG_LEVEL_NONE
        Serial.begin(LOG_SERIAL_BUS_FREQUENCY);
    #endif
}

void Logger::debug(const char* fmt, ...) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    va_list args;
    va_start(args, fmt);
    vsnprintf(logBuffer, LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    printTimestamp();
    Serial.print("[DEBUG] ");
    Serial.println(logBuffer);
#endif
}

void Logger::info(const char* fmt, ...) {
#if LOG_LEVEL >= LOG_LEVEL_INFO
    va_list args;
    va_start(args, fmt);
    vsnprintf(logBuffer, LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    printTimestamp();
    Serial.print("[INFO]  ");
    Serial.println(logBuffer);
#endif
}

void Logger::warn(const char* fmt, ...) {
#if LOG_LEVEL >= LOG_LEVEL_WARN
    va_list args;
    va_start(args, fmt);
    vsnprintf(logBuffer, LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    printTimestamp();
    Serial.print("[WARN]  ");
    Serial.println(logBuffer);
#endif
}

void Logger::vector(const char* name, Vector vector){
#if LOG_LEVEL >= LOG_LEVEL_INFO
    printTimestamp();
    Serial.print("[INFO]  ");
    
    Serial.print(name);
    Serial.print(" : (");

    Serial.print(vector.x, 5);
    Serial.print(", ");
    Serial.print(vector.y);
    Serial.print(", ");
    Serial.print(vector.z);
    Serial.println(")");
    
#endif
}


void Logger::printTimestamp() {
    Serial.print('[');
    Serial.print(millis());
    Serial.print(" ms] ");
}