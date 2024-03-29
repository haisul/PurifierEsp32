#ifndef __LOGGER_ESP_
#define __LOGGER_ESP_
#include <Arduino.h>

#define i(format, ...) info(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define w(format, ...) warring(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define e(format, ...) error(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define iL(format, ...) info_lite(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define wL(format, ...) warring_lite(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define eL(format, ...) error_lite(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define NORMAL "\n\x1B[0m"
#define INFO "\n\033[1m\x1B[34m"
#define WARRING "\n\033[33m"
#define ERROR "\n\033[1m\x1B[31m"

class LoggerESP {
public:
    LoggerESP();
    void info(const char *file, int line, const char *func, const char *format, ...);
    void warring(const char *file, int line, const char *func, const char *format, ...);
    void error(const char *file, int line, const char *func, const char *format, ...);

    void info_lite(const char *file, int line, const char *func, const char *format, ...);
    void warring_lite(const char *file, int line, const char *func, const char *format, ...);
    void error_lite(const char *file, int line, const char *func, const char *format, ...);

private:
    String formatBuf(char *buf);
    void printLog(const String &stytle, const String &level, const String &file, const uint16_t &line, const String &func, const String &buf);
    void printLogLite(const String &stytle, const String &level, const String &file, const uint16_t &line, const String &func, const String &buf);
};

extern LoggerESP logger;

#endif