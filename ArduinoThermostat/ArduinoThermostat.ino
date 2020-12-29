#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <AutoPID.h>
#include <ButtonKing.h>
#include <EEPROM.h>
#include "RemoteThermostatController.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "ThermostatFont.h"


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

#define MIN_VALID_TEMP    -50.0f

#define KP 0.5
#define KI 0 //0.05
#define KD 0 //0.01

#define HEATER_PIN        D0
#define LED_CLOCK         D1
#define LED_DATA          D2
#define ONE_WIRE_PIN      D4
#define UP_BUTTON_PIN     D5
#define DOWN_BUTTON_PIN   D6
#define POWER_BUTTON_PIN  D7

#define BUTTON_DEBOUNCE_TIME 50

#define FARENHEIT_EEPROM_ADDR 0

#define CURRENT_TEMP_BRIGHTNESS 1
#define TARGET_TEMP_BRIGHTNESS 15
#define TARGET_TEMP_DURATION 3000
#define POWER_ON_MSG_DURATION 1000

unsigned long lastTemperatureUpdate = 0;
RemoteThermostatController thermostatController(API_KEY, THERMOSTAT_NAME, false);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

double target, current;
bool relayState;
AutoPIDRelay pid(&current, &target, &relayState, HEATER_RELAY_WINDOW_SIZE, KP, KI, KD);

ButtonKing upButton(UP_BUTTON_PIN, false);
ButtonKing downButton(DOWN_BUTTON_PIN, false);
ButtonKing powerButton(POWER_BUTTON_PIN, false);

bool useFahrenheit = false;
bool didToggleFahrenheit = false;

Adafruit_8x16matrix matrix = Adafruit_8x16matrix();

unsigned long setTempTime = 0;

bool temperatureError = false;
bool didSetThermostatPowerOn = false;

bool oldPowerState = false;
float oldTemperature = 0;

void setup()
{
    USE_SERIAL.begin(115200);
    matrix.begin(0x70);  // pass in the address

    matrix.setFont(&Thermostat_Font);
    matrix.setTextSize(1);
    matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
    matrix.setTextColor(LED_ON);
    matrix.setRotation(3);
  
    matrix.setBrightness(15);
  
    matrix.clear();
    matrix.setCursor(0,5);
    matrix.print("Hi!");
    matrix.writeDisplay();
    
    
    pinMode(HEATER_PIN, OUTPUT);

    upButton.setLongClickStart(upButtonLongPressStart);
    upButton.setLongClickStop(upButtonLongPressStop);

    upButton.setClick([]() 
    {
        adjustTargetTemp(1);
    });
  
    downButton.setClick([]() 
    {
        adjustTargetTemp(-1);
    });  

    powerButton.setClick([]()
    {
        toggleThermostatPower();
    });

    sensors.requestTemperatures(); // Get initial temperature reading
  
    WiFi.begin(STASSID, STAPSK);
  
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    useFahrenheit = boolean(EEPROM.read(FARENHEIT_EEPROM_ADDR));  
}

void loop()
{
    upButton.isClick();
    downButton.isClick();
    powerButton.isClick();
  
    updateTemperature();
    updateHeaterController();
  
    // wait for WiFi connection
    if (WiFi.status() == WL_CONNECTED)
    {
        thermostatController.Update();
    }

    updateLED();
}

void upButtonLongPressStart()
{
  if(!didToggleFahrenheit)
  {
    useFahrenheit = !useFahrenheit;
    didToggleFahrenheit = true;

    EEPROM.write(FARENHEIT_EEPROM_ADDR, (byte)useFahrenheit);
    EEPROM.commit();
  
    USE_SERIAL.print("Use Farenheit: ");
    USE_SERIAL.println(useFahrenheit);
  }
}

void upButtonLongPressStop()
{
  didToggleFahrenheit = false;
}

void updateLED()
{
  matrix.setFont(&Thermostat_Font);
  matrix.setTextSize(1);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  //matrix.setRotation(3);

  matrix.clear();
  matrix.setCursor(0,5);

  if(millis() < setTempTime + TARGET_TEMP_DURATION)
  {
    matrix.setBrightness(TARGET_TEMP_BRIGHTNESS);

    if(thermostatController.GetPowerState() == false)
    {
      matrix.print(F("OFF"));    
    }
    else
    {
      if(didSetThermostatPowerOn && millis() < setTempTime + POWER_ON_MSG_DURATION)
      {
        matrix.print(F("ON"));    
      }
      else
      {        
        if(useFahrenheit)
        {
          matrix.print(int(round((thermostatController.GetTargetTemperature() * 9/5) + 32)));
          matrix.print(F("f"));    
        }
        else
        {
          matrix.print(int(round(thermostatController.GetTargetTemperature())));
          matrix.print(F("c"));        
        }   
      }
    }
    
  }
  else
  {
    matrix.setBrightness(CURRENT_TEMP_BRIGHTNESS);
    didSetThermostatPowerOn = false;

    if(temperatureError)
    {
      matrix.print(F("ERR"));    
    }
    else
    {
      if(useFahrenheit)
      {
        matrix.print(int(round((thermostatController.GetCurrentTemperature() * 9/5) + 32)));
        matrix.print(F("f"));    
      }
      else
      {
        matrix.print(int(round(thermostatController.GetCurrentTemperature())));
        matrix.print(F("c"));        
      }  
    }
  }
  
  matrix.writeDisplay();
  
}

void updateTemperature()
{
    
    if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL)
    {
        float currentTemperature = sensors.getTempCByIndex(0);

        if(currentTemperature > MIN_VALID_TEMP)
        {      
          thermostatController.SetCurrentTemperature(sensors.getTempCByIndex(0));
          temperatureError = false;
        }
        else
        {
          temperatureError = true;
        }
        
        lastTemperatureUpdate = millis();
        sensors.requestTemperatures(); //request reading for next time
    }
}

void adjustTargetTemp(int delta)
{
  if(thermostatController.GetPowerState())
  {
    if(useFahrenheit)
    {
      float f = (thermostatController.GetTargetTemperature() * 9/5) + 32;
      Serial.print("f ");
      Serial.println(f);
      f += delta;
      Serial.print("(after) f ");
      Serial.println(f);
      thermostatController.SetTargetTemperature((f - 32) * 5/9);
    }
    else
    {
      int temp = min(ABSOLUTE_MAX_TEMP, max(ABSOLUTE_MIN_TEMP, thermostatController.GetTargetTemperature() + delta));
      thermostatController.SetTargetTemperature(temp);

    }
  }

    setTempTime = millis();
}

void toggleThermostatPower()
{
    thermostatController.SetPowerState(!thermostatController.GetPowerState());
    didSetThermostatPowerOn = thermostatController.GetPowerState();

    setTempTime = millis();
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

    // If the thermostat is unpowered, simply set the target to current. This prevents the relay from toggling too rapidly if a user spams the power
    if(thermostatController.GetPowerState() && !temperatureError)
    {
      target = (double)thermostatController.GetTargetTemperature();
    }
    else
    {
      target = (double)thermostatController.GetCurrentTemperature();
    }

    pid.run();
    toggleHeater(relayState);      
}
