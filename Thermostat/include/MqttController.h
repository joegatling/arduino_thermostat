#pragma once

#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>

#include "Thermostat.h"

class LedController; // Forward declaration

#define MSG_BUFFER_SIZE	(2048)

#define DISCOVERY_PREFIX                    "homeassistant"

#define TOPIC_PREFIX                        "home/"
#define MODE_COMMAND_TOPIC_SUFFIX           "/mode/set"
#define MODE_STATE_TOPIC_SUFFIX             "/mode"
#define CURRENT_TEMPERATURE_TOPIC_SUFFIX    "/temperature"
#define TARGET_TEMPERATURE_COMMAND_TOPIC_SUFFIX     "/target_temperature/set"
#define TARGET_TEMPERATURE_STATE_TOPIC_SUFFIX       "/target_temperature"
#define CALL_FOR_HEAT_TOPIC_SUFFIX          "/call_for_heat"
#define AVAILABILITY_TOPIC_SUFFIX          "/availability"
#define DISPLAY_MESSAGE_COMMAND_TOPIC_SUFFIX "/display/set"

#define RECONNECT_INTERVAL_MS              5000


class MqttController
{
public:
    MqttController();
    ~MqttController();

    void update();
    void setThermostat(Thermostat* thermostat);
    void setLedController(LedController* ledController);

    void setConnectionInfo(const String& server, int port, const String& user, const String& password);

    void setDeviceName(const String& name);
    String getDeviceName() const;

private:

    void sendDiscoveryMessage();
    void sendEmptyDiscoveryMessage();

    void sendCurrentTemperatureTopic();
    void sendTargetTemperatureTopic();
    void sendModeTopic();
    void sendCallForHeatTopic();

    void connectMqtt();
    bool canTryReconnect() { return lastReconnectAttemptTime == 0 || (millis() - lastReconnectAttemptTime > RECONNECT_INTERVAL_MS); }

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
    LedController* ledController;

    unsigned long lastReconnectAttemptTime = 0;

};