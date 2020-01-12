#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <AutoPID.h>

#include "Configuration.h"
/*
 * Configuration.h is where the following is defined:
 * API Key for accessing the remote server.
 * Name of the thermostat (This will be used if controlling multiple thermostats is ever supported).
 * Wifi SSID and password.
 */

#include "RemoteThermostatController.h"

#define USE_SERIAL Serial

#define GET_DATA_URL "http://temp.joegatling.com/get-thermostat-data.php"
#define SET_CURRENT_TEMPERATURE_URL "http://joegatling.com/sites/temperature/set-current-temperature.php"

#define SERVER_POLL_INTERVAL 10000
#define TEMPERATURE_POLL_INTERVAL 800

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 9

unsigned long lastTemperatureUpdate = 0;
unsigned long lastServerUpdate = 0;

StaticJsonDocument<400> jsonDocument;
JsonObject jsonObject;

struct temperatureInfo
{
  float celsius;  
  String timestamp;  
};

temperatureInfo currentTemperature;
temperatureInfo targetTemperature;

//RemoteThermostatController thermostatController;

// Setup a oneWire instance to communicate with any OneWire devices 
// (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

String getDataUrl;

void setup() 
{
  USE_SERIAL.begin(115200);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    USE_SERIAL.print(".");
  }

  //thermostatController = new RemoteThermostatController(API_KEY, THERMOSTAT_NAME, false); 

  // Calculate our server request URL once, since it never changes
  getDataUrl = String(GET_DATA_URL);
  getDataUrl.concat("?key=");
  getDataUrl.concat(API_KEY);
}

void loop() 
{
  updateTemperature();
  updateHeaterController();
  
  // wait for WiFi connection
  if (WiFi.status() == WL_CONNECTED) 
  {
    if((millis() - lastServerUpdate) > SERVER_POLL_INTERVAL)
    {
      getInfoFromServer();    
      postInfoToServer();
      
      lastServerUpdate = millis();
    }
  }

  delay(SERVER_POLL_INTERVAL);
}

void updateTemperature()
{
  if ((millis() - lastTemperatureUpdate) > TEMPERATURE_POLL_INTERVAL) 
  {
    currentTemperature.celsius = sensors.getTempCByIndex(0);

    USE_SERIAL.print("Detected temperature is ");
    USE_SERIAL.println(currentTemperature.celsius);
    USE_SERIAL.println("");
    
    lastTemperatureUpdate = millis();
    sensors.requestTemperatures(); //request reading for next time  
  }
}

void updateHeaterController()
{
  // To do
}

void postInfoToServer()
{  
  WiFiClient client;
  HTTPClient http;

  // configure traged server and url
  String url = String(SET_CURRENT_TEMPERATURE_URL);
  url.concat("?key=");
  url.concat(API_KEY);
  url.concat("&c=");
  url.concat(currentTemperature.celsius);

  USE_SERIAL.print("URL: ");
  USE_SERIAL.println(url);
  
  http.begin(client, url); //HTTP
  //http.addHeader("Content-Type", "application/json");

  //start connection and send HTTP header and body
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) 
  {
    // HTTP header has been send and Server response header has been handled
    //USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) 
    {
      const String& payload = http.getString();
      
      USE_SERIAL.print("received response: ");
      USE_SERIAL.println(payload);
    }
  } 
  else 
  {
    USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();   

  USE_SERIAL.println("");
}

void getInfoFromServer()
{
  WiFiClient client;
  HTTPClient http;

  USE_SERIAL.print("URL: ");
  USE_SERIAL.println(getDataUrl);

  // configure traged server and url
  http.begin(client, getDataUrl); //HTTP
  http.addHeader("Content-Type", "application/json");

  // start connection and send HTTP header and body
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) 
  {
    // HTTP header has been send and Server response header has been handled
    //USE_SERIAL.printf("[HTTP] POST... code: %d\n", httpCode);

    // file found at server
    if (httpCode == HTTP_CODE_OK) 
    {
      const String& payload = http.getString();
      
      USE_SERIAL.println("received payload:");
      USE_SERIAL.print(payload);

      auto error = deserializeJson(jsonDocument, payload);
      
      if(!error)
      {
        jsonObject = jsonDocument.as<JsonObject>();

        // This thermostat is the authority on what the current temperature is, so no need to 
        // read the temperature coming from the server
//        if(jsonObject["current"].containsKey("celsius"))
//        {
//          currentTemperature.celsius = float(jsonObject["current"]["celsius"]);
//          USE_SERIAL.print("Current Temperature: ");
//          USE_SERIAL.println(currentTemperature.celsius);
//        }

        if(jsonObject["target"].containsKey("celsius"))
        {
          targetTemperature.celsius = float(jsonObject["target"]["celsius"]);
          USE_SERIAL.print("Target Temperature: ");
          USE_SERIAL.println(targetTemperature.celsius);
       }          
      }
    }
  } 
  else 
  {
    USE_SERIAL.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  USE_SERIAL.println("");

  http.end();  
}
