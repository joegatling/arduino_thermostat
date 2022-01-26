#include "RemoteThermostatController.h"

#define HOST "http://joegatling.com"
#define GET_DATA_URL HOST "/sites/temperature/get-thermostat-data.php"
#define SET_CURRENT_TEMPERATURE_URL HOST "/sites/temperature/set-current-temperature.php"
#define SET_TARGET_TEMPERATURE_URL HOST "/sites/temperature/set-target-temperature.php"

#define SERVER_POLL_INTERVAL 10000


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

  _getDataUrl = String(GET_DATA_URL);
  _getDataUrl.concat(F("?key="));
  _getDataUrl.concat(_apiKey);
  _getDataUrl.concat(F("&thermostat="));
  _getDataUrl.concat(_thermostat);

  _request.setDebug(false);
  _request.onReadyStateChange([=](void* optParm, asyncHTTPrequest* request, int readyState)
  {
    OnRequestReadyStateChanged(optParm, request, readyState);
  });

  _currentRequestType = NO_REQUEST;

  // Assume the thermostat is off at first. This will get replaced by server data once it arrives.
  _isThermostatOn = false;

  GetDataFromServer();
}

void RemoteThermostatController::Update()
{
  _wasTemperatureSetRemotely = false;
  _wasPowerSetRemotely = false;
  _remoteTemperatureChangeDelta = 0;
  
  if((millis() - _lastServerUpdate) > SERVER_POLL_INTERVAL)
  {
    _shouldSendCurrentTemperature = true;
    _shouldSendTargetTemperature = true;  
    _shouldGetData = _isInLocalMode == false;

    _lastServerUpdate = millis();

  
  }  

  if(!IsRequestInProgress())
  {
   if(_isSyslogOn)
    {
      syslog.log(LOG_DEBUG, "Time to sync"); 
    }      
    
    // This if statement ensures we only do one of these operations each update.
    if(_shouldSendCurrentTemperature && _isCurrentTemperatureSetLocally)
    {
      SendCurrentTemperatureToServer();
      _shouldSendCurrentTemperature = false;
    }
    else if(_shouldSendTargetTemperature && _isTargetTemperatureSetLocally)
    {
      SendTargetTemperatureToServer();
      _shouldSendTargetTemperature = false;
    }
    else if(_shouldGetData)
    {
      GetDataFromServer();
      _shouldGetData = false;
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
  _currentTemperature = celsius;
  _isCurrentTemperatureSetLocally = true;
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
  
void RemoteThermostatController::SendCurrentTemperatureToServer()
{
  if(!IsRequestInProgress())
  {
    String url = String(SET_CURRENT_TEMPERATURE_URL);
    url.concat(F("?key="));
    url.concat(_apiKey);
    url.concat(F("&c="));
    url.concat(_currentTemperature);
    url.concat(F("&thermostat="));
    url.concat(_thermostat);

    _currentRequestType = SEND_CURRENT_TEMPERATURE;

    if(_isSyslogOn)
    {
      syslog.log(LOG_DEBUG, "Sending current data to server."); 
      syslog.log(LOG_DEBUG, url);               
    }

    SERIAL_OUTPUT.println(F("Sending Current Temperature"));     
       
    _request.open("GET", url.c_str());
    _request.send();
  }
}

void RemoteThermostatController::SendTargetTemperatureToServer()
{
  if(!IsRequestInProgress())
  {    
    // configure traged server and url
    String url = String(SET_TARGET_TEMPERATURE_URL);
    url.concat(F("?key="));
    url.concat(_apiKey);
    url.concat(F("&c="));
    url.concat(_targetTemperature);
    url.concat(F("&thermostat="));
    url.concat(_thermostat);
    url.concat(F("&power="));
    url.concat(_isThermostatOn);
    
    _currentRequestType = SET_TARGET_TEMPERATURE;

    if(_isSyslogOn)
    {
      syslog.log(LOG_DEBUG, "Sending target temp to server."); 
      syslog.log(LOG_DEBUG, url);               
    }       

    SERIAL_OUTPUT.println(F("Sending Target Temperature"));    
    
    _request.open("GET", url.c_str());
    _request.send();
 
  }
}

void RemoteThermostatController::GetDataFromServer()
{
  if(!IsRequestInProgress())
  {
    _currentRequestType = GET_DATA;

    SERIAL_OUTPUT.println(F("Get Data"));        
    _request.open("GET", _getDataUrl.c_str());
    _request.send();   

    if(_isSyslogOn)
    {
      syslog.log(LOG_DEBUG, "Requesting data from server."); 
    }     
  }
}

void RemoteThermostatController::OnRequestReadyStateChanged(void* optParm, asyncHTTPrequest* request, int readyState)
{
//  if(_isSyslogOn)
//  {
//    syslog.logf(LOG_DEBUG, "Ready state changed: %d", readyState);
//  }
//   else
//  {     
//    SERIAL_OUTPUT.println(F("No Syslog")); 
//  }
 
  
  if(readyState == 4)
  {    
    if(_currentRequestType == SEND_CURRENT_TEMPERATURE)
    {
      AsyncRequestResponseSendCurrentTemperature();
    }
    else if(_currentRequestType == SET_TARGET_TEMPERATURE)
    {
      AsyncRequestResponseSetTargetTemperature();
    }
    else if(_currentRequestType == GET_DATA)
    {
      AsyncRequestResponseGetData();
    }   

    _lastServerResponse = millis();

     _currentRequestType = NO_REQUEST;
  }
}

void RemoteThermostatController::AsyncRequestResponseSendCurrentTemperature()
{
//  if(_request.responseHTTPcode() == 200)
//  {   
//    SERIAL_OUTPUT.print(F("Set Current Temp: "));  
//    SERIAL_OUTPUT.println(_request.responseText());    
//  }

    if(_isSyslogOn)
    {
      syslog.logf(LOG_DEBUG, "Response Code: %d", (int)_request.responseHTTPcode());
      syslog.log(LOG_DEBUG, _request.responseText()); 
    } 
}



void RemoteThermostatController::AsyncRequestResponseSetTargetTemperature()
{
//  if(_request.responseHTTPcode() == 200)
//  {  
//    SERIAL_OUTPUT.print(F("Set Target Temp: "));  
//    SERIAL_OUTPUT.println(_request.responseText());    
//  }

    if(_isSyslogOn)
    {
      syslog.logf(LOG_DEBUG, "Response Code: %d", (int)_request.responseHTTPcode());
      syslog.log(LOG_DEBUG, _request.responseText()); 
    } 
}

void RemoteThermostatController::AsyncRequestResponseGetData()
{
  if(_request.responseHTTPcode() == 200)
  {
    const String& payload = _request.responseText();
    
    SERIAL_OUTPUT.println(F("Thermostat Data:"));
    SERIAL_OUTPUT.println(payload);

    if(_isSyslogOn)
    {
      syslog.log(LOG_DEBUG, "Thermostat Data:"); 
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

      _isCurrentTemperatureSetLocally = false;
      _isTargetTemperatureSetLocally = false;
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
