#pragma once

#include "SimpleButton.h"

#include "Thermostat.h"

#define UP_BUTTON_PIN     A2
#define DOWN_BUTTON_PIN   A1
#define POWER_BUTTON_PIN  A0

class ButtonController
{
public:
    ButtonController();
    ~ButtonController();    
    void setThermostat(Thermostat* thermostat);
    void update();

    void setConfigModeCallback(std::function<void()> callback);

private:
    static ButtonController* s_instance;
    static void upButtonEndPressCallback();
    static void downButtonEndPressCallback();
    static void powerButtonEndPressCallback();

    static void upButtonHoldCallback();
    static void downButtonHoldCallback();
    static void powerButtonHoldCallback();

    void onUpButtonEndPress();
    void onDownButtonEndPress();
    void onPowerButtonEndPress();

    void onUpButtonHold();
    void onDownButtonHold();
    void onPowerButtonHold();

    SimpleButton upButton;
    SimpleButton downButton;
    SimpleButton powerButton;    

    Thermostat* thermostat;
    std::function<void()> configModeCallback;

    
};