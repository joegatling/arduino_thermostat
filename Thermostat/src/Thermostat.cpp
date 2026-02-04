#include "Thermostat.h"

// Pin definitions
#define ONE_WIRE_PIN A3
#define HEATER_RELAY_PIN GPIO_NUM_36

#define TEMPERATURE_POLL_INTERVAL   5000
#define TEMPERATURE_ERROR_OFFSET    -1.0

// 960000 = 16 minutes
// 1920000 = 32 minutes
#define HEATER_RELAY_WINDOW_SIZE    1920000  
#define HEATER_MIN_TOGGLE_TIME      120000

#define ABSOLUTE_MAX_TEMP_C           32.0f
#define ABSOLUTE_MIN_TEMP_C           10.0f

#define MIN_VALID_TEMP              -50.0f
#define MIN_TEMPERATURE_DIFFEREENCE 0.01f

#define GEORGE_BOOST_TIME           180000

// Temperature sensor

Thermostat::Thermostat() :
    heaterPID(&currentTemperature, &heaterTargetTemperature, &pidState, HEATER_RELAY_WINDOW_SIZE, THERMOSTAT_KP, THERMOSTAT_KI, THERMOSTAT_KD),
    oneWire(ONE_WIRE_PIN),
    sensors(&oneWire),

    currentTemperature(20.0f),
    targetTemperature(20.0f),
    heaterTargetTemperature(20.0f),
    heaterState(HEATER_MIN_TOGGLE_TIME, false),
    pidState(false),

    currentMode(OFF),
    currentPreset(NONE),

    lastTemperatureUpdateTime(0),
    lastHeaterToggleTime(0),
    lastPresetChangeTime(0),
    
    isTemperatureError(false)
{
    pinMode(HEATER_RELAY_PIN, OUTPUT);
    digitalWrite(HEATER_RELAY_PIN, LOW); 

    sensors.setWaitForConversion(false);    
    sensors.requestTemperatures(); // Get initial temperature reading
}

Thermostat::~Thermostat()
{
    digitalWrite(HEATER_RELAY_PIN, LOW);
}

void Thermostat::setTargetTemperature(float newTargetTemperature, bool forceCelsius)
{
    // If forceCelsius is true, then the temperature will be provided in celsius, ignoring
    // the current unit setting.

    if(useFahrenheit)
    {
        if(forceCelsius)
        {
            newTargetTemperature = C_TO_F(newTargetTemperature);
        }

        newTargetTemperature = max(C_TO_F(ABSOLUTE_MIN_TEMP_C), newTargetTemperature);
        newTargetTemperature = min(C_TO_F(ABSOLUTE_MAX_TEMP_C), newTargetTemperature);
    }
    else
    {
        newTargetTemperature = max(ABSOLUTE_MIN_TEMP_C, newTargetTemperature);
        newTargetTemperature = min(ABSOLUTE_MAX_TEMP_C, newTargetTemperature);
    }
 
    if (fabs(targetTemperature - newTargetTemperature) >= MIN_TEMPERATURE_DIFFEREENCE) 
    {
        targetTemperature = newTargetTemperature;
        onTargetTemperatureChangedEvent.emit(newTargetTemperature);
    }
}

float Thermostat::getTargetTemperature(bool forceCelsius)
{
    float temperature = targetTemperature;

    // If forceCelsius is true and we're using Fahrenheit, convert to Celsius
    if (forceCelsius && useFahrenheit) 
    {
        temperature = F_TO_C(temperature);
    }
    
    return temperature;
}

float Thermostat::getCurrentTemperature(bool forceCelsius)
{
    float temperature = currentTemperature;

    // If forceCelsius is true and we're using Fahrenheit, convert to Celsius
    if (forceCelsius && useFahrenheit) 
    {
        temperature = F_TO_C(temperature);
    }
    
    return temperature;
}

void Thermostat::update()
{
    updateCurrentTemperature();
    updateHeater();
}

size_t Thermostat::onTargetTemperatureChanged(std::function<void(float)> callback)
{
    return onTargetTemperatureChangedEvent.subscribe(callback);
}

size_t Thermostat::onCurrentTemperatureChanged(std::function<void(float)> callback)
{
    return onCurrentTemperatureChangedEvent.subscribe(callback);
}

size_t Thermostat::onModeChanged(std::function<void(ThermostatMode)> callback)
{
    return onModeChangedEvent.subscribe(callback);
}

size_t Thermostat::onUseFahrenheitChanged(std::function<void(bool)> callback)
{
    return onUseFahrenheitChangedEvent.subscribe(callback);
}

size_t Thermostat::onHeaterPowerChanged(std::function<void(bool)> callback)
{
    return onHeaterPowerChangedEvent.subscribe(callback);
}

size_t Thermostat::onPresetChanged(std::function<void(ThermostatPreset)> callback)
{
    return onPresetChangedEvent.subscribe(callback);
}

void Thermostat::updateCurrentTemperature()
{
    unsigned long currentTime = millis();
    if (currentTime - lastTemperatureUpdateTime >= TEMPERATURE_POLL_INTERVAL) 
    {
        lastTemperatureUpdateTime = millis();

        // Serial.print("Reading Temperature... ");
        float newTemperature = sensors.getTempCByIndex(0) + TEMPERATURE_ERROR_OFFSET;
        // Serial.print("Done (");
        // Serial.print(newTemperature);
        // Serial.println("C)");

        sensors.requestTemperatures(); //request new temperature readings

        if(newTemperature < MIN_VALID_TEMP)
        {
            isTemperatureError = true; // retain old temperature on error
            return;
        }

        isTemperatureError = false;

        if(isUsingFahrenheit())
        {
            newTemperature = C_TO_F(newTemperature);
        }

        if(fabs(newTemperature - currentTemperature) >= MIN_TEMPERATURE_DIFFEREENCE)
        {
            currentTemperature = newTemperature;
            onCurrentTemperatureChangedEvent.emit(currentTemperature);
        }
        
    }
}

void Thermostat::updateHeater()
{
    // If there's a temperature sensor error, switch to OFF mode for safety.
    // Manual intervention is required to switch back to HEAT mode after the error clears,
    // ensuring the user is aware of the error condition before resuming normal operation.
    if(isTemperatureError && currentMode != OFF)
    {
        setMode(OFF);
    }

    if(currentMode == OFF)
    {
        heaterTargetTemperature = MIN_VALID_TEMP;
        heaterState.setValue(false, true); // turn off heater immediately
    }
    else if(currentMode == HEAT)
    {
        bool forceOn = false;

        if(currentPreset == BOOST)
        {
            heaterTargetTemperature = targetTemperature;
            forceOn = currentTemperature < targetTemperature;

            if(millis() - lastPresetChangeTime >= GEORGE_BOOST_TIME) // Automatically return to eco preset
            {
                setPreset(ECO);
            }   
        }
        else if(currentPreset == SLEEP)
        {
            heaterTargetTemperature = targetTemperature - (useFahrenheit ? C_TO_F_DELTA(3.0f) : 3.0f);
        }
        else
        {
            heaterTargetTemperature = targetTemperature;
        }

        heaterPID.run();
        heaterState.setValue(pidState || forceOn, forceOn);         
    }

    if(previousHeaterState != heaterState.getValue())
    {
        previousHeaterState = heaterState.getValue();
        onHeaterPowerChangedEvent.emit(heaterState.getValue());
    }

    if(heaterState.getValue())
    {
        digitalWrite(HEATER_RELAY_PIN, HIGH);
    }
    else
    {
        digitalWrite(HEATER_RELAY_PIN, LOW);
    }    
}

void Thermostat::setMode(ThermostatMode mode)
{
    if(currentMode != mode)
    {
        currentMode = mode;
        onModeChangedEvent.emit(mode);
    }
}

ThermostatMode Thermostat::getMode()
{
    return currentMode;
}

void Thermostat::setPreset(ThermostatPreset preset)
{
    if(currentPreset != preset)
    {
        currentPreset = preset;
        lastPresetChangeTime = millis();
        onPresetChangedEvent.emit(preset);
    }
}

ThermostatPreset Thermostat::getPreset()
{
    return currentPreset;
}

void Thermostat::setUsingFahrenheit(bool shouldUseFahrenheit)
{
    if(useFahrenheit != shouldUseFahrenheit)
    {
        useFahrenheit = shouldUseFahrenheit;

        // Convert current and target temperatures to the new unit
        if(useFahrenheit)
        {            
            currentTemperature = C_TO_F(currentTemperature);            
            targetTemperature = C_TO_F(targetTemperature);
        }
        else
        {
            currentTemperature = F_TO_C(currentTemperature);
            targetTemperature = F_TO_C(targetTemperature);
        }

        onUseFahrenheitChangedEvent.emit(useFahrenheit);
        onCurrentTemperatureChangedEvent.emit(currentTemperature);
        onTargetTemperatureChangedEvent.emit(targetTemperature);
    }
}

bool Thermostat::isUsingFahrenheit()
{
    return useFahrenheit;
}

bool Thermostat::isUsingCelsius()
{
    return !useFahrenheit;
}