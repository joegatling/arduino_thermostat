#pragma once

#include <AsyncHTTPRequest_Generic.hpp>  

#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>

#if NODE_MCU
#include <ESP8266WiFi.h> 
#else
#include <WiFi.h>
#endif

#include <WiFiUdp.h>
#include "WebLogger.h"

#define SERIAL_OUTPUT Serial
//#define JSON_SIZE 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + 248
#define JSON_SIZE 512

#define REQUEST_TIMEOUT 100000

//#define TELNET_PORT 23


enum ServerRequestType
{
  NO_REQUEST,
  SYNC_DATA
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
    bool GetIfLastServerResponseWasValid() { return _lastServerResponseWasValid; }

    // void SetSyslogMode(bool isSyslogOn) { _isSyslogOn = isSyslogOn; }
    // bool GetSyslogMode() { return _isSyslogOn; }

    bool GetErrorState() { return _request.responseHTTPcode() < 0; }
    //bool IsTelnetConnected() { return _telnet.isConnected(); }
      
  private: 
    static RemoteThermostatController* __instance;

    String _apiKey = "";
    String _thermostat = "default";
    String _syncDataUrl = "";

    float _currentTemperature = 0;
    float _targetTemperature = 0;

    bool _isThermostatOn = true;

    float _maxTemp = 30.0f;
    float _minTemp = 16.0f;

    unsigned long _lastServerResponse = 0;
    bool _lastServerResponseWasValid = true;

    bool _isCurrentTemperatureSetLocally = false;
    bool _isTargetTemperatureSetLocally = false;

    bool _hasSentCurrentTemperature = false;
    bool _hasSentTargetTemperature = false;

    bool _shouldUseRemoteTemperature = true;

    bool _wasTemperatureSetRemotely = false;
    float _remoteTemperatureChangeDelta = 0;
    bool _wasPowerSetRemotely = false;

    bool _isInLocalMode = false;
    //bool _isSyslogOn = false;
    
    //ESPTelnet _telnet;

    StaticJsonDocument<JSON_SIZE> _jsonDocument;
    JsonObject _jsonObject;

    AsyncHTTPRequest _request;
    ServerRequestType _currentRequestType;

    void OnRequestReadyStateChanged(void* optParm, AsyncHTTPRequest* request, int readyState);
    void AsyncRequestResponseSyncData();
    
    void SyncDataWithServer();

    bool IsRequestInProgress() { return _currentRequestType != NO_REQUEST; }


    // void SetupTelnet();

    // void OnTelnetConnect(String ip);
    // void OnTelnetDisconnect(String ip);
    // void OnTelnetReconnect(String ip);
    // void OnTelnetConnectionAttempt(String ip);
    // void OnTelnetInput(String str);    

    // static void OnTelnetConnectWrapper(String ip);
    // static void OnTelnetDisconnectWrapper(String ip);
    // static void OnTelnetReconnectWrapper(String ip);
    // static void OnTelnetConnectionAttemptWrapper(String ip);
    // static void OnTelnetInputWrapper(String str);        
};
