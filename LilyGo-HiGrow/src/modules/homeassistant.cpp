// Filename: homeassistant.cpp
// V1.4 ESP32 sendet data_path in Entity-Metadaten für automatisches Mapping
// V1.3 ESP32 sendet Entity-Metadaten (type, device_class, unit) an HA
// V1.2 Home Assistant Integration Implementation
// V1.1 Initial

#include "homeassistant.h"

HomeAssistantIntegration::HomeAssistantIntegration(AsyncWebServer* srv, SensorManager* sensors, PWMControl* pump) {
    server = srv;
    sensorMgr = sensors;
    pumpControl = pump;
    mac_address = getMacAddress();
    device_id = getDeviceId();
}

//*********************************
// Initialization
//*********************************
bool HomeAssistantIntegration::init() {
    Serial.println("=== Home Assistant Integration ===");
    
    // 1. Setup mDNS
    if (!setupMDNS()) {
        Serial.println("ERROR: mDNS setup failed!");
        return false;
    }
    
    // 2. Setup REST API Endpoints
    setupRestAPI();
    
    Serial.println("HA Integration ready!");
    Serial.print("Access via: http://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local");
    
    return true;
}

//*********************************
// mDNS Setup - Service Discovery
//*********************************
bool HomeAssistantIntegration::setupMDNS() {
    // Start mDNS responder
    if (!MDNS.begin(MDNS_HOSTNAME)) {
        return false;
    }
    
    Serial.print("mDNS responder started: ");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local");
    
    // Add service for Home Assistant discovery
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "model", DEVICE_MODEL);
    MDNS.addServiceTxt("http", "tcp", "version", FIRMWARE_VERSION);
    MDNS.addServiceTxt("http", "tcp", "type", "irrigation");
    
    return true;
}

//*********************************
// REST API Endpoints Setup
//*********************************
void HomeAssistantIntegration::setupRestAPI() {
    server->on("/mada", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleDeviceInfo(request);
    });
    
    server->on("/rpc/mada.GetStatus", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleGetStatus(request);
    });
    
    server->on("/rpc/Pump.Set", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handlePumpSet(request, this->pumpSetBody);
        this->pumpSetBody = ""; // Clear buffer
    }, NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Collect body data
        for (size_t i = 0; i < len; i++) {
            this->pumpSetBody += (char)data[i];
        }
    });
    
    server->on("/rpc/Pump.SetPWM", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handlePumpSetPWM(request, this->pumpSetPWMBody);
        this->pumpSetPWMBody = ""; // Clear buffer
    }, NULL,
    [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        // Collect body data
        for (size_t i = 0; i < len; i++) {
            this->pumpSetPWMBody += (char)data[i];
        }
    });
    
    Serial.println("REST API endpoints registered:");
    Serial.println("  GET  /mada");
    Serial.println("  GET  /rpc/mada.GetStatus");
    Serial.println("  POST /rpc/Pump.Set");
    Serial.println("  POST /rpc/Pump.SetPWM");
}

//*********************************
// Endpoint 1: Device Info + Entity Metadata
// ESP32 sagt HA was was ist UND wo die Daten liegen!
//*********************************
void HomeAssistantIntegration::handleDeviceInfo(AsyncWebServerRequest *request) {
    StaticJsonDocument<2560> doc;
    
    // Device Information
    doc["name"] = DEVICE_NAME;
    doc["model"] = DEVICE_MODEL;
    doc["version"] = FIRMWARE_VERSION;
    doc["mac"] = mac_address;
    doc["id"] = device_id;
    doc["type"] = "irrigation_controller";
    doc["hostname"] = String(MDNS_HOSTNAME) + ".local";
    
    // Entity Definitions mit data_path für automatisches Mapping
    JsonArray entities = doc.createNestedArray("entities");
    
    // Sensor: Bodenfeuchte
    JsonObject soil_moisture = entities.createNestedObject();
    soil_moisture["id"] = "bodenfeuchte";
    soil_moisture["name"] = "Bodenfeuchte";
    soil_moisture["type"] = "sensor";
    soil_moisture["device_class"] = "moisture";
    soil_moisture["unit"] = "%";
    soil_moisture["state_class"] = "measurement";
    JsonArray sm_path = soil_moisture.createNestedArray("data_path");
    sm_path.add("soil");
    sm_path.add("moisture");
    
    // Sensor: Salzgehalt
    JsonObject salt = entities.createNestedObject();
    salt["id"] = "salzgehalt";
    salt["name"] = "Salzgehalt";
    salt["type"] = "sensor";
    salt["device_class"] = "voltage";
    salt["unit"] = "mV";
    salt["state_class"] = "measurement";
    JsonArray salt_path = salt.createNestedArray("data_path");
    salt_path.add("soil");
    salt_path.add("salt");
    
    // Sensor: Batterie
    JsonObject battery = entities.createNestedObject();
    battery["id"] = "batterie";
    battery["name"] = "Batterie";
    battery["type"] = "sensor";
    battery["device_class"] = "battery";
    battery["unit"] = "%";
    battery["state_class"] = "measurement";
    JsonArray bat_path = battery.createNestedArray("data_path");
    bat_path.add("battery");
    bat_path.add("percent");
    
    // Sensor: Temperatur (wenn vorhanden)
    if (sensorMgr->has_dht11 || sensorMgr->has_sht3xSensor || sensorMgr->has_ds18b20) {
        JsonObject temp = entities.createNestedObject();
        temp["id"] = "temperatur";
        temp["name"] = "Temperatur";
        temp["type"] = "sensor";
        temp["device_class"] = "temperature";
        temp["unit"] = "°C";
        temp["state_class"] = "measurement";
        JsonArray temp_path = temp.createNestedArray("data_path");
        temp_path.add("temperature");
        temp_path.add("value");
    }
    
    // Sensor: Luftfeuchtigkeit (wenn vorhanden)
    if (sensorMgr->has_dht11 || sensorMgr->has_sht3xSensor) {
        JsonObject humidity = entities.createNestedObject();
        humidity["id"] = "luftfeuchtigkeit";
        humidity["name"] = "Luftfeuchtigkeit";
        humidity["type"] = "sensor";
        humidity["device_class"] = "humidity";
        humidity["unit"] = "%";
        humidity["state_class"] = "measurement";
        JsonArray hum_path = humidity.createNestedArray("data_path");
        hum_path.add("humidity");
        hum_path.add("value");
    }
    
    // Sensor: Helligkeit (wenn vorhanden)
    if (sensorMgr->has_lightSensor) {
        JsonObject light = entities.createNestedObject();
        light["id"] = "helligkeit";
        light["name"] = "Helligkeit";
        light["type"] = "sensor";
        light["device_class"] = "illuminance";
        light["unit"] = "lx";
        light["state_class"] = "measurement";
        JsonArray light_path = light.createNestedArray("data_path");
        light_path.add("light");
        light_path.add("lux");
    }
    
    // Switch: Pumpe
    JsonObject pump_switch = entities.createNestedObject();
    pump_switch["id"] = "pumpe";
    pump_switch["name"] = "Pumpe";
    pump_switch["type"] = "switch";
    pump_switch["device_class"] = "switch";
    JsonArray pump_path = pump_switch.createNestedArray("data_path");
    pump_path.add("pump");
    pump_path.add("running");
    
    // Number: Pumpenleistung
    JsonObject pump_pwm = entities.createNestedObject();
    pump_pwm["id"] = "pumpenleistung";
    pump_pwm["name"] = "Pumpenleistung";
    pump_pwm["type"] = "number";
    pump_pwm["device_class"] = "power_factor";
    pump_pwm["unit"] = "%";
    pump_pwm["min"] = 0;
    pump_pwm["max"] = 100;
    pump_pwm["step"] = 1;
    JsonArray pwm_path = pump_pwm.createNestedArray("data_path");
    pwm_path.add("pump");
    pwm_path.add("pwm_target");
    
    String response;
    serializeJson(doc, response);
    
    request->send(200, "application/json", response);
}

//*********************************
// Endpoint 2: Get Status
//*********************************
void HomeAssistantIntegration::handleGetStatus(AsyncWebServerRequest *request) {
    String response;
    createStatusJSON(response);
    request->send(200, "application/json", response);
}

//*********************************
// Create Status JSON
//*********************************
void HomeAssistantIntegration::createStatusJSON(String& output) {
    StaticJsonDocument<2048> doc;
    higrow_sensors_event_t val = {0};
    
    // Soil Moisture
    sensorMgr->getSensorEvent(SOIL_SENSOR_ID, val);
    doc["soil"]["moisture"] = val.soli;
    
    // Salt Level
    sensorMgr->getSensorEvent(SALT_SENSOR_ID, val);
    doc["soil"]["salt"] = val.salt;
    
    // Battery
    sensorMgr->getSensorEvent(VOLTAGE_SENSOR_ID, val);
    doc["battery"]["voltage"] = val.voltage;
    int battPercent = map(val.voltage, 3300, 4200, 0, 100); if (battPercent > 100) battPercent = 100; if (battPercent < 0) battPercent = 0; doc["battery"]["percent"] = battPercent;
    
    // Temperature & Humidity
    if (sensorMgr->has_dht11) {
        sensorMgr->getSensorEvent(DHTxx_SENSOR_ID, val);
        doc["temperature"]["value"] = val.temperature;
        doc["temperature"]["source"] = "DHT11";
        doc["humidity"]["value"] = val.humidity;
        doc["humidity"]["source"] = "DHT11";
    } else if (sensorMgr->has_sht3xSensor) {
        sensorMgr->getSensorEvent(SHT3x_SENSOR_ID, val);
        doc["temperature"]["value"] = val.temperature;
        doc["temperature"]["source"] = "SHT3x";
        doc["humidity"]["value"] = val.humidity;
        doc["humidity"]["source"] = "SHT3x";
    } else if (sensorMgr->has_ds18b20) {
        sensorMgr->getSensorEvent(DS18B20_SENSOR_ID, val);
        doc["temperature"]["value"] = val.temperature;
        doc["temperature"]["source"] = "DS18B20";
    }
    
    // Light
    if (sensorMgr->has_lightSensor) {
        sensorMgr->getSensorEvent(BHT1750_SENSOR_ID, val);
        doc["light"]["lux"] = val.light;
    }
    
    // BME280 (wenn vorhanden)
    if (sensorMgr->has_bmeSensor) {
        sensorMgr->getSensorEvent(BME280_SENSOR_ID, val);
        doc["bme280"]["temperature"] = val.temperature;
        doc["bme280"]["humidity"] = val.humidity;
        doc["bme280"]["pressure"] = val.pressure;
        doc["bme280"]["altitude"] = val.altitude;
    }
    
    // Pump Status
    if (pumpControl) {
        doc["pump"]["running"] = pumpControl->isPumpRunning();
        doc["pump"]["pwm_target"] = pumpControl->getTargetPWM();
        doc["pump"]["pwm_active"] = pumpControl->getActivePWM();
    }
    
    // System Info
    doc["system"]["uptime"] = millis() / 1000;
    doc["system"]["wifi_rssi"] = WiFi.RSSI();
    doc["system"]["free_heap"] = ESP.getFreeHeap();
    
    serializeJson(doc, output);
}

//*********************************
// Endpoint 3: Pump Control
//*********************************
void HomeAssistantIntegration::handlePumpSet(AsyncWebServerRequest *request, String body) {
    if (!pumpControl) {
        request->send(503, "application/json", "{\"error\":\"Pump not available\"}");
        return;
    }
    
    Serial.print("Pump.Set received body: ");
    Serial.println(body);
    
    if (body.length() == 0) {
        request->send(400, "application/json", "{\"error\":\"Empty body\"}");
        return;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc["on"].is<bool>()) {
        request->send(400, "application/json", "{\"error\":\"Missing 'on' parameter\"}");
        return;
    }
    
    bool turnOn = doc["on"].as<bool>();
    pumpControl->setPumpState(turnOn);
    
    StaticJsonDocument<256> response;
    response["success"] = true;
    response["running"] = pumpControl->isPumpRunning();
    response["pwm_active"] = pumpControl->getActivePWM();
    
    String responseStr;
    serializeJson(response, responseStr);
    
    Serial.print("Pump ");
    Serial.println(turnOn ? "ON" : "OFF");
    
    request->send(200, "application/json", responseStr);
}

//*********************************
// Endpoint 4: Set PWM
//*********************************
void HomeAssistantIntegration::handlePumpSetPWM(AsyncWebServerRequest *request, String body) {
    if (!pumpControl) {
        request->send(503, "application/json", "{\"error\":\"Pump not available\"}");
        return;
    }
    
    Serial.print("Pump.SetPWM received body: ");
    Serial.println(body);
    
    if (body.length() == 0) {
        request->send(400, "application/json", "{\"error\":\"Empty body\"}");
        return;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, body);
    
    if (error) {
        Serial.print("JSON parse error: ");
        Serial.println(error.c_str());
        request->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }
    
    if (!doc["pwm"].is<int>()) {
        request->send(400, "application/json", "{\"error\":\"Missing 'pwm' parameter\"}");
        return;
    }
    
    int pwmValue = doc["pwm"].as<int>();
    
    if (pwmValue < 0 || pwmValue > 100) {
        request->send(400, "application/json", "{\"error\":\"PWM must be 0-100\"}");
        return;
    }
    
    pumpControl->setTargetPWM(pwmValue);
    
    StaticJsonDocument<256> response;
    response["success"] = true;
    response["pwm"] = pumpControl->getTargetPWM();
    
    String responseStr;
    serializeJson(response, responseStr);
    
    Serial.print("PWM set to: ");
    Serial.print(pwmValue);
    Serial.println("%");
    
    request->send(200, "application/json", responseStr);
}

String HomeAssistantIntegration::getMacAddress() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

String HomeAssistantIntegration::getDeviceId() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char idStr[13];
    sprintf(idStr, "%02x%02x%02x%02x%02x%02x", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(idStr);
}

void HomeAssistantIntegration::loop() {
    // mDNS wird automatisch von ESP32 Framework verwaltet
}

// EOF
