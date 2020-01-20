#include "RemoteThermostatController.h"

#define HOST "http://joegatling.com"
#define GET_DATA_URL HOST "/sites/temperature/get-thermostat-data.php"
#define SET_CURRENT_TEMPERATURE_URL HOST "/sites/temperature/set-current-temperature.php"
#define SET_TARGET_TEMPERATURE_URL HOST "/sites/temperature/set-target-temperature.php"

#define SERVER_POLL_INTERVAL 10000


RemoteThermostatController::RemoteThermostatController(String key, String thermostatName, bool useRemoteTemperature)
{
  _apiKey = key;
  _thermostat = thermostatName;
  _shouldUseRemoteTemperature = useRemoteTemperature;

  _getDataUrl = String(GET_DATA_URL);
  _getDataUrl.concat("?key=");
  _getDataUrl.concat(_apiKey);
  _getDataUrl.concat("&thermostat=");
  _getDataUrl.concat(_thermostat);

  _request.setDebug(false);
  _isRequestActive = false;

  GetDataFromServer();
}

void RemoteThermostatController::Update()
{
  if((millis() - _lastServerUpdate) > SERVER_POLL_INTERVAL)
  {
    _shouldSendCurrentTemperature = true;
    _shouldSendTargetTemperature = true;  
    _shouldGetData = true;  

    _lastServerUpdate = millis();
  }  

  if(!IsRequestInProgress())
  {
    // This if statement ensure we only do one of these operations each update.
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
  _targetTemperature = celsius;
  _isTargetTemperatureSetLocally = true;
}

float RemoteThermostatController::GetTargetTemperature()
{
  return _targetTemperature;
}
  
void RemoteThermostatController::SendCurrentTemperatureToServer()
{
  if(!IsRequestInProgress())
  {  
    String url = String(SET_CURRENT_TEMPERATURE_URL);
    url.concat("?key=");
    url.concat(_apiKey);
    url.concat("&c=");
    url.concat(_currentTemperature);
    url.concat("&thermostat=");
    url.concat(_thermostat);

    _request.onReadyStateChange([=](void* optParm, asyncHTTPrequest* request, int readyState)
    {
      if(readyState == 4)
      {
        AsyncRequestResponseSetCurrentTemperature();
        _isRequestActive = false;
      }
    });

    _isRequestActive = true;
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
    url.concat("?key=");
    url.concat(_apiKey);
    url.concat("&c=");
    url.concat(_targetTemperature);
    url.concat("&thermostat=");
    url.concat(_thermostat);
    

    _request.onReadyStateChange([=](void* optParm, asyncHTTPrequest* request, int readyState)
    {
      if(readyState == 4)
      {      
        AsyncRequestResponseSetTargetTemperature();
        
        _isRequestActive = false;
      }
    });

    _isRequestActive = true;
    _request.open("GET", url.c_str());
    _request.send();
  }
}

void RemoteThermostatController::GetDataFromServer()
{
  if(!IsRequestInProgress())
  {
    _request.onReadyStateChange([=](void* optParm, asyncHTTPrequest* request, int readyState)
    {
      if(readyState == 4)
      {
        AsyncRequestResponseGetData();
        _isRequestActive = false;
      }
    });

    _isRequestActive = true;    
    _request.open("GET", _getDataUrl.c_str());
    _request.send();    
  }
}

void RemoteThermostatController::AsyncRequestResponseSetCurrentTemperature()
{
  if(_request.responseHTTPcode() == 200)
  {   
    SERIAL_OUPUT.print("Set Current Temp: ");  
    SERIAL_OUPUT.println(_request.responseText());    
  }
}

void RemoteThermostatController::AsyncRequestResponseSetTargetTemperature()
{
  if(_request.responseHTTPcode() == 200)
  {  
    SERIAL_OUPUT.print("Set Target Temp: ");  
    SERIAL_OUPUT.println(_request.responseText());    
  }
}

void RemoteThermostatController::AsyncRequestResponseGetData()
{
  if(_request.responseHTTPcode() == 200)
  {
    const String& payload = _request.responseText();
    
    SERIAL_OUPUT.println("Thermostat Data:");
    SERIAL_OUPUT.println(payload);
    
    auto error = deserializeJson(_jsonDocument, payload);
    
    if(!error)
    {
      _jsonObject = _jsonDocument.as<JsonObject>();
    
      if(_shouldUseRemoteTemperature && !_isCurrentTemperatureSetLocally && _jsonObject["current"].containsKey("celsius"))
      {
        _currentTemperature = float(_jsonObject["current"]["celsius"]);
//        SERIAL_OUPUT.print("Current Temperature: ");
//        SERIAL_OUPUT.println(_currentTemperature);
      }
    
      if(!_isTargetTemperatureSetLocally && _jsonObject["target"].containsKey("celsius"))
      {
        _targetTemperature = float(_jsonObject["target"]["celsius"]);
//        SERIAL_OUPUT.print("Target Temperature: ");
//        SERIAL_OUPUT.println(_targetTemperature);
      }
    
      _isCurrentTemperatureSetLocally = false;
      _isTargetTemperatureSetLocally = false;
    }
    else
    {
      SERIAL_OUPUT.print(F("deserializeJson() failed: "));
      SERIAL_OUPUT.println(error.c_str());      
    }
  }
}
