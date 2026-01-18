#pragma once
#include "Thermostat.h"

#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_NeoPixel.h>

#include "ThermostatFont.h"

#define DISPLAY_SCALE       1
#define DISPLAY_ROTATION    3

#define STATUS_MESSAGE_DURATION       6000
#define STATUS_MESSAGE_SCROLL_DELAY   500
#define STATUS_MESSAGE_SCROLL_STEP    100

#define TARGET_TEMP_DURATION          3000
#define POWER_ON_MSG_DURATION         1000

#define CURRENT_TEMP_BRIGHTNESS    1
#define TARGET_TEMP_BRIGHTNESS     2
#define MESSAGE_BRIGHTNESS         5 


class LedController
{
public:
    LedController();
    ~LedController();   
    
    void setThermostat(Thermostat* thermostat);
    void update();  

    void showStatusMessage(const String& message);
    void showProgressBar(float progress);

private:
    void updateMatrix();
    void updateNeoPixel();

    bool shouldShowStatusMessage();
    unsigned long getStatusMessageTime();
    void drawStatusMessage();

    bool shouldShowTargetTemperature();
    void drawTargetTemperature();

    void drawCurrentTemperature();

    Thermostat* thermostat;

    Adafruit_8x16matrix matrix;
    Adafruit_NeoPixel pixel;

    float currentTemperature = 0.0f;
    float targetTemperature = 0.0f;
    bool useFahrenheit = false;

    unsigned long lastTargetTemperatureTime = 0;
    unsigned long lastCurrentTemperatureTime = 0;
    unsigned long lastHeaterModeChangeTime = 0;
    unsigned long lastStatusMessageTime = 0;

    ThermostatMode previousMode = OFF;
    ThermostatMode currentMode = OFF;

    String statusMessage;

    uint16_t statusMessageWidth, statusMessageHeight;
};