// Filename: homeassistant.h
// Home Assistant Integration nach Shelly Gen2 Prinzip
// mDNS + HTTP JSON-RPC API

#ifndef HOMEASSISTANT_H
#define HOMEASSISTANT_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "sensor_manager.h"
#include "pwm_control.h"

// mDNS Hostname (higrow.local)
#define MDNS_HOSTNAME "higrow"

// Device Information
#define DEVICE_NAME "HiGrow"
#define DEVICE_MODEL "LilyGo-HiGrow-v1.1"
#define FIRMWARE_VERSION "1.4-HA"

class HomeAssistantIntegration {
private:
    AsyncWebServer* server;
    SensorManager* sensorMgr;
    PWMControl* pumpControl;
    
    String mac_address;
    String device_id;
    
    // Body buffers for POST requests
    String pumpSetBody;
    String pumpSetPWMBody;
    
    // Endpoint Handlers
    void handleDeviceInfo(AsyncWebServerRequest *request);
    void handleGetStatus(AsyncWebServerRequest *request);
    void handlePumpSet(AsyncWebServerRequest *request, String body);
    void handlePumpSetPWM(AsyncWebServerRequest *request, String body);
    
    // Helper Functions
    String getMacAddress();
    String getDeviceId();
    void createStatusJSON(String& output);
    
public:
    HomeAssistantIntegration(AsyncWebServer* srv, SensorManager* sensors, PWMControl* pump);
    
    // Initialization
    bool init();
    
    // mDNS Setup
    bool setupMDNS();
    
    // REST API Endpoints Setup
    void setupRestAPI();
    
    // Loop (for mDNS updates if needed)
    void loop();
};

#endif // HOMEASSISTANT_H
