#include "ButtonController.h"

// Static instance pointer for callbacks
ButtonController* ButtonController::s_instance = nullptr;

void ButtonController::upButtonEndPressCallback() {
    if (s_instance) s_instance->onUpButtonEndPress();
}

void ButtonController::downButtonEndPressCallback() {
    if (s_instance) s_instance->onDownButtonEndPress();
}

void ButtonController::powerButtonEndPressCallback() {
    if (s_instance) s_instance->onPowerButtonEndPress();
}

void ButtonController::upButtonHoldCallback() {
    if (s_instance) s_instance->onUpButtonHold();
}

void ButtonController::downButtonHoldCallback() {
    if (s_instance) s_instance->onDownButtonHold();
}

void ButtonController::powerButtonHoldCallback() {
    if (s_instance) s_instance->onPowerButtonHold();
}

ButtonController::ButtonController() :
    upButton(UP_BUTTON_PIN),
    downButton(DOWN_BUTTON_PIN),
    powerButton(POWER_BUTTON_PIN),
    thermostat(nullptr),
    configModeCallback(nullptr)
{
    s_instance = this;
}

ButtonController::~ButtonController()
{
}

void ButtonController::setThermostat(Thermostat* newThermostat)
{
    if (newThermostat == nullptr)
    {
        return;
    }

    if(thermostat != nullptr)
    {
        return; // Already initialized
    }
    

    this->thermostat = newThermostat;

    upButton.SetEndPressCallback(ButtonController::upButtonEndPressCallback);
    upButton.SetHoldCallback(ButtonController::upButtonHoldCallback);
    
    downButton.SetEndPressCallback(ButtonController::downButtonEndPressCallback);
    downButton.SetHoldCallback(ButtonController::downButtonHoldCallback);

    powerButton.SetEndPressCallback(ButtonController::powerButtonEndPressCallback);
    powerButton.SetHoldCallback(ButtonController::powerButtonHoldCallback);
}

void ButtonController::update()
{
    if (thermostat == nullptr)
    {
        return;
    }

    upButton.Update();
    downButton.Update();
    powerButton.Update();

    // TODO: Handle button press events
    // Check button states and update thermostat accordingly
}

void ButtonController::onUpButtonEndPress()
{
    if (thermostat == nullptr) return;

    thermostat->setTargetTemperature(thermostat->getTargetTemperature() + 1);
    
    if(thermostat->getCurrentTemperature() < thermostat->getTargetTemperature())
    {
        thermostat->setMode(BOOST);
    }
    else
    {
        thermostat->setMode(HEAT);
    }
}

void ButtonController::onDownButtonEndPress()
{
    if (thermostat == nullptr) return;

    thermostat->setMode(HEAT);
    thermostat->setTargetTemperature(thermostat->getTargetTemperature() - 1);
}

void ButtonController::onPowerButtonEndPress()
{
    if (thermostat == nullptr) return;
    if(thermostat->getMode() == OFF)
    {
        thermostat->setMode(HEAT);
    }
    else
    {
        thermostat->setMode(OFF);
    }
}

void ButtonController::onUpButtonHold()
{
    if (thermostat == nullptr) return;
    thermostat->setUsingFahrenheit(!thermostat->isUsingFahrenheit());
}

void ButtonController::onDownButtonHold()
{
    if (thermostat == nullptr) return;
    // Optional: Implement long press behavior for down button
}   

void ButtonController::onPowerButtonHold()
{
    if (thermostat == nullptr) return;

    if (configModeCallback) 
    {
        configModeCallback();
    }
}   

void ButtonController::setConfigModeCallback(std::function<void()> callback)
{
    this->configModeCallback = callback;
}