#pragma once

#include <Arduino.h>
#include <AutoPID.h>
#include <OneWire.h>
#include <Wire.h>
#include <DallasTemperature.h>

#include "EventEmitter.h"

#define THERMOSTAT_KP 0.8
#define THERMOSTAT_KI 0
#define THERMOSTAT_KD 0

#define F_TO_C(t) ((t - 32) * 5.0f / 9.0f)
#define C_TO_F(t) ((t * 9.0f / 5.0f) + 32)
#define C_TO_F_DELTA(t) ((t * 9.0f / 5.0f))

enum ThermostatMode
{
    OFF,
    HEAT
};

enum ThermostatPreset
{
    NONE,
    ECO,
    BOOST,
    SLEEP
};

struct TimeDelayBoolean
{
    public:
        unsigned long minToggleTime;


    TimeDelayBoolean(unsigned long minToggleTimeMillis, bool initialState = false) :
        minToggleTime(minToggleTimeMillis),
        currentState(initialState),
        targetState(initialState),
        lastToggleTime(0)
    {
    }

    bool getValue()
    {
        update();
        return currentState;
    }

    void setValue(bool newState, bool ignoreMinToggleTime = false)
    {        
        targetState = newState;

        if(ignoreMinToggleTime)
        {
            currentState = targetState;
            lastToggleTime = millis();
        }
        else
        {
            update();
        }
    }

    String toString()
    {
        update();
        String result = currentState ? "true" : "false";

        if(currentState != targetState)
        {
            result += " (target: ";
            result += targetState ? "true" : "false";
            result += ", ";
            result += String(millis() - lastToggleTime);
            result += "/";
            result += String(minToggleTime);
            result += ")";
        }
        return result;
    }

    private:
        bool currentState;
        bool targetState;
        
        unsigned long lastToggleTime;    

        void update()
        {
            if(currentState == targetState)
            {
                return;
            }

            if((millis() - lastToggleTime >= minToggleTime) || (lastToggleTime == 0))
            {
                currentState = targetState;
                lastToggleTime = millis();
            }
        }
};

class Thermostat
{
public:

    Thermostat();
    ~Thermostat();

    void setTargetTemperature(float temperature, bool forceCelsius = false);
    float getTargetTemperature(bool forceCelsius = false);

    float getCurrentTemperature(bool forceCelsius = false);

    void setMode(ThermostatMode mode);
    ThermostatMode getMode();

    void setUsingFahrenheit(bool shouldUseFahrenheit);
    
    bool isUsingFahrenheit();
    bool isUsingCelsius();

    bool getHeaterPowerState() { return heaterState.getValue(); }
    
    void setPreset(ThermostatPreset preset);
    ThermostatPreset getPreset();

    void update();

    size_t onTargetTemperatureChanged(std::function<void(float)> callback);
    size_t onCurrentTemperatureChanged(std::function<void(float)> callback);
    size_t onModeChanged(std::function<void(ThermostatMode)> callback);
    size_t onUseFahrenheitChanged(std::function<void(bool)> callback);
    size_t onHeaterPowerChanged(std::function<void(bool)> callback);
    size_t onPresetChanged(std::function<void(ThermostatPreset)> callback);

private:  

    void updateCurrentTemperature();
    void updateHeater();

    AutoPIDRelay heaterPID;
    OneWire oneWire;
    DallasTemperature sensors;

    EventEmitter<float> onTargetTemperatureChangedEvent;
    EventEmitter<float> onCurrentTemperatureChangedEvent;
    EventEmitter<ThermostatMode> onModeChangedEvent;
    EventEmitter<bool> onUseFahrenheitChangedEvent;
    EventEmitter<bool> onHeaterPowerChangedEvent;
    EventEmitter<ThermostatPreset> onPresetChangedEvent;

    double currentTemperature = 20.0f;
    double targetTemperature = 20.0f;
    double heaterTargetTemperature = 20.0f;

    ThermostatMode currentMode = OFF;
    ThermostatPreset currentPreset = NONE;

    TimeDelayBoolean heaterState;
    bool previousHeaterState = false;
    bool pidState = false;

    unsigned long lastTemperatureUpdateTime = 0;
    unsigned long lastHeaterToggleTime = 0;
    unsigned long lastPresetChangeTime = 0;

    bool useFahrenheit = false;

    bool isTemperatureError;
};