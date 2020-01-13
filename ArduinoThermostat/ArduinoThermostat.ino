#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <AutoPID.h>

#include "Configuration.h"
/*
   Configuration.h is where the following is defined:
   API Key for accessing the remote server.
   Name of the thermostat (This will be used if controlling multiple thermostats is ever supported).
   Wifi SSID and password.
*/

#include "RemoteThermostatController.h"

#define USE_SERIAL Serial

#define SERVER_POLL_INTERVAL 10000
#define TEMPERATURE_POLL_INTERVAL 800

#define HEATER_RELAY_WINDOW_SIZE 30000

#define OUTPUT_MIN 0
#define OUTPUT_MAX 255
#define KP 2
#define KI 5
#define KD 1

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 9

#define STATUS_LED_PIN BUILTIN_LED
#define HEATER_PIN 1

unsigned long lastTemperatureUpdate = 0;
unsigned long heaterWindowStartTime = 0;

RemoteThermostatController thermostatController(API_KEY, THERMOSTAT_NAME, false);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

double target, current;
bool relayState;
AutoPIDRelay pid(&current, &target, &relayState, HEATER_RELAY_WINDOW_SIZE, KP, KI, KD);

#define WINDOW_DISPLAY_WIDTH 30
char* displayString = new char[WINDOW_DISPLAY_WIDTH+1];

void setup()
{
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);
  
  USE_SERIAL.begin(115200);

  WiFi.begin(STASSID, STAPSK);

  USE_SERIAL.print("Connecing to Wireless...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    USE_SERIAL.print(".");
  }
  USE_SERIAL.println(" Done");

  pid.setBangBang(10);
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
  if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL)
  {
    thermostatController.SetCurrentTemperature(sensors.getTempCByIndex(0));

    USE_SERIAL.print("Detected temperature is ");
    USE_SERIAL.println(thermostatController.GetCurrentTemperature());
    USE_SERIAL.println("");

    lastTemperatureUpdate = millis();
    sensors.requestTemperatures(); //request reading for next time
  }
}

void updateHeaterController()
{
  current = (double)thermostatController.GetCurrentTemperature();
  target = (double)thermostatController.GetTargetTemperature();

  

  pid.run();

  if (relayState)
  {
    digitalWrite(HEATER_PIN, LOW);
    digitalWrite(STATUS_LED_PIN, LOW);
  }  
  else
  {
    digitalWrite(HEATER_PIN, HIGH);
    digitalWrite(STATUS_LED_PIN, HIGH);
  }

   // Debug Output
  int displayWidth = 20;
  int on = (pid.getPulseValue() / (float)HEATER_RELAY_WINDOW_SIZE) * displayWidth;
  int off = displayWidth - on;

  for(int i = 0; i < WINDOW_DISPLAY_WIDTH; i++)
  {
      if(i <= on)
      {
        displayString[i] = '^';
      }
      else
      {
        displayString[i] = '_';
      }
  }
  displayString[displayWidth] = '\0';

  USE_SERIAL.print("Current temperature: ");
  USE_SERIAL.println(current);

  USE_SERIAL.print("Target temperature: ");
  USE_SERIAL.println(target);

  USE_SERIAL.print("Pulse Width: ");
  USE_SERIAL.println(pid.getPulseValue());
}
