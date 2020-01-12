#include "RemoteThermostatController.h"

#define GET_DATA_URL "http://temp.joegatling.com/get-thermostat-data.php"
#define SET_CURRENT_TEMPERATURE_URL "http://joegatling.com/sites/temperature/set-current-temperature.php"

#define SERVER_POLL_INTERVAL 10000


RemoteThermostatController::RemoteThermostatController(String key, String thermostatName, bool useRemoteTemperature)
{
    _apiKey = key;
    _thermostat = thermostatName;
    _shouldUseRemoteTemperature = useRemoteTemperature;

    GetDataFromServer();
}

void RemoteThermostatController::Update()
{
}
    
void RemoteThermostatController::SetCurrentTemperature(float celsius)
{
  _currentTemperature = celsius;
  _isTargetTemperatureSetLocally = true;
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
  WiFiClient client;
  HTTPClient http;

  // configure traged server and url
  String url = String(SET_CURRENT_TEMPERATURE_URL);
  url.concat("?key=");
  url.concat(_apiKey);
  url.concat("&c=");
  url.concat(_currentTemperature);

  SERIAL_OUPUT.print("URL: ");
  SERIAL_OUPUT.println(url);
  
  http.begin(client, url);

  //start connection and send HTTP header and body
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) 
  {
    // file found at server
    if (httpCode == HTTP_CODE_OK) 
    {
      const String& payload = http.getString();
      
      SERIAL_OUPUT.print("received response: ");
      SERIAL_OUPUT.println(payload);
    }
  } 
  else 
  {
    SERIAL_OUPUT.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();   

  SERIAL_OUPUT.println("");  
}

void RemoteThermostatController::SendTargetTemperatureToServer()
{
  //TODO
}

void RemoteThermostatController::GetDataFromServer()
{
  WiFiClient client;
  HTTPClient http;

  SERIAL_OUPUT.print("URL: ");
  SERIAL_OUPUT.println(_getDataUrl);

  // configure traged server and url
  http.begin(client, _getDataUrl); //HTTP
  http.addHeader("Content-Type", "application/json");

  // start connection and send HTTP header and body
  int httpCode = http.GET();

  // httpCode will be negative on error
  if (httpCode > 0) 
  {
    // file found at server
    if (httpCode == HTTP_CODE_OK) 
    {
      const String& payload = http.getString();
      
      SERIAL_OUPUT.println("received payload:");
      SERIAL_OUPUT.print(payload);

      auto error = deserializeJson(_jsonDocument, payload);
      
      if(!error)
      {
        _jsonObject = _jsonDocument.as<JsonObject>();

        if(_shouldUseRemoteTemperature && !_isCurrentTemperatureSetLocally && _jsonObject["current"].containsKey("celsius"))
        {
          _currentTemperature = float(_jsonObject["current"]["celsius"]);
          SERIAL_OUPUT.print("Current Temperature: ");
          SERIAL_OUPUT.println(_currentTemperature);
        }

        if(!_isTargetTemperatureSetLocally && _jsonObject["target"].containsKey("celsius"))
        {
          _targetTemperature = float(_jsonObject["target"]["celsius"]);
          SERIAL_OUPUT.print("Target Temperature: ");
          SERIAL_OUPUT.println(_targetTemperature);
        }

        _isCurrentTemperatureSetLocally = false;
        _isTargetTemperatureSetLocally = false;
      }
    }
  } 
  else 
  {
    SERIAL_OUPUT.printf("Error: %s\n", http.errorToString(httpCode).c_str());
  }

  SERIAL_OUPUT.println("");

  http.end();   
}
