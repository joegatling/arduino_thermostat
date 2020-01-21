#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <AutoPID.h>
#include <ButtonKing.h>
#include <EEPROM.h>
#include "RemoteThermostatController.h"


#include "Configuration.h"
/*
   Configuration.h is where the following is defined:
   API Key for accessing the remote server.
   Name of the thermostat (This will be used if controlling multiple thermostats is ever supported).
   Wifi SSID and password.
*/



#define USE_SERIAL Serial

#define TEMPERATURE_POLL_INTERVAL   5000
#define HEATER_RELAY_WINDOW_SIZE    300000

#define ABSOLUTE_MAX_TEMP 32.0f
#define ABSOLUTE_MIN_TEMP 10.0f

#define KP 0.5
#define KI 0 //0.05
#define KD 0 //0.01

#define HEATER_PIN        D2
#define ONE_WIRE_PIN      D4
#define UP_BUTTON_PIN     D5
#define DOWN_BUTTON_PIN   D6
#define POWER_BUTTON_PIN  D7

#define BUTTON_DEBOUNCE_TIME 50

#define FARENHEIT_EEPROM_ADDR 0



unsigned long lastTemperatureUpdate = 0;
RemoteThermostatController thermostatController(API_KEY, THERMOSTAT_NAME, false);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

double target, current;
bool relayState;
AutoPIDRelay pid(&current, &target, &relayState, HEATER_RELAY_WINDOW_SIZE, KP, KI, KD);

ButtonKing upButton(UP_BUTTON_PIN, false);
ButtonKing downButton(DOWN_BUTTON_PIN, false);

bool useFarenheit = false;
bool didToggleFahrenheit = false;

void setup()
{
    USE_SERIAL.begin(115200);
    
    pinMode(HEATER_PIN, OUTPUT);

    upButton.setClick([]() 
    {
        adjustTargetTemp(1);
    });

    upButton.setLongClickStart(upButtonLongPressStart);
    upButton.setLongClickStop(upButtonLongPressStop);
  
    downButton.setClick([]() 
    {
        adjustTargetTemp(-1);
    });        

    sensors.requestTemperatures(); // Get initial temperature reading
  
    WiFi.begin(STASSID, STAPSK);
  
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    useFarenheit = boolean(EEPROM.read(FARENHEIT_EEPROM_ADDR));  
}

void loop()
{
    updateTemperature();
    updateHeaterController();
  
    // wait for WiFi connection
    if (WiFi.status() == WL_CONNECTED)
    {
        thermostatController.Update();
    }
}

void upButtonLongPressStart()
{
  if(!didToggleFahrenheit)
  {
    useFarenheit = !useFarenheit;
    didToggleFahrenheit = true;

    EEPROM.write(FARENHEIT_EEPROM_ADDR, (byte)useFarenheit);
    EEPROM.commit();
  
    USE_SERIAL.print("Use Farenheit: ");
    USE_SERIAL.println(useFarenheit);
  }
}

void upButtonLongPressStop()
{
  didToggleFahrenheit = false;
}

void updateTemperature()
{
    upButton.isClick();
    downButton.isClick();
    
    if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL)
    {
        thermostatController.SetCurrentTemperature(sensors.getTempCByIndex(0));

        lastTemperatureUpdate = millis();
        sensors.requestTemperatures(); //request reading for next time
    }
}

void adjustTargetTemp(int delta)
{
    int temp = min(ABSOLUTE_MAX_TEMP, max(ABSOLUTE_MIN_TEMP, thermostatController.GetTargetTemperature() + delta));
    thermostatController.SetTargetTemperature(temp);
}

void toggleHeater(bool isOn)
{
    if (isOn)
    {
        digitalWrite(HEATER_PIN, HIGH);
    }  
    else
    {
        digitalWrite(HEATER_PIN, LOW);
    }  
}

void updateHeaterController()
{
    current = (double)thermostatController.GetCurrentTemperature();
    target = (double)thermostatController.GetTargetTemperature();

    if(thermostatController.GetPowerState())
    {
        pid.run();
        toggleHeater(relayState);      
    }
    else
    {
        if(!pid.isStopped())
        {
          pid.stop();
          toggleHeater(false);
        }
    }
}
