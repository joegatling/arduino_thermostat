#ifndef REMOTE_THERMOSTAT_CONTROLLER_H
#define REMOTE_THERMOSTAT_CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h> 
#include <ESP8266HTTPClient.h>
#include <asyncHTTPrequest.h>

#define SERIAL_OUTPUT Serial
//#define JSON_SIZE 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 248
#define JSON_SIZE 512

enum ServerRequestType
{
  NO_REQUEST,
  SEND_CURRENT_TEMPERATURE,
  SET_TARGET_TEMPERATURE,
  GET_DATA
};

class RemoteThermostatController
{
  public:
  
    RemoteThermostatController(String key, String thermostatName, bool useRemoteTemperature);

    void Update();
    
    void SetCurrentTemperature(float celsius);
    float GetCurrentTemperature();

    void SetTargetTemperature(float celsius);
    float GetTargetTemperature();

    void SetPowerState(bool isOn);
    boolean GetPowerState();
      
  private: 


    String _apiKey = "";
    String _thermostat = "default";
    String _getDataUrl = "";

    float _currentTemperature = 0;
    float _targetTemperature = 0;

    bool _isThermostatOn = true;

    float _maxTemp = 30.0f;
    float _minTemp = 16.0f;

    unsigned long _lastServerUpdate = 0;

    bool _isCurrentTemperatureSetLocally = false;
    bool _isTargetTemperatureSetLocally = false;

    bool _shouldUseRemoteTemperature = true;

    bool _shouldSendCurrentTemperature = false;
    bool _shouldSendTargetTemperature = false;
    bool _shouldGetData = false;

    
    StaticJsonDocument<JSON_SIZE> _jsonDocument;
    JsonObject _jsonObject;

    asyncHTTPrequest _request;
    //bool _isRequestActive = false;
    ServerRequestType _currentRequestType;

    void OnRequestReadyStateChanged(void* optParm, asyncHTTPrequest* request, int readyState);
    void AsyncRequestResponseSendCurrentTemperature();
    void AsyncRequestResponseSetTargetTemperature();
    void AsyncRequestResponseGetData();


    void SendCurrentTemperatureToServer();
    void SendTargetTemperatureToServer();    
    
    void GetDataFromServer();

    bool IsRequestInProgress() { return _currentRequestType != NO_REQUEST; }

};
#endif
