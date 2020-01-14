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

#define MAX_TEMP 28.0f
#define MIN_TEMP 18.0f

#define KP 0.5
#define KI 0.05
#define KD 0.01

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 9

#define STATUS_LED_PIN BUILTIN_LED
#define HEATER_PIN 1

#define UP_BUTTON_PIN  D5
#define DOWN_BUTTON_PIN D6

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

int upButtonState = 0;
int downButtonState = 0;

void setup()
{
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(STATUS_LED_PIN, OUTPUT);

  pinMode(UP_BUTTON_PIN, INPUT);
  
  USE_SERIAL.begin(115200);

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
  if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL)
  {
    thermostatController.SetCurrentTemperature(sensors.getTempCByIndex(0));

//    USE_SERIAL.print("Detected temperature is ");
//    USE_SERIAL.println(thermostatController.GetCurrentTemperature());
//    USE_SERIAL.println("");

    lastTemperatureUpdate = millis();
    sensors.requestTemperatures(); //request reading for next time
  }

  int upButton = digitalRead(UP_BUTTON_PIN);
  int downButton = digitalRead(DOWN_BUTTON_PIN);

  if(upButton && !upButtonState)
  {
    int temp = min(MAX_TEMP, thermostatController.GetTargetTemperature() + 1);
    thermostatController.SetTargetTemperature(temp);

    USE_SERIAL.print("Setting temp to ");
    USE_SERIAL.println(temp);
  }
  upButtonState = upButton;

  if(downButton && !downButtonState)
  {
    int temp = max(MIN_TEMP, thermostatController.GetTargetTemperature() - 1);
    thermostatController.SetTargetTemperature(temp);

    USE_SERIAL.print("Setting temp to ");
    USE_SERIAL.println(temp);
  }  
  downButtonState = downButton;

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
