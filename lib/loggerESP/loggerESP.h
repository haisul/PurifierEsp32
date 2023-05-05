#ifndef __LOGGER_ESP_
#define __LOGGER_ESP_
#include <Arduino.h>

#define i(format, ...) info(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define w(format, ...) warring(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)
#define e(format, ...) error(__FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define WHITE "\n\x1B[0m"
#define BLUE "\n\033[1m\x1B[34m"
#define YELLOW "\n\033[1m\x1B[33m"
#define RED "\n\033[1m\x1B[31m"

class Logger {
public:
    Logger();
    void info(const char *file, int line, const char *func, const char *format, ...);
    void warring(const char *file, int line, const char *func, const char *format, ...);
    void error(const char *file, int line, const char *func, const char *format, ...);

private:
    void formatBuf(char *(&buf));
    void printLog(const String &stytle, const String &level, const String &file, const uint16_t &line, const String &func, char *(&buf));
};

#endif