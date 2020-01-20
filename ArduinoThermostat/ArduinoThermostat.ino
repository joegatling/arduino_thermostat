#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <AutoPID.h>
#include <ButtonDebounce.h>
#include "RemoteThermostatController.h"

#include "Configuration.h"
/*
   Configuration.h is where the following is defined:
   API Key for accessing the remote server.
   Name of the thermostat (This will be used if controlling multiple thermostats is ever supported).
   Wifi SSID and password.
*/



#define USE_SERIAL Serial

#define TEMPERATURE_POLL_INTERVAL   800
#define HEATER_RELAY_WINDOW_SIZE    30000

#define MAX_TEMP 30.0f
#define MIN_TEMP 18.0f

#define KP 0.5
#define KI 0 //0.05
#define KD 0 //0.01

#define ONE_WIRE_PIN      D4
#define UP_BUTTON_PIN     D5
#define DOWN_BUTTON_PIN   D6
#define POWER_BUTTON_PIN  D7
#define STATUS_LED_PIN    BUILTIN_LED
#define HEATER_PIN        D2

#define BUTTON_DEBOUNCE_TIME 50



unsigned long lastTemperatureUpdate = 0;
RemoteThermostatController thermostatController(API_KEY, THERMOSTAT_NAME, false);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

double target, current;
bool relayState;
AutoPIDRelay pid(&current, &target, &relayState, HEATER_RELAY_WINDOW_SIZE, KP, KI, KD);

ButtonDebounce upButton(UP_BUTTON_PIN, BUTTON_DEBOUNCE_TIME);
ButtonDebounce downButton(DOWN_BUTTON_PIN, BUTTON_DEBOUNCE_TIME);



void setup()
{
    USE_SERIAL.begin(115200);
    
    pinMode(STATUS_LED_PIN, OUTPUT);
  
    upButton.setCallback([](const int state) 
    {
        if(state)
        {
            adjustTargetTemp(1);
        }
    });
  
    downButton.setCallback([](const int state) 
    {
        if(state)
        {
            adjustTargetTemp(-1);
        }
    });  
      

    sensors.requestTemperatures(); // Get initial temperature reading
  
    WiFi.begin(STASSID, STAPSK);
  
    USE_SERIAL.print("Connecing to Wireless...");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        USE_SERIAL.print(".");
    }
    USE_SERIAL.println(" Done");
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

void updateTemperature()
{
    upButton.update();
    downButton.update();
    
    if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL)
    {
        thermostatController.SetCurrentTemperature(sensors.getTempCByIndex(0));

//        USE_SERIAL.println(sensors.getTempCByIndex(0));
      
        lastTemperatureUpdate = millis();
        sensors.requestTemperatures(); //request reading for next time
    }
}

void adjustTargetTemp(int delta)
{
    int temp = min(MAX_TEMP, max(MIN_TEMP, thermostatController.GetTargetTemperature() + delta));
    thermostatController.SetTargetTemperature(temp);

    USE_SERIAL.print("Setting temp to ");
    USE_SERIAL.println(temp);  
}

void toggleHeater(bool isOn)
{
    if (isOn)
    {
        digitalWrite(HEATER_PIN, LOW);
        digitalWrite(STATUS_LED_PIN, LOW);
    }  
    else
    {
        digitalWrite(HEATER_PIN, HIGH);
        digitalWrite(STATUS_LED_PIN, HIGH);
    }  
}

void updateHeaterController()
{
    current = (double)thermostatController.GetCurrentTemperature();
    target = (double)thermostatController.GetTargetTemperature();
  
    pid.run();

    toggleHeater(relayState);

//
//  USE_SERIAL.print("Current temperature: ");
//  USE_SERIAL.println(current);
//
//  USE_SERIAL.print("Target temperature: ");
//  USE_SERIAL.println(target);
//
//  USE_SERIAL.print("Duration: ");
//  USE_SERIAL.println(pid.getPulseValue() * HEATER_RELAY_WINDOW_SIZE);
}
