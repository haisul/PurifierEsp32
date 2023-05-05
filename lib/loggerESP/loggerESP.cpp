#include "loggerESP.h"

Logger::Logger() {
    Serial.begin(115200);
}

void Logger::info(const char *file, int line, const char *func, const char *format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 5; // 取得 buf 的大小
    va_end(args);
    buf = new char[bufSize]; // 動態配置 buf 的大小
    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    formatBuf(buf);
    printLog(BLUE, "INFO", file, line, func, buf);
}

void Logger::warring(const char *file, int line, const char *func, const char *format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 5; // 取得 buf 的大小
    va_end(args);
    buf = new char[bufSize]; // 動態配置 buf 的大小
    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    formatBuf(buf);
    printLog(YELLOW, "WARRING", file, line, func, buf);
    delete[] buf;
}

void Logger::error(const char *file, int line, const char *func, const char *format, ...) {
    char *buf;
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 5; // 取得 buf 的大小
    va_end(args);
    buf = new char[bufSize]; // 動態配置 buf 的大小
    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    formatBuf(buf);
    printLog(RED, "ERROR", file, line, func, buf);
    delete[] buf;
}

void Logger::formatBuf(char *(&buf)) {
    int len = strlen(buf);
    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            memmove(buf + i + 3, buf + i, len - i);
            buf[i + 1] = '|';
            buf[i + 2] = ' ';
            buf[i + 3] = ' ';
            len += 2;
            i += 3;
            buf[len] = '\0';
        }
    }
}

void Logger::printLog(const String &stytle, const String &level, const String &file, const uint16_t &line, const String &func, char *(&buf)) {
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
    /*Serial.print(stytle);
    Serial.println("=======================================================================");
    Serial.println("|  #" + level);
    Serial.printf("|  [%s:%d]\n", file, line);
    Serial.printf("|  [Function:%s]\n", func);
    Serial.println("|----------------------------------------------------------------------");
    Serial.println("|");
    Serial.printf("|  %s\n", buf);
    Serial.println("|");
    Serial.println("=======================================================================\033[0m\n");*/
}