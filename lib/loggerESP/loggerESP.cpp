#include "loggerESP.h"
LoggerESP logger;
LoggerESP::LoggerESP() {
    Serial.begin(115200);
}

void LoggerESP::info(const char *file, int line, const char *func, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    char buf[bufSize];

    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    printLog(INFO, "INFO", file, line, func, formatBuf(buf));
}

void LoggerESP::warring(const char *file, int line, const char *func, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    char buf[bufSize];

    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    printLog(WARRING, "WARRING", file, line, func, formatBuf(buf));
}

void LoggerESP::error(const char *file, int line, const char *func, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int bufSize = vsnprintf(NULL, 0, format, args) + 1;
    va_end(args);

    char buf[bufSize];

    va_start(args, format);
    vsnprintf(buf, bufSize, format, args);
    va_end(args);

    printLog(ERROR, "ERROR", file, line, func, formatBuf(buf));
}

String LoggerESP::formatBuf(char *buf) {
    const int MAX_LINE_LENGTH = 100;
    int len = strlen(buf);
    int addSize = len % MAX_LINE_LENGTH;

    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            addSize += 3;
        }
        if (i > 0 && i % MAX_LINE_LENGTH == 0) {
            addSize++;
        }
    }
    addSize += 1;
    char tmp[len + addSize];
    int tmp_len = len + addSize;
    int lineCount = 0; // 紀錄當前行的字元數

    strcpy(tmp, buf);
    for (int i = 0; i < tmp_len; i++) {
        lineCount++;
        if (lineCount >= MAX_LINE_LENGTH) {
            memmove(tmp + i + 2, tmp + i + 1, tmp_len - i - 2);
            tmp[i + 1] = '\n';
            i++;
            lineCount = 0;
        }
        if (tmp[i] == '\n') {
            memmove(tmp + i + 4, tmp + i + 1, tmp_len - i - 4);
            tmp[i + 1] = '|';
            tmp[i + 2] = ' ';
            tmp[i + 3] = ' ';
            i += 4;

            lineCount = 0;
        }
    }
    tmp[tmp_len] = '\0';
    String tmpStr(tmp);
    return tmpStr;
}

void LoggerESP::printLog(const String &stytle, const String &level, const String &file, const uint16_t &line, const String &func, const String &buf) {
    String line1 = "=", line2;
    for (int i = 0; i < 109; i++) {
        line1 += "=";
        line2 += "-";
    }
    String loggerMsg = stytle + line1 + "\n"
                                        "|  #" +
                       level + "\n"
                               "|  [" +
                       file + ":" + line + "]\n"
                                           "|  [Function:" +
                       func + "]\n"
                              "|" +
                       line2 + "\n"
                               "|\n"
                               "|  " +
                       buf + "\n"
                             "|\n" +
                       line1 + "\033[0m\n";

    Serial.println(loggerMsg);
}