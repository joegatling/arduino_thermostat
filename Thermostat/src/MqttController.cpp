#include "MqttController.h"
#include "LEDController.h"

#define LOG Serial

MqttController::MqttController(): 
    thermostat(nullptr), 
    ledController(nullptr),
    mqtt_server(""), 
    mqtt_port(0), 
    mqtt_user(""), 
    mqtt_password(""),
    mqttClient(espClient),
    deviceName("Thermostat"),
    lastReconnectAttemptTime(0)
{
    mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
    mqttClient.setBufferSize(MSG_BUFFER_SIZE);
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length)
    {
        LOG.println("MQTT Message Received:");
        LOG.print("Topic: ");
        LOG.println(topic);
        LOG.print("Payload: ");
        for (unsigned int i = 0; i < length; i++)
        {
            LOG.print((char)payload[i]);
        }
        LOG.println();

        callback(topic, payload, length);
    });
}

MqttController::~MqttController()
{
}

void MqttController::setThermostat(Thermostat* newThermostat)
{
    if (newThermostat == nullptr)
    {
        return;
    }

    if(thermostat != nullptr)
    {
        return; // Already initialized
    }

    thermostat = newThermostat;

    thermostat->onTargetTemperatureChanged([this](float newTargetTemp)
    {   
        sendTargetTemperatureTopic();
    });

    thermostat->onCurrentTemperatureChanged([this](float newCurrentTemp)
    {
        sendCurrentTemperatureTopic();
    });

    thermostat->onModeChanged([this](ThermostatMode newMode)
    {
        sendModeTopic();
        sendActionTopic();
    });

    thermostat->onHeaterPowerChanged([this](bool newHeaterPowerState)
    {
        sendCallForHeatTopic();
        sendActionTopic();
    });


//    sendDiscoveryMessage();
}

void MqttController::setLedController(LedController* newLedController)
{
    ledController = newLedController;
}

void MqttController::setConnectionInfo(const String& server, int port, const String& user, const String& password)
{
    mqtt_server = server;
    mqtt_port = port;
    mqtt_user = user;
    mqtt_password = password;

    mqttClient.setServer(mqtt_server.c_str(), mqtt_port);
}

void MqttController::update()
{
    if(!isReady()) return;

    if(!mqttClient.connected())
    {
        connectMqtt();
    }

    mqttClient.loop();
}

void MqttController::sendDiscoveryMessage()
{
    if(!isReady()) return;
    if(!mqttClient.connected()) return;

    LOG.println("Sending Discovery Message...");

    String baseTopic = String(DISCOVERY_PREFIX) + "/device/" + getDeviceId() + "_device/config";
    
    JsonDocument doc;

    //doc["discovery_version"] = 4;

    String deviceNameLower = deviceName;
    deviceNameLower.toLowerCase();

    doc["device"]["name"] = deviceName;
    doc["device"]["manufacturer"] = "Joe Gatling";
    doc["device"]["model"] = "Yellow Boxy"; 
    doc["device"]["identifiers"][0] = getDeviceId() + "_device";

    doc["origin"]["name"] = "mqtt";

    doc["components"]["climate"]["platform"] = "climate";
    doc["components"]["climate"]["name"] = "Thermostat";
    doc["components"]["climate"]["unique_id"] = getDeviceId() + "_climate";    
    doc["components"]["climate"]["temperature_unit"] = "C";
    doc["components"]["climate"]["min_temp"] = 16;
    doc["components"]["climate"]["max_temp"] = 27;    
    doc["components"]["climate"]["mode_command_topic"] = buildTopic(MODE_COMMAND_TOPIC_SUFFIX);
    doc["components"]["climate"]["mode_state_topic"] = buildTopic(MODE_STATE_TOPIC_SUFFIX);
    doc["components"]["climate"]["modes"] = JsonArray();
    doc["components"]["climate"]["modes"].add("off");
    doc["components"]["climate"]["modes"].add("heat");    
    doc["components"]["climate"]["current_temperature_topic"] = buildTopic(CURRENT_TEMPERATURE_TOPIC_SUFFIX);    
    doc["components"]["climate"]["temperature_command_topic"] = buildTopic(TARGET_TEMPERATURE_COMMAND_TOPIC_SUFFIX);
    doc["components"]["climate"]["temperature_state_topic"] = buildTopic(TARGET_TEMPERATURE_STATE_TOPIC_SUFFIX);
    doc["components"]["climate"]["availability_topic"] = buildTopic(AVAILABILITY_TOPIC_SUFFIX);
    doc["components"]["climate"]["preset_mode_command_topic"] = buildTopic(PRESET_MODE_COMMAND_TOPIC_SUFFIX);
    doc["components"]["climate"]["preset_mode_state_topic"] = buildTopic(PRESET_MODE_STATE_TOPIC_SUFFIX);
    doc["components"]["climate"]["preset_modes"] = JsonArray();
    doc["components"]["climate"]["preset_modes"].add("eco");
    doc["components"]["climate"]["preset_modes"].add("boost");
    doc["components"]["climate"]["preset_modes"].add("sleep");
    doc["components"]["climate"]["action_topic"] = buildTopic(ACTION_TOPIC_SUFFIX);
       
    doc["components"]["temperature"]["platform"] = "sensor";
    doc["components"]["temperature"]["name"] = "Temperature Sensor";
    doc["components"]["temperature"]["unit_of_measurement"] = "\u00B0C";
    doc["components"]["temperature"]["device_class"] = "temperature";
    doc["components"]["temperature"]["state_topic"] = buildTopic(CURRENT_TEMPERATURE_TOPIC_SUFFIX);    
    doc["components"]["temperature"]["state_class"] = "measurement";
    doc["components"]["temperature"]["unique_id"] = getDeviceId() + "_temperature";
    doc["components"]["temperature"]["availability_topic"] = buildTopic(AVAILABILITY_TOPIC_SUFFIX);

    doc["components"]["call_for_heat"]["platform"] = "binary_sensor";
    doc["components"]["call_for_heat"]["name"] = "Call for Heat";
    doc["components"]["call_for_heat"]["device_class"] = "power";
    doc["components"]["call_for_heat"]["state_topic"] = buildTopic(CALL_FOR_HEAT_TOPIC_SUFFIX);    
    doc["components"]["call_for_heat"]["unique_id"] = getDeviceId() + "_call_for_heat";
    doc["components"]["call_for_heat"]["payload_on"] = "ON";
    doc["components"]["call_for_heat"]["payload_off"] = "OFF";
    doc["components"]["call_for_heat"]["availability_topic"] = buildTopic(AVAILABILITY_TOPIC_SUFFIX);
    
    doc["components"]["display"]["platform"] = "text";
    doc["components"]["display"]["name"] = "Display Message";
    doc["components"]["display"]["unique_id"] = getDeviceId() + "_display";
    doc["components"]["display"]["command_topic"] = buildTopic(DISPLAY_MESSAGE_COMMAND_TOPIC_SUFFIX);
    doc["components"]["display"]["availability_topic"] = buildTopic(AVAILABILITY_TOPIC_SUFFIX);
    
    // Serialize JSON to string
    String payload;
    serializeJson(doc, payload);

    auto result = mqttClient.publish(baseTopic.c_str(), payload.c_str(), true); // Retain the message

    LOG.println(baseTopic.c_str());
    LOG.println();
    LOG.println(payload.c_str());
        LOG.println();
    LOG.print("Size: ");
    LOG.println(String(baseTopic.length() + payload.length()));

    LOG.println(result == true ? "Success" : "Failed");

    if(result == false)
    {
        LOG.print("MQTT Publish failed with state: ");
        LOG.println(mqttClient.state());
    }    
}

void MqttController::sendEmptyDiscoveryMessage()
{
    if(!isReady()) return;
    if(!mqttClient.connected()) return;

    LOG.print("Sending Empty Discovery Message...");

    String baseTopic = String(DISCOVERY_PREFIX) + "/device/" + getDeviceId() + "_device/config";

    auto result = mqttClient.publish(baseTopic.c_str(), "", true); // Retain the message

    LOG.println(result == true ? "Success" : "Failed");

    if(result == false)
    {
        LOG.print("MQTT Publish failed with state: ");
        LOG.println(mqttClient.state());
    }
}

void MqttController::sendCurrentTemperatureTopic()
{
    if(!isReady()) return;

    String topic = buildTopic(CURRENT_TEMPERATURE_TOPIC_SUFFIX);
    mqttClient.publish(topic.c_str(), String(thermostat->getCurrentTemperature(true)).c_str(), true);    
}   

void MqttController::sendTargetTemperatureTopic()
{
    if(!isReady()) return;

    String topic = buildTopic(TARGET_TEMPERATURE_STATE_TOPIC_SUFFIX);
    mqttClient.publish(topic.c_str(), String(thermostat->getTargetTemperature(true)).c_str(), true);    
}

void MqttController::sendModeTopic()
{
    if(!isReady()) return; 

    String topic = buildTopic(MODE_STATE_TOPIC_SUFFIX);
    String modeStr;
    switch (thermostat->getMode())
    {
        case OFF:
            modeStr = "off";
            break;
        case HEAT:
            modeStr = "heat";
            break;
        default:
            modeStr = "off";
            break;
    }
    mqttClient.publish(topic.c_str(), modeStr.c_str(), true);
}

void MqttController::sendCallForHeatTopic()
{
    if(!isReady()) return; 

    String topic = buildTopic(CALL_FOR_HEAT_TOPIC_SUFFIX);
    String powerStateStr = thermostat->getHeaterPowerState() ? "ON" : "OFF";
    mqttClient.publish(topic.c_str(), powerStateStr.c_str(), true);
}

void MqttController::sendPresetModeTopic()
{
    if(!isReady()) return; 

    String topic = buildTopic(PRESET_MODE_STATE_TOPIC_SUFFIX);
    String presetModeStr;
    switch (thermostat->getPreset())
    {
        case BOOST:
            presetModeStr = "boost";
            break;
        case ECO:
            presetModeStr = "eco";
            break;
        case SLEEP:
            presetModeStr = "sleep";
            break;
        case NONE:
            presetModeStr = "none";
            break;
        default:
            presetModeStr = "eco";
            break;
    }
    mqttClient.publish(topic.c_str(), presetModeStr.c_str(), true);
}

void MqttController::sendActionTopic()
{
    if(!isReady()) return; 

    String topic = buildTopic(ACTION_TOPIC_SUFFIX);
    String actionStr;

    if(thermostat->getMode() == OFF)
    {
        actionStr = "off";
    }
    else
    {
        if(thermostat->getHeaterPowerState())
        {
            actionStr = "heating";
        }
        else
        {
            actionStr = "idle";
        }
    }

   
    mqttClient.publish(topic.c_str(), actionStr.c_str(), true);
}

void MqttController::connectMqtt()
{
    if (!mqttClient.connected() && canTryReconnect())
    {
        lastReconnectAttemptTime = millis();
        LOG.print("Connecting to MQTT...");

        String availabilityTopic = buildTopic(AVAILABILITY_TOPIC_SUFFIX);
        
        // Set Last Will and Testament: if device disconnects unexpectedly, broker publishes "offline"
        if (mqttClient.connect(
            getDeviceId().c_str(), 
            mqtt_user.c_str(), 
            mqtt_password.c_str(),
            availabilityTopic.c_str(),  // will_topic
            1,                           // will_qos
            true,                        // will_retain
            "offline"                    // will_message
        ))
        {
            LOG.println("connected");

            String modeCommandTopic = buildTopic(MODE_COMMAND_TOPIC_SUFFIX);
            String targetTemperatureCommandTopic = buildTopic(TARGET_TEMPERATURE_COMMAND_TOPIC_SUFFIX);
            String displayMessageCommandTopic = buildTopic(DISPLAY_MESSAGE_COMMAND_TOPIC_SUFFIX);
            String presetModeCommandTopic = buildTopic(PRESET_MODE_COMMAND_TOPIC_SUFFIX);

            mqttClient.subscribe(modeCommandTopic.c_str());
            LOG.print("Subscribed: ");
            LOG.println(modeCommandTopic);

            mqttClient.subscribe(targetTemperatureCommandTopic.c_str());
            LOG.print("Subscribed: ");
            LOG.println(targetTemperatureCommandTopic);

            mqttClient.subscribe(displayMessageCommandTopic.c_str());
            LOG.print("Subscribed: ");
            LOG.println(displayMessageCommandTopic);

            mqttClient.subscribe(presetModeCommandTopic.c_str());
            LOG.print("Subscribed: ");
            LOG.println(presetModeCommandTopic);

            // Publish "online" to availability topic
            mqttClient.publish(availabilityTopic.c_str(), "online", true);
            LOG.print("Availability: ");
            LOG.println(availabilityTopic);

            // Once connected, publish an announcement...
            sendDiscoveryMessage();

            sendCurrentTemperatureTopic();
            sendTargetTemperatureTopic();
            sendModeTopic();
            sendCallForHeatTopic();
            sendPresetModeTopic();
            sendActionTopic();
        }
        else
        {
            LOG.print("failed, rc=");
            switch(mqttClient.state())
            {
                case MQTT_CONNECTION_TIMEOUT:
                    LOG.println(" - MQTT_CONNECTION_TIMEOUT");
                    break;
                case MQTT_CONNECTION_LOST:
                    LOG.println(" - MQTT_CONNECTION_LOST");
                    break;
                case MQTT_CONNECT_FAILED:
                    LOG.println(" - MQTT_CONNECT_FAILED");
                    break;
                case MQTT_DISCONNECTED:
                    LOG.println(" - MQTT_DISCONNECTED");
                    break;
                case MQTT_CONNECTED:
                    LOG.println(" - MQTT_CONNECTED");
                    break;
                case MQTT_CONNECT_BAD_PROTOCOL:
                    LOG.println(" - MQTT_CONNECT_BAD_PROTOCOL");
                    break;
                case MQTT_CONNECT_BAD_CLIENT_ID:
                    LOG.println(" - MQTT_CONNECT_BAD_CLIENT_ID");
                    break;
                case MQTT_CONNECT_UNAVAILABLE:
                    LOG.println(" - MQTT_CONNECT_UNAVAILABLE");
                    break;
                case MQTT_CONNECT_BAD_CREDENTIALS:
                    LOG.println(" - MQTT_CONNECT_BAD_CREDENTIALS");
                    break;
                case MQTT_CONNECT_UNAUTHORIZED:
                    LOG.println(" - MQTT_CONNECT_UNAUTHORIZED");
                    break;
                default:
                    LOG.println("");
                    break;
            }
        }
    }
}

void MqttController::callback(char* topic, byte* payload, unsigned int length)
{
    if(!isReady()) return;

    LOG.println("MQTT Message Received:");
    LOG.print("Topic: ");
    LOG.println(topic);
    LOG.print("Payload: ");
    for (unsigned int i = 0; i < length; i++)
    {
        LOG.print((char)payload[i]);
    }
    LOG.println();
    
    String topicStr = String(topic);
    String payloadStr = String((char*)payload).substring(0, length);

    String modeCommandTopic = buildTopic(MODE_COMMAND_TOPIC_SUFFIX);
    String targetTemperatureTopic = buildTopic(TARGET_TEMPERATURE_COMMAND_TOPIC_SUFFIX);
    String displayMessageCommandTopic = buildTopic(DISPLAY_MESSAGE_COMMAND_TOPIC_SUFFIX);
    String presetModeCommandTopic = buildTopic(PRESET_MODE_COMMAND_TOPIC_SUFFIX);

    if (topicStr == modeCommandTopic)
    {
        if (payloadStr == "off")
        {
            thermostat->setMode(OFF);
        }
        else if (payloadStr == "heat")
        {
            thermostat->setMode(HEAT);
        }
        else
        {
            LOG.print("Unknown mode: ");
            LOG.println(payloadStr);
        }
    }
    else if (topicStr == targetTemperatureTopic)
    {
        float targetTemp = payloadStr.toFloat();

        // Since we are a heat-only thermostat, we can assume a 0 target is invalid
        if(targetTemp > 0 == false)
        {
            LOG.print("Invalid target temperature: ");
            LOG.println(payloadStr);
            return;
        }

        thermostat->setTargetTemperature(targetTemp, true);

        thermostat->setMode(HEAT);

        if(thermostat->getCurrentTemperature() < thermostat->getTargetTemperature())
        {
            thermostat->setPreset(BOOST);
        }
    }
    else if (topicStr == displayMessageCommandTopic)
    {
        if (ledController != nullptr)
        {
            // Validate message is not empty and length is reasonable
            if(payloadStr.length() == 0 || payloadStr.length() > 256)
            {
                LOG.print("Invalid display message length: ");
                LOG.println(payloadStr.length());
                return;
            }

            payloadStr.toUpperCase();
            ledController->showStatusMessage(payloadStr, false, true);
            LOG.print("Display message: ");
            LOG.println(payloadStr);
        }
        else
        {
            LOG.println("LED Controller not initialized");
        }
    }
    else if (topicStr == presetModeCommandTopic)
    {
        if (payloadStr == "boost")
        {
            thermostat->setPreset(BOOST);
        }
        else if(payloadStr == "sleep")
        {
            thermostat->setPreset(SLEEP);
        }
        else if (payloadStr == "eco")
        {
            thermostat->setPreset(ECO);
        }
        else if (payloadStr == "none")
        {
            thermostat->setPreset(NONE);
        }
        else
        {
            LOG.print("Unknown preset mode: ");
            LOG.println(payloadStr);
        }
    }
    else
    {
        LOG.print("Unknown topic: ");
        LOG.println(topicStr);
    }
}

String MqttController::getDeviceId()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char deviceId[13];
    snprintf(deviceId, sizeof(deviceId), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(deviceId);
}

void MqttController::setDeviceName(const String& name)
{
    if(!name.equals(deviceName))
    {
        sendEmptyDiscoveryMessage();
        deviceName = name;
        sendDiscoveryMessage();
    }

}

String MqttController::getDeviceName() const
{
    return deviceName;
}

bool MqttController::isReady()
{
    return thermostat != nullptr;
}

String MqttController::buildTopic(const char* suffix)
{
    String deviceNameLower = deviceName;
    deviceNameLower.toLowerCase();
    return String(TOPIC_PREFIX) + deviceNameLower + suffix;
}