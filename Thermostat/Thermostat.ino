#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <AutoPID.h>
#include <ButtonKing.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

#include "Adafruit_LEDBackpack.h"
#include "ThermostatFont.h"
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
// 960000 = 16 minutes
// 1920000 = 32 minutes
#define HEATER_RELAY_WINDOW_SIZE    1920000  
#define MIN_TOGGLE_TIME             120000

#define ABSOLUTE_MAX_TEMP 32.0f
#define ABSOLUTE_MIN_TEMP 10.0f

#define MIN_VALID_TEMP    -50.0f

#define KP 0.7
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
#define GRAPH_EEPROM_ADDR 1
#define LOCAL_EEPROM_ADDR 2

#define CURRENT_TEMP_BRIGHTNESS 1
#define TARGET_TEMP_BRIGHTNESS 15
#define TARGET_TEMP_DURATION 3000
#define POWER_ON_MSG_DURATION 1000

unsigned long lastTemperatureUpdate = 0;
RemoteThermostatController thermostatController(API_KEY, THERMOSTAT_NAME, false);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

double target, current;
bool pidState;
bool heaterState;
AutoPIDRelay pid(&current, &target, &pidState, HEATER_RELAY_WINDOW_SIZE, KP, KI, KD);

ButtonKing upButton(UP_BUTTON_PIN, false);
ButtonKing downButton(DOWN_BUTTON_PIN, false);
ButtonKing powerButton(POWER_BUTTON_PIN, false);

bool useFahrenheit = false;
bool didToggleFahrenheit = false;

bool showGraph = false;
bool didToggleGraph = false;

bool didToggleLocalMode = false;

Adafruit_8x16matrix matrix = Adafruit_8x16matrix();

unsigned long setTempTime = 0;
unsigned long lastToggleTime = 0;

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
  matrix.setCursor(0, 5);
  matrix.print("HI!");
  matrix.writeDisplay();

  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);
  heaterState = false;


  upButton.setLongClickStart(upButtonLongPressStart);
  upButton.setLongClickStop(upButtonLongPressStop);
  upButton.setClick([]()
  {
    adjustTargetTemp(1);
  });

  downButton.setLongClickStart(downButtonLongPressStart);
  downButton.setLongClickStop(downButtonLongPressStop);
  downButton.setClick([]()
  {
    adjustTargetTemp(-1);
  });


  powerButton.setLongClickStart(powerButtonLongPressStart);
  powerButton.setLongClickStop(powerButtonLongPressStop);
  powerButton.setClick([]()
  {
    toggleThermostatPower();
  });

  sensors.requestTemperatures(); // Get initial temperature reading
  sensors.setWaitForConversion(false);

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
  }

  EEPROM.begin(3);
  useFahrenheit = boolean(EEPROM.read(FARENHEIT_EEPROM_ADDR));  
  showGraph = boolean(EEPROM.read(GRAPH_EEPROM_ADDR));
  thermostatController.SetLocalMode(boolean(EEPROM.read(LOCAL_EEPROM_ADDR)));
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

    if (thermostatController.WasPowerSetRemotely())
    {
      setTempTime = millis();
      didSetThermostatPowerOn = true;
    }
    else if (thermostatController.WasTemperatureSetRemotely())
    {
      setTempTime = millis();
    }

    thermostatController.Update();
  }

  updateLED();
}

void upButtonLongPressStart()
{
  if (!didToggleFahrenheit)
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

void downButtonLongPressStart()
{
  if (!didToggleGraph)
  {
    showGraph = !showGraph;
    didToggleGraph = true;

    EEPROM.write(GRAPH_EEPROM_ADDR, (byte)showGraph);
    EEPROM.commit();    
  }
}

void downButtonLongPressStop()
{
  didToggleGraph = false;
}


void powerButtonLongPressStart()
{
  if (!didToggleLocalMode)
  {
    thermostatController.SetLocalMode(!thermostatController.IsInLocalMode());
    didToggleLocalMode = true;

    EEPROM.write(LOCAL_EEPROM_ADDR, (byte)thermostatController.IsInLocalMode());
    EEPROM.commit();    
  }
}

void powerButtonLongPressStop()
{
  didToggleLocalMode = false;
}

void updateLED()
{
  matrix.setFont(&Thermostat_Font);
  matrix.setTextSize(1);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  //matrix.setRotation(3);

  matrix.clear();
  matrix.setCursor(0, 4);

  if (millis() < setTempTime + TARGET_TEMP_DURATION)
  {
    matrix.setBrightness(TARGET_TEMP_BRIGHTNESS);

    if (thermostatController.GetPowerState() == false)
    {
      matrix.print(F("OFF"));
    }
    else
    {
      if (didSetThermostatPowerOn && millis() < setTempTime + POWER_ON_MSG_DURATION)
      {
        matrix.print(F("ON"));
      }
      else
      {
        if (useFahrenheit)
        {
          matrix.print(int(round((thermostatController.GetTargetTemperature() * 9 / 5) + 32)));
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

    if (temperatureError)
    {
      matrix.print(F("ERR"));
    }
    else
    {
      if (useFahrenheit)
      {
        matrix.print(int(round((thermostatController.GetCurrentTemperature() * 9 / 5) + 32)));
        matrix.print(F("f"));
      }
      else
      {
        matrix.print(int(round(thermostatController.GetCurrentTemperature())));
        matrix.print(F("c"));
      }

      if(thermostatController.IsInLocalMode())
      {
        matrix.print(F("!!!"));
      }
    }

    if (showGraph && thermostatController.GetPowerState())
    {
      drawPulseState();
    }
  }

  matrix.writeDisplay();

}

void drawPulseState()
{
  int totalWidth = 16;
  int blinking = (int)(millis() / 500) % 2;

  float t = (float)(millis() % HEATER_RELAY_WINDOW_SIZE) / HEATER_RELAY_WINDOW_SIZE;
  //t /= HEATER_RELAY_WINDOW_SIZE;

  //  USE_SERIAL.print(F("Millis: "));
  //  USE_SERIAL.print(millis());
  //  USE_SERIAL.print(F("  Last Time: "));
  //  USE_SERIAL.println(pid.getLastPulseTime());
  //  USE_SERIAL.print(F("T: "));
  //  USE_SERIAL.println(t);

  if (pid.getPulseValue() > 0.0f)
  {
    matrix.drawLine(0, 6, pid.getPulseValue() * totalWidth, 6, LED_ON);
  }

  if (pid.getPulseValue() < 1.0f)
  {
    matrix.drawLine(pid.getPulseValue() * totalWidth, 7, totalWidth, 7, LED_ON);
  }

  matrix.drawPixel(t * totalWidth, heaterState ? 6 : 7, blinking);
  //matrix.drawLine(0,7,(int)((pid.getPulseValue() / HEATER_RELAY_WINDOW_SIZE) * totalWidth),7,1);

}

void updateTemperature()
{

  if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL)
  {
    float currentTemperature = sensors.getTempCByIndex(0);

    if (currentTemperature > MIN_VALID_TEMP)
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
  if (thermostatController.GetPowerState())
  {
    if(millis() < setTempTime + TARGET_TEMP_DURATION)
    {    
      if (useFahrenheit)
      {
        float f = (thermostatController.GetTargetTemperature() * 9 / 5) + 32;
        f += delta;
        thermostatController.SetTargetTemperature((f - 32) * 5 / 9);
      }
      else
      {
        int temp = min(ABSOLUTE_MAX_TEMP, max(ABSOLUTE_MIN_TEMP, thermostatController.GetTargetTemperature() + delta));
        thermostatController.SetTargetTemperature(temp);  
      }
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
  if(isOn != heaterState)
  {
    lastToggleTime = millis();

    if (isOn)
    {
      heaterState = true;
      digitalWrite(HEATER_PIN, HIGH);
    }
    else
    {
      heaterState = false;
      digitalWrite(HEATER_PIN, LOW);
    }
  }
  
  
}

void updateHeaterController()
{
  current = (double)thermostatController.GetCurrentTemperature();

  // If the thermostat is unpowered, simply set the target to current. This prevents the relay from toggling too rapidly if a user spams the power
  if (thermostatController.GetPowerState() && !temperatureError)
  {
    target = (double)thermostatController.GetTargetTemperature();
  }
  else
  {
    target = (double)thermostatController.GetCurrentTemperature();
  }


  pid.run();

  if (millis() - lastToggleTime > MIN_TOGGLE_TIME)
  {
    toggleHeater(pidState);  
  }
}

unsigned long getTimeLeftInRelayPulse()
{
  unsigned long timeInCurrentWindow = millis() % HEATER_RELAY_WINDOW_SIZE;
  unsigned long pulseTime = pid.getPulseValue() * HEATER_RELAY_WINDOW_SIZE;
  return pulseTime - timeInCurrentWindow;
}
