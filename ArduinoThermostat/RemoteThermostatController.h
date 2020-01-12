#ifndef REMOTE_THERMOSTAT_CONTROLLER_H
#define REMOTE_THERMOSTAT_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>

#define SERIAL_OUPUT Serial

class RemoteThermostatController
{
  public:
  
    RemoteThermostatController(String key, String thermostatName, bool useRemoteTemperature);

    void Update();
    
    void SetCurrentTemperature(float celsius);
    float GetCurrentTemperature();

    void SetTargetTemperature(float celsius);
    float GetTargetTemperature();
      
  private: 
    String _apiKey = "";
    String _thermostat = "default";
    String _getDataUrl = "";

    float _currentTemperature = 0;
    float _targetTemperature = 0;

    unsigned long _lastServerUpdate = 0;

    bool _isCurrentTemperatureSetLocally = false;
    bool _isTargetTemperatureSetLocally = false;

    bool _shouldUseRemoteTemperature = true;

    StaticJsonDocument<400> _jsonDocument;
    JsonObject _jsonObject;

    void SendCurrentTemperatureToServer();
    void SendTargetTemperatureToServer();    
    void GetDataFromServer();
};
#endif
