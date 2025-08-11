#ifndef __UTILITIES_H__
    #define __UTILITIES_H__

#include "config.h"
#include "dataStructures.h"

class Logger {
public:
    static void setup();
    static void debug(const char* fmt, ...);
    static void info(const char* fmt, ...);
    static void warn(const char* fmt, ...);
    static void vector(const char* name, Vector vector);
private:
    static void printTimestamp();
};

#endif