#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#if NODE_MCU
  #include <ESP8266WiFi.h>
  #include <ESP8266HTTPClient.h>
#elif QT_PY
  #include <Adafruit_NeoPixel.h>
#endif

#if TEST_ENVIRONMENT
    #include <Adafruit_SSD1306.h>
#endif

#include <ArduinoJson.h>
#include <AutoPID.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <ArduinoOTA.h>

#include <AsyncHTTPRequest_Generic.h>

#include <Adafruit_LEDBackpack.h>

#include "ThermostatFont.h"
#include "RemoteThermostatController.h"
#include "SimpleButton.h"

#include <stdio.h>

#include "Configuration.h"
/*
   Configuration.h is where the following is defined:
   API Key for accessing the remote server.
   Name of the thermostat (This will be used if controlling multiple thermostats is ever supported).
   Wifi SSID and password.
*/

#define USE_PULLUP_BUTTONS          0  // Use pullup resistors on buttons.
//#define DEMO_MODE           0  // Demo mode sends random temperatures to the server.

#define TEMPERATURE_ERROR_OFFSET    -1.0

#define TEMPERATURE_POLL_INTERVAL   5000
// 960000 = 16 minutes
// 1920000 = 32 minutes
#define HEATER_RELAY_WINDOW_SIZE    1920000  
#define MIN_TOGGLE_TIME             120000

#define ABSOLUTE_MAX_TEMP           32.0f
#define ABSOLUTE_MIN_TEMP           10.0f

#define MIN_VALID_TEMP              -50.0f

#define GEORGE_BOOST_TIME           180000

#define KP 0.8
#define KI 0
#define KD 0

#if NODE_MCU
  #define HEATER_PIN        D0

  #define LED_CLOCK         D1
  #define LED_DATA          D2

  #define ONE_WIRE_PIN      D4

  #define UP_BUTTON_PIN     D5
  #define DOWN_BUTTON_PIN   D6
  #define POWER_BUTTON_PIN  D7
#elif QT_PY
  #define HEATER_PIN        GPIO_NUM_36

  #define LED_CLOCK         SCL
  #define LED_DATA          SDA

  #define ONE_WIRE_PIN      A3

  #define UP_BUTTON_PIN     A2
  #define DOWN_BUTTON_PIN   A1
  #define POWER_BUTTON_PIN  A0
#endif

#if TEST_ENVIRONMENT
  #define DISPLAY_SCALE       8
  #define DISPLAY_ROTATION    0
#else
  #define DISPLAY_SCALE       1
  #define DISPLAY_ROTATION    3
#endif

#define BUTTON_DEBOUNCE_TIME 50

#define FAHRENHEIT_EEPROM_ADDR        0
#define DEBUG_EEPROM_ADDR             1
#define LOCAL_EEPROM_ADDR             2

bool isInGeorgeBoostTime();
#define CURRENT_TEMP_BRIGHTNESS isInGeorgeBoostTime() ? 8 : 1

#define TARGET_TEMP_BRIGHTNESS        2
#define TARGET_TEMP_DURATION          3000
#define POWER_ON_MSG_DURATION         1000

#define STATUS_MESSAGE_DURATION       6000
#define STATUS_MESSAGE_SCROLL_DELAY   500
#define STATUS_MESSAGE_SCROLL_STEP    100
 
#define SERVER_TIMEOUT_TIME           120000

#define MESSAGE_WIFI          F("WIFI?")
#define MESSAGE_TIMEOUT_ERR   F("SERVER TIMEOUT ERROR")
#define MESSAGE_RECONNECTED   F("CONNECTED")
#define MESSAGE_TEMP_ERR      F("TEMP ERROR")
#define MESSAGE_LOCAL         F("WIFI: OFF")
#define MESSAGE_ONLINE        F("WIFI: ON")

#define TARGET_TEMPERATURE_F    int(round((thermostatController.GetTargetTemperature() * 9 / 5) + 32))
#define TARGET_TEMPERATURE_C    int(round(thermostatController.GetTargetTemperature()))

#define CURRENT_TEMPERATURE_F   int(round((thermostatController.GetCurrentTemperature() * 9 / 5) + 32))
#define CURRENT_TEMPERATURE_C   int(round(thermostatController.GetCurrentTemperature()))

unsigned long lastTemperatureUpdate = 0;
RemoteThermostatController thermostatController(API_KEY, THERMOSTAT_NAME, false);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

double target, current;
bool pidState;
bool heaterState;
AutoPIDRelay pid(&current, &target, &pidState, HEATER_RELAY_WINDOW_SIZE, KP, KI, KD);

#if QT_PY
  SimpleButton upButton(UP_BUTTON_PIN);
  SimpleButton downButton(DOWN_BUTTON_PIN);
  SimpleButton powerButton(POWER_BUTTON_PIN);
#else
  // Breadboard version of thermostat uses pulldowns. Replace with pullup code when remaking the circuit
  SimpleButton upButton(UP_BUTTON_PIN, true);
  SimpleButton downButton(DOWN_BUTTON_PIN, true);
  SimpleButton powerButton(POWER_BUTTON_PIN, true);
#endif

bool useFahrenheit = false;
bool showDebugInfo = false;

#if TEST_ENVIRONMENT
  Adafruit_SSD1306 matrix = Adafruit_SSD1306(128, 64);
#else
  Adafruit_8x16matrix matrix = Adafruit_8x16matrix();
#endif

#if QT_PY && HAS_NEOPIXEL
  Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, PIN_NEOPIXEL);
#endif

unsigned long setTempTime = 0;
unsigned long lastToggleTime = 0;
unsigned long georgeBoostTime = 0;

bool temperatureError = false;
bool didSetThermostatPowerOn = false;

bool oldPowerState = false;
float oldTemperature = 0;

bool isWifiConnected = false;
bool isInTimeoutError = false;

#define IS_ERROR_STATE (!isWifiConnected || isInTimeoutError)

unsigned long statusMessageTime = 0;
String statusMessage;
uint16_t statusMessageWidth, statusMessageHeight;

char str[8];

void showStatusMessage(String message)
{
  statusMessage = message;
  statusMessageTime = millis();

  int16_t  x1, y1;

  matrix.getTextBounds(statusMessage, 0, 1, &x1, &y1, &statusMessageWidth, &statusMessageHeight);
}

void tryToReconnect()
{
  isWifiConnected = false;
  
  Serial.println("Disconnected from WIFI access point");
  Serial.println("Reconnecting...");

  showStatusMessage("DISCONNECTED");

  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);  
}

#if NODE_MCU
  void onStationDisconnected(const WiFiEventStationModeDisconnected& evt) 
  {
    tryToReconnect();
  }
#elif QT_PY
  void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
  {
    tryToReconnect();
  }
#endif



void upButtonLongPress()
{
  useFahrenheit = !useFahrenheit;

  EEPROM.write(FAHRENHEIT_EEPROM_ADDR, (byte)useFahrenheit);
  EEPROM.commit();

  Serial.print("Use Farenheit: ");
  Serial.println(useFahrenheit);
}

void downButtonLongPress()
{
  showDebugInfo = !showDebugInfo;

  //thermostatController.SetSyslogMode(showDebugInfo);

  EEPROM.write(DEBUG_EEPROM_ADDR, (byte)showDebugInfo);
  EEPROM.commit();    
}

void powerButtonLongPress()
{
  thermostatController.SetLocalMode(!thermostatController.IsInLocalMode());

  if(thermostatController.IsInLocalMode())
  {
    showStatusMessage(MESSAGE_LOCAL);
  }
  else
  {
    showStatusMessage(MESSAGE_ONLINE);

    isWifiConnected = false;
  
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);      
  }

  EEPROM.write(LOCAL_EEPROM_ADDR, (byte)thermostatController.IsInLocalMode());
  EEPROM.commit();    
}

unsigned long lastReportTime = 0;
bool isInGeorgeBoostTime()
{
  // if((millis() / 1000) != lastReportTime)
  // {
  //   Serial.print("Millis: ");
  //   Serial.println(millis());
  //   Serial.print("George Boost Time: ");
  //   Serial.println(georgeBoostTime);
  //   Serial.print("Curent / Target: ");
  //   Serial.print(thermostatController.GetCurrentTemperature());
  //   Serial.print(" / ");
  //   Serial.println(thermostatController.GetTargetTemperature());

  //   Serial.print("Toggle in: ");
  //   Serial.println(max((unsigned long)0, MIN_TOGGLE_TIME - (millis() - lastToggleTime)));

  //   lastReportTime = millis() / 1000;
  // }


  return (georgeBoostTime > 0) && 
         (millis() - georgeBoostTime) < GEORGE_BOOST_TIME &&
         thermostatController.GetCurrentTemperature() < thermostatController.GetTargetTemperature();
}


bool shouldShowTargetTemperature()
{
  return millis() < setTempTime + TARGET_TEMP_DURATION;
}

unsigned long getStatusMessageTime()
{
  return millis() - statusMessageTime;
}

bool shouldShowStatusMessage()
{
  if(statusMessageTime == 0)
  {
    return false;
  }
  else if(statusMessageWidth > 16)
  {
    return getStatusMessageTime() < (unsigned long)(STATUS_MESSAGE_SCROLL_DELAY * 2 + statusMessageWidth * STATUS_MESSAGE_SCROLL_STEP / DISPLAY_SCALE);
  }
  else
  {
    return getStatusMessageTime() < STATUS_MESSAGE_DURATION;
  }    
}

void drawStatusMessage()
{
  int xOffset = 0;
  
  if(statusMessageWidth > matrix.width())
  {
    if(getStatusMessageTime() > STATUS_MESSAGE_SCROLL_DELAY)
    {
      xOffset = max(-statusMessageWidth+matrix.width(),-(int)(getStatusMessageTime() / STATUS_MESSAGE_SCROLL_STEP * DISPLAY_SCALE));
    } 
  }
  
  // Do not multiply xOffset by DISPLAY_SCALE because it already takes into account the scale
  matrix.setCursor(xOffset, 5 * DISPLAY_SCALE);
  matrix.print(statusMessage);
}

void drawPulseState()
{
  int totalWidth = 16;
  int blinking = (int)(millis() / 500) % 2;

  float t = (float)(millis() % HEATER_RELAY_WINDOW_SIZE) / HEATER_RELAY_WINDOW_SIZE;

  if (pid.getPulseValue() > 0.0f)
  {
    matrix.drawLine(0, 6 * DISPLAY_SCALE, pid.getPulseValue() * totalWidth * DISPLAY_SCALE, 6 * DISPLAY_SCALE, LED_ON);
  }

  if (pid.getPulseValue() < 1.0f)
  {
    matrix.drawLine(pid.getPulseValue() * totalWidth * DISPLAY_SCALE, 7 * DISPLAY_SCALE, totalWidth * DISPLAY_SCALE, 7 * DISPLAY_SCALE, LED_ON);
  }

  matrix.drawPixel(t * totalWidth * DISPLAY_SCALE, (heaterState ? 6 : 7)  * DISPLAY_SCALE, blinking);
}

#define SIN_01(x) ((sin(x) + 1.0) / 2.0)

void updateNeoPixel()
{
  #if QT_PY && HAS_NEOPIXEL
    if(IS_ERROR_STATE)
    {
      pixel.fill(pixel.Color(SIN_01(millis() / 200) * 255, 0, 0));
      pixel.show();
    }
    // else if(thermostatController.IsTelnetConnected())
    // {
    //   pixel.fill(pixel.Color(SIN_01(millis() / 500) * 255, SIN_01(millis() / 500) * 255, 255));
    //   pixel.show();      
    // }
    else if(isInGeorgeBoostTime())
    {
      pixel.rainbow((millis()*20) % 0xFFFF);
      pixel.show();
    }
    else
    {
      const float pulseTime = 500.0f;
      if(thermostatController.GetTimeSinceLastServerResponse() < pulseTime)
      {
        float t = (1.0f - (thermostatController.GetTimeSinceLastServerResponse() / pulseTime));
        pixel.fill(pixel.Color(64 * t, 64 * t, 0));
        pixel.show();
      }
      else
      {
        pixel.clear();
        pixel.show();
      }
    }
  #endif
}

void updateLED()
{
  matrix.setFont(&Thermostat_Font);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  matrix.setTextSize(DISPLAY_SCALE);
  matrix.setRotation(DISPLAY_ROTATION);

  #ifdef TEST_ENVIRONMENT
    matrix.clearDisplay();
  #else
    matrix.clear();
  #endif
  
  if(shouldShowStatusMessage())
  {
    #ifndef TEST_ENVIRONMENT
      matrix.setBrightness(TARGET_TEMP_BRIGHTNESS);
    #endif

    drawStatusMessage();
  }
  else
  {
    if (shouldShowTargetTemperature())
    {  
      #ifndef TEST_ENVIRONMENT
        matrix.setBrightness(TARGET_TEMP_BRIGHTNESS);
      #endif
  
      if (thermostatController.GetPowerState() == false)
      {
        sprintf(str, "OFF");
      }
      else
      {
        if (didSetThermostatPowerOn && millis() < setTempTime + POWER_ON_MSG_DURATION)
        {
          sprintf(str, "ON");
        }
        else
        {
          didSetThermostatPowerOn = false;
          if (useFahrenheit)
          {
            sprintf(str, "%df", TARGET_TEMPERATURE_F);
          }
          else
          {
            sprintf(str, "%dc", TARGET_TEMPERATURE_C);
          }       
        }
      }

      int x = 1;
      int y = 5;
      int16_t  x1, y1;
      uint16_t w, h;
      
      matrix.getTextBounds(str, x * DISPLAY_SCALE, y * DISPLAY_SCALE, &x1, &y1, &w, &h);
  
      matrix.setCursor(x * DISPLAY_SCALE, y * DISPLAY_SCALE);
      matrix.fillRect((x1-1),(y1-1),(w+2),(h+2),LED_ON);
      matrix.setTextColor(LED_OFF);
      matrix.print(str);         
    }
    else
    {
      #ifndef TEST_ENVIRONMENT
        matrix.setBrightness(CURRENT_TEMP_BRIGHTNESS);
      #endif

      if(thermostatController.IsInLocalMode())
      {
        if (useFahrenheit)
        {
          sprintf(str, "%dfL", CURRENT_TEMPERATURE_F);
        }
        else
        {
          sprintf(str, "%dcL", CURRENT_TEMPERATURE_C);
        }      
      }
      else
      {
        if (useFahrenheit)
        {
          sprintf(str, "%df", CURRENT_TEMPERATURE_F);
        }
        else
        {
          sprintf(str, "%dc", CURRENT_TEMPERATURE_C);
        }               
      }

      int x = 1;
      int y = 5;
      matrix.setCursor(x * DISPLAY_SCALE, y * DISPLAY_SCALE);
      matrix.setTextColor(LED_ON);
      matrix.print(str);      
    }

    if(isInGeorgeBoostTime())    
    {
        float timeRemaining = 1.0f - ((float)(millis() - georgeBoostTime) / GEORGE_BOOST_TIME);
        matrix.fillRect(matrix.width() - (3 * DISPLAY_SCALE),(matrix.height()-1 - floor(timeRemaining * matrix.height())),3 * DISPLAY_SCALE,ceil(timeRemaining * matrix.height()), LED_ON);
    }
    else
    {
        if (showDebugInfo && thermostatController.GetPowerState())
        {
          drawPulseState();
        }
    }
  }

  #ifndef QT_PY
    if(IS_ERROR_STATE )
    {
      matrix.drawPixel(0,0, LED_ON);
    }
  #endif

  #if TEST_ENVIRONMENT
    matrix.display();
  #else
    matrix.writeDisplay();
  #endif
}

void updateTemperature()
{

  if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL)
  {
    #if TEST_ENVIRONMENT
      float currentTemperature = ABSOLUTE_MIN_TEMP + (ABSOLUTE_MAX_TEMP - ABSOLUTE_MIN_TEMP) / 2;
    #else
      float currentTemperature = sensors.getTempCByIndex(0) + TEMPERATURE_ERROR_OFFSET;
    #endif

    if (currentTemperature > MIN_VALID_TEMP)
    {
      thermostatController.SetCurrentTemperature(currentTemperature);
      temperatureError = false;
    }
    else
    {
      if(temperatureError == false)
      {
        temperatureError = true;
        showStatusMessage(MESSAGE_TEMP_ERR);
      }
      
    }

    lastTemperatureUpdate = millis();
    sensors.requestTemperatures(); //request reading for next time
  }
}

void adjustTargetTemp(int delta)
{
  if (thermostatController.GetPowerState())
  {
    if(shouldShowTargetTemperature())
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

      if(delta > 0)
      {
        georgeBoostTime = millis();
      }
    }
    
    didSetThermostatPowerOn = false;
  }
  else
  {
      if (useFahrenheit)
      {
        float f = (thermostatController.GetCurrentTemperature() * 9 / 5) + 32;
        f += delta;
        thermostatController.SetTargetTemperature((f - 32) * 5 / 9);
      }
      else
      {
        int temp = min(ABSOLUTE_MAX_TEMP, max(ABSOLUTE_MIN_TEMP, thermostatController.GetCurrentTemperature() + delta));
        thermostatController.SetTargetTemperature(temp);  
      }
    
      if(delta > 0)
      {
        georgeBoostTime = millis();
      }

      thermostatController.SetPowerState(true);
      didSetThermostatPowerOn = true;
  }  

  setTempTime = millis();
}

void toggleThermostatPower()
{
  thermostatController.SetPowerState(!thermostatController.GetPowerState());
  didSetThermostatPowerOn = thermostatController.GetPowerState();

  setTempTime = millis();
}

void wakeUp()
{
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

  bool isInGeorgeBoost = false;

  // If the thermostat is unpowered, simply set the target to current. This prevents the relay from toggling too rapidly if a user spams the power
  if (thermostatController.GetPowerState() && !temperatureError)
  {
    target = (double)thermostatController.GetTargetTemperature();
    isInGeorgeBoost = isInGeorgeBoostTime();
  }
  else
  {
    target = (double)thermostatController.GetCurrentTemperature();
  }

  pid.run();

  if (millis() - lastToggleTime > MIN_TOGGLE_TIME)
  {
    toggleHeater(pidState || isInGeorgeBoost);  
  }
}

unsigned long getTimeLeftInRelayPulse()
{
  unsigned long timeInCurrentWindow = millis() % HEATER_RELAY_WINDOW_SIZE;
  unsigned long pulseTime = pid.getPulseValue() * HEATER_RELAY_WINDOW_SIZE;
  return pulseTime - timeInCurrentWindow;
}


void setup()
{
  Serial.begin(19200);
  delay(200);

  Serial.println("");
  Serial.println("");
  Serial.println("");
  Serial.print("Initializing Pins... ");
  
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);  
  heaterState = false;

  #if QT_PY && HAS_NEOPIXEL
    pixel.begin();
    pixel.fill(pixel.Color(255,255,0));
    pixel.show();
  #endif

  Serial.println("Done");
  Serial.print("Initializing LED Matrix... ");  

  #if TEST_ENVIRONMENT
    matrix.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    matrix.invertDisplay(false);
  #else
    matrix.begin(0x70);  // pass in the address
  #endif

  matrix.setFont(&Thermostat_Font);
  matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
  matrix.setTextColor(LED_ON);
  matrix.setTextSize(DISPLAY_SCALE);
  matrix.setRotation(DISPLAY_ROTATION);

  #ifndef TEST_ENVIRONMENT
    matrix.setBrightness(15);
    matrix.clear();
  #else
    matrix.clearDisplay();
  #endif

  matrix.setCursor(1 * DISPLAY_SCALE, 5 * DISPLAY_SCALE);
  matrix.print("HI!");

  #if TEST_ENVIRONMENT  
    matrix.display();
  #else
    matrix.writeDisplay();
  #endif

  Serial.println("Done");
  Serial.print("Initializing Buttons... ");  

  upButton.SetEndPressCallback([]()
  {
    adjustTargetTemp(1);
  });
  upButton.SetBeginPressCallback(wakeUp);
  upButton.SetHoldCallback(upButtonLongPress);
  
  downButton.SetEndPressCallback([]()
  {
    adjustTargetTemp(-1);
  });
  downButton.SetBeginPressCallback(wakeUp); 
  downButton.SetHoldCallback(downButtonLongPress);

  powerButton.SetEndPressCallback([]()
  {
    toggleThermostatPower();
  });
  powerButton.SetBeginPressCallback(wakeUp);   
  powerButton.SetHoldCallback(powerButtonLongPress);

  Serial.println("Done");

  sensors.requestTemperatures(); // Get initial temperature reading
  sensors.setWaitForConversion(false);

  Serial.print("Connecting to Wifi");  

#ifdef USE_STATIC_IP
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
#endif

  WiFi.mode(WIFI_STA);
  WiFi.hostname(F(HOSTNAME));
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  delay(200);
  int dots = 0;
  
  while (WiFi.status() != WL_CONNECTED)
  {
    #ifdef TEST_ENVIRONMENT
      matrix.clearDisplay();
    #else
      matrix.clear();
    #endif

    
    matrix.setCursor(0 * DISPLAY_SCALE, 5 * DISPLAY_SCALE);
    matrix.print("HI!");

    Serial.print(".");  

    for(int i = 0; i < dots % 4; i++) 
    {
      matrix.print(".");
    }

    dots++;
    
    #ifdef TEST_ENVIRONMENT
      matrix.display();
    #else
      matrix.writeDisplay();    
    #endif
    
    delay(500);
  }

  WiFi.setAutoReconnect(true);
  
  #if NODE_MCU
    WiFi.onStationModeDisconnected(&onStationDisconnected);
  #else
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  #endif
  
  isWifiConnected = false;

  Serial.print(" Done (");
  Serial.print(WiFi.localIP());
  Serial.println(")");

  Serial.println("Stored Settings:");  

  EEPROM.begin(3);
  useFahrenheit = boolean(EEPROM.read(FAHRENHEIT_EEPROM_ADDR));  
  showDebugInfo = boolean(EEPROM.read(DEBUG_EEPROM_ADDR));
  thermostatController.SetLocalMode(boolean(EEPROM.read(LOCAL_EEPROM_ADDR)));
  //thermostatController.SetSyslogMode(true);
  
  Serial.print(" > Use Fahrenheit: ");  
  Serial.println(useFahrenheit);  
  
  Serial.print(" > Show Debug Info: ");  
  Serial.println(showDebugInfo);  

  Serial.print(" > Local Mode: ");  
  Serial.println(thermostatController.IsInLocalMode());  

  Serial.print("Initializing OTA Update..."); 

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onEnd([]() 
  {
    matrix.setFont(&Thermostat_Font);
    matrix.setTextSize(DISPLAY_SCALE);
    matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
    matrix.setTextColor(LED_ON);

    #ifdef TEST_ENVIRONMENT
      matrix.clearDisplay();
    #else
      matrix.clear();
    #endif


    matrix.fillRect(0,0,16 * DISPLAY_SCALE,8 * DISPLAY_SCALE,LED_ON);
  
    #ifdef TEST_ENVIRONMENT
      matrix.display();
    #else
      matrix.writeDisplay();  
    #endif    
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    matrix.setFont(&Thermostat_Font);
    matrix.setTextWrap(false);  // we dont want text to wrap so it scrolls nicely
    matrix.setTextSize(DISPLAY_SCALE);
    matrix.setTextColor(LED_ON);

    #ifdef TEST_ENVIRONMENT
      matrix.clearDisplay();
    #else
      matrix.clear();
    #endif

    int barWidth = ((float)progress/(float)total) * matrix.width();
    if(barWidth > 0)
    {
      matrix.fillRect(0,0,barWidth,matrix.height(),LED_ON);
    }
  
    #ifdef TEST_ENVIRONMENT
      matrix.display();
    #else
      matrix.writeDisplay();
    #endif  
    
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  
  ArduinoOTA.onError([](ota_error_t error) 
  {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  
  ArduinoOTA.begin();

  Serial.println("Done"); 

  #if QT_PY  && HAS_NEOPIXEL
    pixel.fill(pixel.Color(0,255,0));
    pixel.show();

    delay(200);
  #endif

}

void loop()
{
  upButton.Update();
  downButton.Update();
  powerButton.Update();

  updateTemperature();
  updateHeaterController();

  // wait for WiFi connection
  if (WiFi.status() == WL_CONNECTED)
  {
    ArduinoOTA.handle();
    
    // General checking for an error state
    if(thermostatController.GetTimeSinceLastServerResponse() > SERVER_TIMEOUT_TIME)
    {
      if(!isInTimeoutError)
      {
        isInTimeoutError = true;
        showStatusMessage(MESSAGE_TIMEOUT_ERR);
      }
    }
    else
    {
      if(isInTimeoutError)
      {
        isInTimeoutError = false;
        showStatusMessage(MESSAGE_RECONNECTED);
      }      
    }
    
    if(!isWifiConnected)
    {
      isWifiConnected = true;
      showStatusMessage(MESSAGE_RECONNECTED);
    }

    if (thermostatController.WasPowerSetRemotely())
    {
      setTempTime = millis();
      didSetThermostatPowerOn = true;
    }
    else if (thermostatController.WasTemperatureSetRemotely())
    {
      setTempTime = millis();

      if(thermostatController.GetRemoteTemperatureChangeDelta() > 0)
      {
        georgeBoostTime =  millis();
      }
      
    }

    thermostatController.Update();
  }
  else
  {
    if(isWifiConnected)
    {
      isWifiConnected = false;
      showStatusMessage(MESSAGE_WIFI);
    }
  }

  updateLED();
  updateNeoPixel();

  //delay(5);
}

