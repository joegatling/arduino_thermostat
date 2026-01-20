#include "MqttController.h"

#define LOG Serial

MqttController::MqttController(): 
    thermostat(nullptr), 
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
    });

    thermostat->onHeaterPowerChanged([this](bool newHeaterPowerState)
    {
        sendCallForHeatTopic();
    });


//    sendDiscoveryMessage();
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
    doc["device"]["model"] = "MQTT Test Model"; 
    doc["device"]["identifiers"][0] = getDeviceId() + "_device";

    doc["origin"]["name"] = "mqtt";

    doc["components"]["climate"]["platform"] = "climate";
    doc["components"]["climate"]["name"] = "Thermostat";
    doc["components"]["climate"]["unique_id"] = getDeviceId() + "_climate";    
    doc["components"]["climate"]["temperature_unit"] = "C";
    doc["components"]["climate"]["min_temp"] = 10;
    doc["components"]["climate"]["max_temp"] = 35;    
    doc["components"]["climate"]["mode_command_topic"] = TOPIC_PREFIX + deviceNameLower + MODE_COMMAND_TOPIC_SUFFIX;
    doc["components"]["climate"]["mode_state_topic"] = TOPIC_PREFIX + deviceNameLower + MODE_STATE_TOPIC_SUFFIX;
    doc["components"]["climate"]["modes"] = JsonArray();
    doc["components"]["climate"]["modes"].add("off");
    doc["components"]["climate"]["modes"].add("heat");    
    doc["components"]["climate"]["current_temperature_topic"] = TOPIC_PREFIX + deviceNameLower + CURRENT_TEMPERATURE_TOPIC_SUFFIX;    
    doc["components"]["climate"]["temperature_command_topic"] = TOPIC_PREFIX + deviceNameLower + TARGET_TEMPERATURE_COMMAND_TOPIC_SUFFIX;
    doc["components"]["climate"]["temperature_state_topic"] = TOPIC_PREFIX + deviceNameLower + TARGET_TEMPERATURE_STATE_TOPIC_SUFFIX;
    doc["components"]["climate"]["availability_topic"] = TOPIC_PREFIX + deviceNameLower + AVAILABILITY_TOPIC_SUFFIX;
       
    doc["components"]["temperature"]["platform"] = "sensor";
    doc["components"]["temperature"]["name"] = "Temperature Sensor";
    doc["components"]["temperature"]["unit_of_measurement"] = "\u00B0C";
    doc["components"]["temperature"]["device_class"] = "temperature";
    doc["components"]["temperature"]["state_topic"] = TOPIC_PREFIX + deviceNameLower + CURRENT_TEMPERATURE_TOPIC_SUFFIX;    
    doc["components"]["temperature"]["state_class"] = "measurement";
    doc["components"]["temperature"]["unique_id"] = getDeviceId() + "_temperature";
    doc["components"]["temperature"]["availability_topic"] = TOPIC_PREFIX + deviceNameLower + AVAILABILITY_TOPIC_SUFFIX;

    doc["components"]["call_for_heat"]["platform"] = "binary_sensor";
    doc["components"]["call_for_heat"]["name"] = "Call for Heat";
    doc["components"]["call_for_heat"]["device_class"] = "power";
    doc["components"]["call_for_heat"]["state_topic"] = TOPIC_PREFIX + deviceNameLower + CALL_FOR_HEAT_TOPIC_SUFFIX;    
    doc["components"]["call_for_heat"]["unique_id"] = getDeviceId() + "_call_for_heat";
    doc["components"]["call_for_heat"]["payload_on"] = "ON";
    doc["components"]["call_for_heat"]["payload_off"] = "OFF";
    doc["components"]["call_for_heat"]["availability_topic"] = TOPIC_PREFIX + deviceNameLower + AVAILABILITY_TOPIC_SUFFIX;
    
    // TODO: Add the ability to send a screen message via MQTT and have it temporarily display on the screen
    // doc["components"]["screen"]["platform"] = "text";
    // doc["components"]["screen"]["name"] = "Thermostat Screen";
    // doc["components"]["screen"]["unique_id"] = getDeviceId() + "_screen";
    // doc["components"]["screen"]["text_command_topic"] = "home/" + deviceNameLower + "/message/set";


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

    String deviceNameLower = deviceName;
    deviceNameLower.toLowerCase();    

    String topic = String(TOPIC_PREFIX) + deviceNameLower + CURRENT_TEMPERATURE_TOPIC_SUFFIX;
    mqttClient.publish(topic.c_str(), String(thermostat->getCurrentTemperature(true)).c_str(), true);    
}   

void MqttController::sendTargetTemperatureTopic()
{
    if(!isReady()) return;

    String deviceNameLower = deviceName;
    deviceNameLower.toLowerCase();    

    String topic = String(TOPIC_PREFIX) + deviceNameLower + TARGET_TEMPERATURE_STATE_TOPIC_SUFFIX;
    mqttClient.publish(topic.c_str(), String(thermostat->getTargetTemperature(true)).c_str(), true);    
}

void MqttController::sendModeTopic()
{
    if(!isReady()) return; 

    String deviceNameLower = deviceName;
    deviceNameLower.toLowerCase();

    String topic = String(TOPIC_PREFIX) + deviceNameLower + MODE_STATE_TOPIC_SUFFIX;
    String modeStr;
    switch (thermostat->getMode())
    {
        case OFF:
            modeStr = "off";
            break;
        case HEAT:
            modeStr = "heat";
            break;
        case BOOST: // Boost is not a mode that Home Assistant recognizes, map to heat
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

    String deviceNameLower = deviceName;
    deviceNameLower.toLowerCase();

    String topic = String(TOPIC_PREFIX) + deviceNameLower + CALL_FOR_HEAT_TOPIC_SUFFIX;
    String powerStateStr = thermostat->getHeaterPowerState() ? "ON" : "OFF";
    mqttClient.publish(topic.c_str(), powerStateStr.c_str(), true);
}

void MqttController::connectMqtt()
{
    if (!mqttClient.connected() && canTryReconnect())
    {
        lastReconnectAttemptTime = millis();
        LOG.print("Connecting to MQTT...");

        String deviceNameLower = deviceName;
        deviceNameLower.toLowerCase();
        String availabilityTopic = String(TOPIC_PREFIX) + deviceNameLower + AVAILABILITY_TOPIC_SUFFIX;
        
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

            String modeCommandTopic = String(TOPIC_PREFIX) + deviceNameLower + MODE_COMMAND_TOPIC_SUFFIX;
            String targetTemperatureCommandTopic = String(TOPIC_PREFIX) + deviceNameLower + TARGET_TEMPERATURE_COMMAND_TOPIC_SUFFIX;

            mqttClient.subscribe(modeCommandTopic.c_str());
            LOG.print("Subscribed: ");
            LOG.println(modeCommandTopic);

            mqttClient.subscribe(targetTemperatureCommandTopic.c_str());
            LOG.print("Subscribed: ");
            LOG.println(targetTemperatureCommandTopic);

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

    String deviceNameLower = deviceName;
    deviceNameLower.toLowerCase();

    String modeCommandTopic = String(TOPIC_PREFIX) + deviceNameLower + MODE_COMMAND_TOPIC_SUFFIX;
    String targetTemperatureTopic = String(TOPIC_PREFIX) + deviceNameLower + TARGET_TEMPERATURE_COMMAND_TOPIC_SUFFIX;

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
        else if (payloadStr == "boost")
        {
            thermostat->setMode(BOOST);
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

        if(targetTemp == 0)
        {
            LOG.print("Invalid target temperature: ");
            LOG.println(payloadStr);
            return;
        }

        thermostat->setTargetTemperature(targetTemp, true);
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