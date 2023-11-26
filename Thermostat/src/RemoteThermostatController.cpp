#include "RemoteThermostatController.h"

#define HOST "http://joegatling.com"
#define SYNC_DATA_URL HOST "/sites/temperature/sync.php"

#define MIN_SERVER_SYNC_INTERVAL 2000
#define MAX_SERVER_SYNC_INTERVAL 10000

#define DEVICE_HOSTNAME "thermostat"
#define APP_NAME "sync"

#define SYSLOG_SERVER "255.255.255.255"
#define SYSLOG_PORT 514

WiFiUDP udpClient;

// Create a new syslog instance with LOG_KERN facility
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN);


RemoteThermostatController::RemoteThermostatController(String key, String thermostatName, bool useRemoteTemperature)
{  
  _apiKey = key;
  _thermostat = thermostatName;
  _shouldUseRemoteTemperature = useRemoteTemperature;

  _syncDataUrl = String(SYNC_DATA_URL);
  _syncDataUrl.concat(F("?key="));
  _syncDataUrl.concat(_apiKey);
  _syncDataUrl.concat(F("&thermostat="));
  _syncDataUrl.concat(_thermostat);

  _request.setDebug(false);
  _request.onReadyStateChange([=](void* optParm, AsyncHTTPRequest* request, int readyState)
  {
    OnRequestReadyStateChanged(optParm, request, readyState);
  });

  _currentRequestType = NO_REQUEST;
  
  _isCurrentTemperatureSetLocally = false;
  _isTargetTemperatureSetLocally = false; 

  _hasSentCurrentTemperature = false;
  _hasSentTargetTemperature = false;

  // Assume the thermostat is off at first. This will get replaced by server data once it arrives.
  _isThermostatOn = false;

  SyncDataWithServer();
}

void RemoteThermostatController::Update()
{
  _wasTemperatureSetRemotely = false;
  _wasPowerSetRemotely = false;
  _remoteTemperatureChangeDelta = 0;
  
  if(!IsRequestInProgress())
  {
    if((millis() - _lastServerResponse) > MIN_SERVER_SYNC_INTERVAL)
    {
      if(_isCurrentTemperatureSetLocally || _isTargetTemperatureSetLocally)
      {
        SyncDataWithServer();
      }
      else if((millis() - _lastServerResponse) > MAX_SERVER_SYNC_INTERVAL)
      {
        SyncDataWithServer();
      }
    }    
  }
  else
  {
    // Abandon the request if it times too long
    if(_request.elapsedTime() > REQUEST_TIMEOUT)
    {
      syslog.log(LOG_ERR, "Request Timeout");
            
      _currentRequestType = NO_REQUEST;
      _request.abort();      
    }
  }
}
    
void RemoteThermostatController::SetCurrentTemperature(float celsius)
{
  if(abs(_currentTemperature - celsius) > 0.05f)
  {
    _currentTemperature = celsius;
    _isCurrentTemperatureSetLocally = true;
  }
}

float RemoteThermostatController::GetCurrentTemperature()
{
  return _currentTemperature;
}

void RemoteThermostatController::SetTargetTemperature(float celsius)
{
  _targetTemperature = min(_maxTemp, max(_minTemp, celsius));
  _isTargetTemperatureSetLocally = true;
}

float RemoteThermostatController::GetTargetTemperature()
{
  return _targetTemperature;
}

void RemoteThermostatController::SetPowerState(bool isOn)
{
  _isTargetTemperatureSetLocally = (isOn != _isThermostatOn);
  _isThermostatOn = isOn;
}

boolean RemoteThermostatController::GetPowerState()
{
  return _isThermostatOn;
}
  

void RemoteThermostatController::SyncDataWithServer()
{
  if(!IsRequestInProgress())
  {
    _currentRequestType = SYNC_DATA;

    String url = String(_syncDataUrl);

    if(_isCurrentTemperatureSetLocally)
    {
      url.concat(F("&current_c="));
      url.concat(_currentTemperature);    
      
      //_isCurrentTemperatureSetLocally = false;
      _hasSentCurrentTemperature = true;
    }    
    
    if(_isTargetTemperatureSetLocally)
    {
      url.concat(F("&target_c="));
      url.concat(_targetTemperature);      
      url.concat(F("&power="));
      url.concat(_isThermostatOn); 
  
      //_isTargetTemperatureSetLocally = false;  
      _hasSentTargetTemperature = true;   
    }
    
    _request.open("GET", url.c_str());
    _request.send();   

    SERIAL_OUTPUT.println(F("Sync Data"));        
    if(_isSyslogOn)
    {
      syslog.log(LOG_DEBUG, "Syncing data with server."); 
      syslog.log(LOG_DEBUG, url.c_str()); 
    }     
  }
}

void RemoteThermostatController::OnRequestReadyStateChanged(void* optParm, AsyncHTTPRequest* request, int readyState)
{
  
  if(readyState == 4)
  {    
    if(_currentRequestType == SYNC_DATA)
    {
      AsyncRequestResponseSyncData();
    }   

    _lastServerResponse = millis();
     _currentRequestType = NO_REQUEST;
  }
}

void RemoteThermostatController::AsyncRequestResponseSyncData()
{
  if(_request.responseHTTPcode() == 200)
  {
    const String& payload = _request.responseText();
    
    SERIAL_OUTPUT.println(F("Received Thermostat Data:"));
    SERIAL_OUTPUT.println(payload);

    if(_isSyslogOn)
    {
      syslog.log(LOG_DEBUG, "Receieved Thermostat Data:"); 
      syslog.log(LOG_DEBUG, payload);               
    }
    else
    {     
      SERIAL_OUTPUT.println(F("No Syslog")); 
    }
    
    auto error = deserializeJson(_jsonDocument, payload);
    
    if(!error)
    {
      _jsonObject = _jsonDocument.as<JsonObject>();
    
      if(_shouldUseRemoteTemperature && !_isCurrentTemperatureSetLocally && _jsonObject[F("current")].containsKey("celsius"))
      {        
        _currentTemperature = float(_jsonObject[F("current")][F("celsius")]);

        SERIAL_OUTPUT.print("Current Temperature: ");
        SERIAL_OUTPUT.println(_currentTemperature);        
      }
    
      if(!_isTargetTemperatureSetLocally)
      {
        if(_jsonObject[F("target")].containsKey(F("celsius")))
        {
          float oldTargetTemperature = _targetTemperature;
          _targetTemperature = float(_jsonObject[F("target")][F("celsius")]);
          float delta = _targetTemperature - oldTargetTemperature;

          if(delta > 0.01 || delta < -0.01)
          {
            _wasTemperatureSetRemotely = true;

            _remoteTemperatureChangeDelta = delta;
          }

          SERIAL_OUTPUT.print("Target Temperature: ");
          SERIAL_OUTPUT.println(_targetTemperature);
        }

        if(_jsonObject[F("target")].containsKey(F("power")))
        {
          bool wasOn = _isThermostatOn;
          _isThermostatOn = _jsonObject[F("target")]["power"].as<int>() > 0;

          if(wasOn != _isThermostatOn)
          {
            _wasPowerSetRemotely = true;
          }

          SERIAL_OUTPUT.print("Is Thermostat On: ");
          SERIAL_OUTPUT.println(_isThermostatOn  );          
        }        
      }

      if(_hasSentTargetTemperature)
      {
        _isTargetTemperatureSetLocally = false;
        _hasSentTargetTemperature = false;
      }
      
      if(_hasSentCurrentTemperature)
      {
        _isCurrentTemperatureSetLocally = false;
        _hasSentCurrentTemperature = false;
      }

      if(_jsonObject.containsKey("thermostat"))
      {
        if(_jsonObject["thermostat"].containsKey("max_temp"))
        {
          _maxTemp = _jsonObject["thermostat"]["max_temp"];
        }
        else if(_jsonObject["thermostat"].containsKey("min_temp"))
        {
          _minTemp = _jsonObject["thermostat"][F("min_temp")];
        }   
      }   
    }
    else
    {
      SERIAL_OUTPUT.print(F("deserializeJson() failed: "));
      SERIAL_OUTPUT.println(error.c_str());      
    }
  }
  else
  {    
    if(_isSyslogOn)
    {
      syslog.logf(LOG_DEBUG, "Response Code: %d", (int)_request.responseHTTPcode());
    }
  }
}
