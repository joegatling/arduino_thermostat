#include <Arduino.h>
#include <EEPROM.h>
#include <ArduinoOTA.h>
#include <stdio.h>
#include <ESPmDNS.h>
#include <WiFiManager.h>
#include <Preferences.h>

#include "Thermostat.h"
#include "LedController.h"
#include "ButtonController.h"
#include "MqttController.h"

#define PREFERENCES_NAMESPACE "jg"

#define PREFERENCES_MQTT_SERVER "m_server"
#define PREFERENCES_MQTT_PORT "m_port"
#define PREFERENCES_MQTT_USER "m_user"
#define PREFERENCES_MQTT_PASSWORD "m_pass"

#define PREFERENCES_TEMPERATURE_UNIT "unit"

#define PREFERENCES_FORCE_CONFIG_NEXT_BOOT "config"

#define CONFIG_MODE_TIMEOUT 5 // seconds
#define CONFIG_PORTAL_TIMEOUT 180 // seconds



#include "Configuration.h"

#define FAHRENHEIT_EEPROM_ADDR        0
#define DEBUG_EEPROM_ADDR             1
#define LOCAL_EEPROM_ADDR             2

Preferences preferences;

Thermostat thermostatController;
LedController ledController;
ButtonController buttonController;
MqttController mqttController;


String mqtt_server = DEFAULT_MQTT_SERVER; 
int mqtt_port = DEFAULT_MQTT_PORT;
String mqtt_password = DEFAULT_MQTT_PASSWORD; 
String mqtt_user = DEFAULT_MQTT_USER;

bool shouldEnterConfigMode = false;
bool shouldSaveConfig = false;
bool useFahrenheit = false;

void configModeCallback(WiFiManager *myWiFiManager) 
{
  // Show some kind of message
  ledController.showStatusMessage("CFG", false, true);
  ledController.setLightColor(255,255,255);

}

void saveConfigCallback() 
{
  Serial.println("Saved config");
  shouldSaveConfig = true;
}

void configModeShortcutPressed()
{
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putBool(PREFERENCES_FORCE_CONFIG_NEXT_BOOT, true);
  preferences.end();  

  // Reboot the esp32
  ESP.restart();
}

void tryToReconnect()
{
  Serial.println("Disconnected from WIFI access point");
  Serial.println("Reconnecting...");

  ledController.showStatusMessage("CON?");

  WiFi.disconnect();
  WiFi.begin();
}

void onThermostatUnitChanged(bool unitChangedToFahrenheit)
{
  useFahrenheit = unitChangedToFahrenheit;

  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.putBool(PREFERENCES_TEMPERATURE_UNIT, useFahrenheit);
  preferences.end();

  Serial.print("Thermostat unit changed to ");
  Serial.println(useFahrenheit ? "Fahrenheit" : "Celsius"); 

  preferences.begin(PREFERENCES_NAMESPACE, true);
  Serial.print("Test: ");
  Serial.println(preferences.getBool(PREFERENCES_TEMPERATURE_UNIT, false));
  preferences.end();  
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  tryToReconnect();
}

void setup()
{
  Serial.begin(19200);

  for(int i = 0; i < 20; i++)
  {
    Serial.print(">");
    delay(50);
  }
  Serial.println();

  Serial.println("Initializing Saved Data... ");

  preferences.begin(PREFERENCES_NAMESPACE, true);

  mqtt_server = preferences.getString(PREFERENCES_MQTT_SERVER, DEFAULT_MQTT_SERVER);
  mqtt_port = preferences.getInt(PREFERENCES_MQTT_PORT, DEFAULT_MQTT_PORT);
  mqtt_user = preferences.getString(PREFERENCES_MQTT_USER, DEFAULT_MQTT_USER);    
  mqtt_password = preferences.getString(PREFERENCES_MQTT_PASSWORD, DEFAULT_MQTT_PASSWORD);
  
  shouldEnterConfigMode = preferences.getBool(PREFERENCES_FORCE_CONFIG_NEXT_BOOT, false);
  useFahrenheit = preferences.getBool(PREFERENCES_TEMPERATURE_UNIT, false);

  preferences.end();

  Serial.print(" > MQTT Server: ");  
  Serial.println(mqtt_server);  
  Serial.print(" > MQTT Port: ");  
  Serial.println(mqtt_port);  
  
  Serial.print(" > MQTT User: ");  
  Serial.println(mqtt_user);  
  Serial.print(" > MQTT Password: ");  
  Serial.println(mqtt_password);  

  Serial.print(" > Force Config Next Boot: ");  
  Serial.println(shouldEnterConfigMode);  

  Serial.print(" > Use Fahrenheit: ");  
  Serial.println(useFahrenheit);  
  
  Serial.println("Done");

  Serial.print("Initializing Thermostat... ");
  thermostatController.setUsingFahrenheit(useFahrenheit);
  thermostatController.setMode(OFF);
  thermostatController.onUseFahrenheitChanged(onThermostatUnitChanged);
  Serial.println("Done");

  Serial.print("Initializing LED Matrix... ");  
  ledController.initialize();
  ledController.setThermostat(&thermostatController);
  Serial.println("Done");

  Serial.print("Initializing Buttons... ");
  buttonController.setThermostat(&thermostatController);
  buttonController.setConfigModeCallback(configModeShortcutPressed);
  Serial.println("Done");

  Serial.println("Initializing WiFi... ");
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  WiFiManager wm;
  wm.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wm.setAPCallback(configModeCallback);
  wm.setSaveConfigCallback(saveConfigCallback);
  wm.setDebugOutput(true);

  String hostString = String(WIFI_getChipId(),HEX);
  hostString.toUpperCase();

  WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT server", mqtt_server.c_str(), 40);
  WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT port", String(mqtt_port).c_str(), 6);
  WiFiManagerParameter custom_mqtt_user("mqtt_user", "MQTT user", mqtt_user.c_str(), 40);
  WiFiManagerParameter custom_mqtt_password("mqtt_password", "MQTT password", mqtt_password.c_str(), 40);
  WiFiManagerParameter custom_use_fahrenheit("use_fahrenheit", "Use Fahrenheit (1 = yes, 0 = no)", useFahrenheit ? "1" : "0", 2);
  
  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_password);
  wm.addParameter(&custom_use_fahrenheit);

  if(shouldEnterConfigMode) 
  {
    Serial.println("Forcing config mode");
  
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putBool(PREFERENCES_FORCE_CONFIG_NEXT_BOOT, false);
    preferences.end();        
  
    wm.startConfigPortal((FALLBACK_SSID + hostString).c_str(), FALLBACK_PSK);

    Serial.println("Returned from config portal");
  }
  else
  {
    wm.autoConnect((FALLBACK_SSID + hostString).c_str(), FALLBACK_PSK); // password protected ap

  }


  //Check if we connected to WiFi
  bool result = (WiFi.status() == WL_CONNECTED);

  if(!result) {
      Serial.println("Failed to connect");

      // Stop forever
      while(1) {}
  } 

  WiFi.hostname(F(HOSTNAME));
  WiFi.setAutoReconnect(true);
  
  #if NODE_MCU
    WiFi.onStationModeDisconnected(&onStationDisconnected);
  #else
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  #endif

  Serial.print(" Done (");
  Serial.print(WiFi.localIP());
  Serial.println(")");

  Serial.print("Initializing MDNS [");
  Serial.print(F(HOSTNAME));
  Serial.print("]... ");
  if (!MDNS.begin(F(HOSTNAME))) 
  {
    Serial.println("Error");
  } 
  else 
  {
    Serial.println("Done");
    // MDNS.addService("http", "tcp", 80); 
    // Serial.println("TCP service added.");
  }    

  mqtt_server = custom_mqtt_server.getValue();
  mqtt_port = atoi(custom_mqtt_port.getValue());    
  mqtt_user = custom_mqtt_user.getValue();   
  mqtt_password = custom_mqtt_password.getValue();
  useFahrenheit = (atoi(custom_use_fahrenheit.getValue()) == 1);
  
  if(shouldSaveConfig)
  {
    Serial.print("Saving config...");

    Serial.print(" > MQTT Server: ");  
    Serial.println(mqtt_server);  
    Serial.print(" > MQTT Port: ");  
    Serial.println(mqtt_port);  
    
    Serial.print(" > MQTT User: ");  
    Serial.println(mqtt_user);  
    Serial.print(" > MQTT Password: ");  
    Serial.println(mqtt_password);  

    Serial.print(" > Force Config Next Boot: ");  
    Serial.println(shouldEnterConfigMode);  

    Serial.print(" > Use Fahrenheit: ");  
    Serial.println(useFahrenheit);      

    preferences.begin(PREFERENCES_NAMESPACE, false);

    preferences.putString(PREFERENCES_MQTT_SERVER, mqtt_server);
    preferences.putInt(PREFERENCES_MQTT_PORT, mqtt_port);
    preferences.putString(PREFERENCES_MQTT_USER, mqtt_user);  
    preferences.putString(PREFERENCES_MQTT_PASSWORD, mqtt_password);  

    preferences.putBool(PREFERENCES_TEMPERATURE_UNIT, useFahrenheit);
    preferences.putBool(PREFERENCES_FORCE_CONFIG_NEXT_BOOT, false);
      
    preferences.end();

    Serial.println("Done");
  }

  Serial.print("Initializing MQTT...");
  mqttController.setThermostat(&thermostatController);
  mqttController.setDeviceName(THERMOSTAT_NAME);
  mqttController.setConnectionInfo(mqtt_server, mqtt_port, mqtt_user, mqtt_password);
  Serial.println("Done");

  Serial.print("Initializing OTA Update..."); 

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onEnd([]() 
  {
    ledController.showProgressBar(1.0f);
    Serial.printf("OTA Update Complete");

  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) 
  {
    ledController.showProgressBar((float)progress / (float)total);
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
}

void loop()
{
  thermostatController.update();
  ledController.update();
  buttonController.update();

  if (WiFi.status() == WL_CONNECTED)
  {
    ArduinoOTA.handle();
    mqttController.update();
  }
}

