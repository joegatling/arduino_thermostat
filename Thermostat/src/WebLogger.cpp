#include "WebLogger.h"

#include <Print.h>


WebLogger* WebLogger::_instance = 0;

WebLogger& WebLogger::getInstance()
{
    if (_instance == 0)
    {
        _instance = new WebLogger();
    }
    return *_instance;
}

void WebLogger::begin()
{
    if (_instance == 0)
    {
        _instance = new WebLogger();
        _instance->_hasBegun = true;
    }

}

WebLogger::WebLogger():
_server(80),
_events("/events"),
_logIndex(0),
_hasBegun(false)
{
    // Initialize log buffer
    for (int i = 0; i < LOG_BUFFER_SIZE; ++i)
    {
        _logBuffer[i] = "";
    }

    _events.onConnect([this](AsyncEventSourceClient *client)
    {
        //send all the existing logs to the newly connected client
        for (int i = 0; i < LOG_BUFFER_SIZE; ++i)
        {
            int index = (_logIndex + i + 1) % LOG_BUFFER_SIZE;
            if (_instance->_logBuffer[index].length() > 0)
            {
                client->send(_instance->_logBuffer[index].c_str());
                client->send("<br/>");
            }
        }   
    });    

    // Start web server
_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", R"rawliteral(
        <!DOCTYPE html>
        <html>
        <head>
        <title>Thermostat Logs</title>
        <style>
        body { font-family: monospace; background:#222; color:#ffd600; margin:0; padding:10px; }
        pre { white-space: pre-wrap; }
        </style>
        </head>
        <body>
        <h2>Thermostat Logs</h2>
        <pre id="log"></pre>
        <script>
        const log = document.getElementById('log');
        const evtSrc = new EventSource('/events');
        evtSrc.onmessage = e => {
        log.innerHTML += e.data;
        window.scrollTo(0, document.body.scrollHeight);
        };
        </script>
        </body>
        </html>
        )rawliteral");
            });

    _server.addHandler(&_events);
    _server.begin();
}

WebLogger::~WebLogger()
{
    // Cleanup
    delete _instance;
    _instance = 0;
}


size_t WebLogger::print(const String &s)
{
    if (!_hasBegun)
    {
        return 0;
    }

    Serial.print(s);
    
    _events.send(s.c_str());

    _logBuffer[_logIndex] += s;
    return s.length();
}

size_t WebLogger::print(const char s[])
{
    String tmp(s);
    return print(tmp);
}

size_t WebLogger::print(char c)
{
    String tmp(c);
    return print(tmp);
}

size_t WebLogger::print(unsigned char num, int base)
{
    String tmp = String(num, base);
    return print(tmp);
}

size_t WebLogger::print(int num, int base)
{
    String tmp = String(num, base);
    return print(tmp);
}

size_t WebLogger::print(unsigned int num, int base)
{
    String tmp = String(num, base);
    return print(tmp);
}

size_t WebLogger::print(long num, int base)
{
    String tmp = String(num, base);
    return print(tmp);
}

size_t WebLogger::print(unsigned long num, int base)
{
    String tmp = String(num, base);
    return print(tmp);
}

size_t WebLogger::print(long long num, int base)
{
    // String on Arduino/ESP usually supports long long; fall back to snprintf if necessary
    String tmp = String(num, base);
    return print(tmp);
}

size_t WebLogger::print(unsigned long long num, int base)
{
    String tmp = String((unsigned long long)num, base);
    return print(tmp);
}

size_t WebLogger::print(double num, int digits)
{
    String tmp = String(num, digits);
    return print(tmp);
}

size_t WebLogger::print(const Printable& printable)
{
    // Collect printable output into a temporary string
    struct Collector : public Print {
        String &out;
        Collector(String &s) : out(s) {}
        size_t write(uint8_t c) override { out += (char)c; return 1; }
        size_t write(const uint8_t *buffer, size_t size) override { out.concat((const char*)buffer, size); return size; }
    };

    String tmp;
    Collector c(tmp);
    printable.printTo(c);
    return print(tmp);
}

size_t WebLogger::print(struct tm * timeinfo, const char * format)
{
    char buf[128];
    if (format) {
        // try strftime; if unavailable, fall back to manual
        #if defined(ARDUINO) && !defined(ESP8266)
        if (strftime(buf, sizeof(buf), format, timeinfo) > 0) {
            return print(String(buf));
        }
        #endif
        // fallback: try to snprintf using common tokens
    }
    // default format: YYYY-MM-DD HH:MM:SS
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900,
             timeinfo->tm_mon + 1,
             timeinfo->tm_mday,
             timeinfo->tm_hour,
             timeinfo->tm_min,
             timeinfo->tm_sec);
    return print(String(buf));
}

/* println implementations */

size_t WebLogger::println(const String &s)
{
    print(s);
    return println();
}

size_t WebLogger::println(const char s[])
{
    print(s);
    return println();
}

size_t WebLogger::println(char c)
{
    print(c);
    return println();
}

size_t WebLogger::println(unsigned char num, int base)
{
    print(num, base);
    return println();
}

size_t WebLogger::println(int num, int base)
{
    print(num, base);
    return println();
}

size_t WebLogger::println(unsigned int num, int base)
{
    print(num, base);
    return println();
}

size_t WebLogger::println(long num, int base)
{
    print(num, base);
    return println();
}

size_t WebLogger::println(unsigned long num, int base)
{
    print(num, base);
    return println();
}

size_t WebLogger::println(long long num, int base)
{
    print(num, base);
    return println();
}

size_t WebLogger::println(unsigned long long num, int base)
{
    print(num, base);
    return println();
}

size_t WebLogger::println(double num, int digits)
{
    print(num, digits);
    return println();
}

size_t WebLogger::println(const Printable& printable)
{
    print(printable);
    return println();
}

size_t WebLogger::println(struct tm * timeinfo, const char * format)
{
    print(timeinfo, format);
    return println();
}

size_t WebLogger::println(void)
{
    if (!_hasBegun)
    {
        return 0;
    }
    
    Serial.println();    
    
    // finalize current line, send to SSE clients and advance ring buffer index
    _logBuffer[_logIndex] += '\n';
    size_t len = _logBuffer[_logIndex].length();

    // advance to next buffer slot (preserve history in the previous slot)
    _logIndex = (_logIndex + 1) % LOG_BUFFER_SIZE;
    // prepare next slot for new content
    _logBuffer[_logIndex] = "";

    _events.send("<br/>");


    return len;
}