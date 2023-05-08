#include "loggerESP.h"

LoggerESP::LoggerESP() {
    Serial.begin(115200);
}

void LoggerESP::info(const char *file, int line, const char *func, const char *format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);
    buf = new char[bufSize];
    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    formatBuf(buf);
    printLog(BLUE, "INFO", file, line, func, buf);
}

void LoggerESP::warring(const char *file, int line, const char *func, const char *format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);
    buf = new char[bufSize];
    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    formatBuf(buf);
    printLog(YELLOW, "WARRING", file, line, func, buf);
    delete[] buf;
}

void LoggerESP::error(const char *file, int line, const char *func, const char *format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);
    buf = new char[bufSize];
    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    formatBuf(buf);
    printLog(RED, "ERROR", file, line, func, buf);
    delete[] buf;
}

char *LoggerESP::formatBuf(char *(&buf)) {
    int len = strlen(buf);
    int addSize = 0;
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            addSize += 3;
        }
    }
    addSize += 1;
    char *tmp = new char[len + addSize];
    int tmp_len = len + addSize;
    strcpy(tmp, buf);
    for (int i = 0; i < tmp_len; i++) {
        if (tmp[i] == '\n') {
            tmp[i + 1] = '|';
            tmp[i + 2] = ' ';
            tmp[i + 3] = ' ';
            i += 3;
            tmp[tmp_len] = '\0';
        }
    }
    delete[] buf;
    buf = tmp;
    return buf;
}

void LoggerESP::printLog(const String &stytle, const String &level, const String &file, const uint16_t &line, const String &func, char *(&buf)) {
    String loggerMsg = stytle + "=======================================================================\n"
                                "|  #" +
                       level + "\n"
                               "|  [" +
                       file + ":" + line + "]\n"
                                           "|  [Function:" +
                       func + "]\n"
                              "|----------------------------------------------------------------------\n"
                              "|\n"
                              "|  " +
                       buf + "\n"
                             "|\n"
                             "=======================================================================\033[0m\n";

    Serial.println(loggerMsg);
}