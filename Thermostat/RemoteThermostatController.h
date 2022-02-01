#pragma once

#include <AsyncHTTPRequest_Generic.hpp>  

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h> 

#include <WiFiUdp.h>
#include <Syslog.h>

#define SERIAL_OUTPUT Serial
//#define JSON_SIZE 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 248
#define JSON_SIZE 512

#define REQUEST_TIMEOUT 100000


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
    bool GetPowerState();

    bool WasTemperatureSetRemotely() { return _wasTemperatureSetRemotely; }
    bool WasPowerSetRemotely() { return _wasPowerSetRemotely; }    

    bool IsInLocalMode() { return _isInLocalMode; }
    void SetLocalMode(bool isLocalMode) { _isInLocalMode = isLocalMode; }

    float GetRemoteTemperatureChangeDelta() { return _remoteTemperatureChangeDelta; }

    unsigned long GetTimeSinceLastServerResponse() { return (_lastServerResponse == 0 || millis() < _lastServerResponse) ? 0 : millis() - _lastServerResponse; }  

    void SetSyslogMode(bool isSyslogOn) { _isSyslogOn = isSyslogOn; }
    bool GetSyslogMode() { return _isSyslogOn; }

    bool GetErrorState() { return _request.responseHTTPcode() < 0; }
      
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
    unsigned long _lastServerResponse = 0;

    bool _isCurrentTemperatureSetLocally = false;
    bool _isTargetTemperatureSetLocally = false;

    bool _shouldUseRemoteTemperature = true;

    bool _shouldSendCurrentTemperature = false;
    bool _shouldSendTargetTemperature = false;
    bool _shouldGetData = false;

    bool _wasTemperatureSetRemotely = false;
    float _remoteTemperatureChangeDelta = 0;
    bool _wasPowerSetRemotely = false;

    bool _isInLocalMode = false;
    bool _isSyslogOn = false;
    
    StaticJsonDocument<JSON_SIZE> _jsonDocument;
    JsonObject _jsonObject;

    AsyncHTTPRequest _request;
    ServerRequestType _currentRequestType;

    void OnRequestReadyStateChanged(void* optParm, AsyncHTTPRequest* request, int readyState);
    void AsyncRequestResponseSendCurrentTemperature();
    void AsyncRequestResponseSetTargetTemperature();
    void AsyncRequestResponseGetData();

    void SendCurrentTemperatureToServer();
    void SendTargetTemperatureToServer();    
    
    void GetDataFromServer();

    bool IsRequestInProgress() { return _currentRequestType != NO_REQUEST; }
};