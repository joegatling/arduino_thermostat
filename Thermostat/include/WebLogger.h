#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define LOG_BUFFER_SIZE 100

class WebLogger
{
public:
    static WebLogger& getInstance();
    static void begin();

    void loop();

    size_t print(const __FlashStringHelper *ifsh) { return print(reinterpret_cast<const char *>(ifsh)); }
    size_t print(const String &);
    size_t print(const char[]);
    size_t print(char);
    size_t print(unsigned char, int = DEC);
    size_t print(int, int = DEC);
    size_t print(unsigned int, int = DEC);
    size_t print(long, int = DEC);
    size_t print(unsigned long, int = DEC);
    size_t print(long long, int = DEC);
    size_t print(unsigned long long, int = DEC);
    size_t print(double, int = 2);
    size_t print(const Printable&);
    size_t print(struct tm * timeinfo, const char * format = NULL);

    size_t println(const __FlashStringHelper *ifsh) { return println(reinterpret_cast<const char *>(ifsh)); }
    size_t println(const String &s);
    size_t println(const char[]);
    size_t println(char);
    size_t println(unsigned char, int = DEC);
    size_t println(int, int = DEC);
    size_t println(unsigned int, int = DEC);
    size_t println(long, int = DEC);
    size_t println(unsigned long, int = DEC);
    size_t println(long long, int = DEC);
    size_t println(unsigned long long, int = DEC);
    size_t println(double, int = 2);
    size_t println(const Printable&);
    size_t println(struct tm * timeinfo, const char * format = NULL);
    size_t println(void);  

private:

    WebLogger();
    ~WebLogger();

    static WebLogger* _instance;
    
    AsyncWebServer _server;
    AsyncEventSource _events;

    bool _hasBegun = false;

    String _logBuffer[LOG_BUFFER_SIZE];
    int _logIndex = 0;
};
