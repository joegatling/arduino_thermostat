#ifndef REMOTE_THERMOSTAT_CONTROLLER_H
#define REMOTE_THERMOSTAT_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>
#include <asyncHTTPrequest.h>

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

    bool _shouldSendCurrentTemperature = false;
    bool _shouldSendTargetTemperature = false;
    bool _shouldGetData = false;

    StaticJsonDocument<400> _jsonDocument;
    JsonObject _jsonObject;

    asyncHTTPrequest _request;
    bool _isRequestActive = false;

    void AsyncRequestResponseSetCurrentTemperature();
    void AsyncRequestResponseSetTargetTemperature();
    void AsyncRequestResponseGetData();


    void SendCurrentTemperatureToServer();
    void SendTargetTemperatureToServer();    
    
    void GetDataFromServer();

    bool IsRequestInProgress() { return _isRequestActive; }

};
#endif
