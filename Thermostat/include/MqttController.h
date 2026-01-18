#pragma once

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "Thermostat.h"

#define MSG_BUFFER_SIZE	(50)

#define DISCOVERY_PREFIX                    "homeassistant"

#define TOPIC_PREFIX                        "home/"
#define MODE_COMMAND_TOPIC_SUFFIX           "/mode/set"
#define MODE_STATE_TOPIC_SUFFIX             "/mode"
#define CURRENT_TEMPERATURE_TOPIC_SUFFIX    "/temperature"
#define TARGET_TEMPERATURE_TOPIC_SUFFIX     "/target_temperature/set"
#define CALL_FOR_HEAT_TOPIC_SUFFIX          "/call_for_heat"


class MqttController
{
public:
    MqttController();
    ~MqttController();

    void update();
    void setThermostat(Thermostat* thermostat);

    void setConnectionInfo(const String& server, int port, const String& user, const String& password);

    void setDeviceName(const String& name);
    String getDeviceName() const;

private:

    void sendDiscoveryMessage();
    void sendEmptyDiscoveryMessage();

    void sendCurrentTemperatureTopic();
    void sendTargetTemperatureTopic();
    void sendModeTopic();

    void connectMqtt();

    void callback(char* topic, byte* payload, unsigned int length);

    bool isReady();

    String getDeviceId();

    WiFiClient espClient;
    PubSubClient mqttClient;

    String mqtt_server; 
    int mqtt_port;
    String mqtt_password; 
    String mqtt_user;

    String deviceName;

    char msg[MSG_BUFFER_SIZE];

    Thermostat* thermostat;

};