// Filename: src.ino
// V1.1
// V1.1 Code aufgeteilt: PWM + Sensor Module, Button/Slider Logik korrigiert
// V1.0 Initial

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <Button2.h>
#include <LoRa.h>
#include <Adafruit_NeoPixel.h>
#include <cJSON.h>
#include "configuration.h"
#include "modules/pwm_control.h"
#include "modules/sensor_manager.h"

#define TIME_TO_SLEEP 3 * 60 * 1000

AsyncWebServer server(80);
ESPDash dashboard(&server);

Button2 button(BOOT_PIN);
Button2 useButton(USER_BUTTON);

Adafruit_NeoPixel* pixels = NULL;
PWMControl* pumpControl = NULL;
SensorManager* sensorMgr = NULL;

bool has_lora_shield = false;
uint64_t timestamp = 0;

Card *dhtTemperature = NULL;
Card *dhtHumidity = NULL;
Card *saltValue = NULL;
Card *batteryValue = NULL;
Card *soilValue = NULL;
Card *illumination = NULL;
Card *bmeTemperature = NULL;
Card *bmeHumidity = NULL;
Card *bmeAltitude = NULL;
Card *bmePressure = NULL;
Card *dsTemperature = NULL;
Card *sht3xTemperature = NULL;
Card *sht3xHumidity = NULL;
Card *motorButton = NULL;
Card *pumpPWMSlider = NULL;
Card *pumpPWMValue = NULL;

void setupLoRa();
void loopLoRa(higrow_sensors_event_t *val);
void setupWiFi();
void deviceSleep();
void smartConfigStart(Button2 &b);
void sleepHandler(Button2 &b);

//*********************************
// SmartConfig WiFi
//*********************************
void smartConfigStart(Button2 &b) {
    Serial.println("SmartConfig started");
    WiFi.disconnect();
    WiFi.beginSmartConfig();
    while (!WiFi.smartConfigDone()) {
        Serial.print(".");
        delay(200);
    }
    WiFi.stopSmartConfig();
    Serial.println();
    Serial.print("Connected: ");
    Serial.print(WiFi.SSID());
    Serial.print(" PSW: ");
    Serial.println(WiFi.psk());
}

//*********************************
// Sleep Mode
//*********************************
void deviceSleep() {
    Serial.println("Entering DeepSleep...");
    
    if (has_lora_shield) {
        LoRa.sleep();
        SPI.end();
    }

    esp_sleep_enable_ext1_wakeup(_BV(35), ESP_EXT1_WAKEUP_ALL_LOW);
    delay(1000);
    esp_deep_sleep_start();
}

void sleepHandler(Button2 &b) {
    deviceSleep();
}

TimerHandle_t sleepTimer;

void wifi_ap_connect_timeout(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.println("WiFi Client connected");
    xTimerDelete(sleepTimer, portMAX_DELAY);
}

//*********************************
// WiFi Setup
//*********************************
void setupWiFi() {
#ifdef SOFTAP_MODE
    Serial.println("Configuring AP...");
    uint8_t mac[6];
    char buff[128];
    WiFi.macAddress(mac);
    sprintf(buff, "T-Higrow-%02X:%02X", mac[4], mac[5]);
    WiFi.softAP(buff);
    Serial.printf("AP: %s -> 192.168.4.1\n", buff);

    WiFi.onEvent(wifi_ap_connect_timeout, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STACONNECTED);

    sleepTimer = xTimerCreate("timer", pdTICKS_TO_MS(TIME_TO_SLEEP), pdFALSE, NULL, [](TimerHandle_t timer) {
        WiFi.mode(WIFI_MODE_NULL);
        deviceSleep();
    });

    xTimerStart(sleepTimer, portMAX_DELAY);
#else
    WiFi.mode(WIFI_STA);
    Serial.print("Connecting to ");
    Serial.print(WIFI_SSID);
    Serial.print("...");

    WiFi.begin(WIFI_SSID, WIFI_PASSWD);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("WiFi failed!");
        delay(3000);
        esp_restart();
    }
    Serial.println("OK");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
#endif
    server.begin();
}

//*********************************
// LoRa Setup
//*********************************
void setupLoRa() {
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    LoRa.setPins(RADIO_CS_PIN, RADIO_RESET_PIN, RADIO_DI0_PIN);
    if (!LoRa.begin(LoRa_frequency)) {
        SPI.end();
        return;
    }
    has_lora_shield = true;
}

//*********************************
// LoRa Loop
//*********************************
void loopLoRa(higrow_sensors_event_t *val) {
    if (!has_lora_shield) return;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "L", String(val->light).c_str());
    cJSON_AddStringToObject(root, "S", String(val->soli).c_str());
    cJSON_AddStringToObject(root, "A", String(val->salt).c_str());
    cJSON_AddStringToObject(root, "V", String(val->voltage).c_str());
    cJSON_AddStringToObject(root, "T", String(val->temperature).c_str());
    cJSON_AddStringToObject(root, "PWM", String(pumpControl->getTargetPWM()).c_str());

    LoRa.beginPacket();
    char *packet = cJSON_Print(root);
    LoRa.print(packet);
    LoRa.endPacket();

    cJSON_Delete(root);
}

//*********************************
// Setup
//*********************************
void setup() {
    Serial.begin(115200);

    button.setLongClickHandler(smartConfigStart);
    useButton.setLongClickHandler(sleepHandler);

    pinMode(POWER_CTRL, OUTPUT);
    digitalWrite(POWER_CTRL, HIGH);
    delay(100);

    Wire.begin(I2C_SDA, I2C_SCL);
    Wire1.begin(I2C1_SDA, I2C1_SCL);

    sensorMgr = new SensorManager();
    sensorMgr->init(Wire, Wire1);

    saltValue = new Card(&dashboard, GENERIC_CARD, DASH_SALT_VALUE_STRING, "%");
    batteryValue = new Card(&dashboard, GENERIC_CARD, DASH_BATTERY_STRING, "mV");
    soilValue = new Card(&dashboard, GENERIC_CARD, DASH_SOIL_VALUE_STRING, "%");

    if (sensorMgr->has_dht11) {
        dhtHumidity = new Card(&dashboard, HUMIDITY_CARD, DASH_DHT_HUMIDITY_STRING, "%");
        dhtTemperature = new Card(&dashboard, TEMPERATURE_CARD, DASH_DHT_TEMPERATURE_STRING, "°C");
    }

    if (sensorMgr->has_sht3xSensor) {
        sht3xTemperature = new Card(&dashboard, TEMPERATURE_CARD, DASH_SHT3X_TEMPERATURE_STRING, "°C");
        sht3xHumidity = new Card(&dashboard, HUMIDITY_CARD, DASH_SHT3X_HUMIDITY_STRING, "%");
    } else if (sensorMgr->has_ds18b20) {
        dsTemperature = new Card(&dashboard, TEMPERATURE_CARD, DASH_DS18B20_STRING, "°C");
    }

    if (sensorMgr->has_lightSensor) {
        illumination = new Card(&dashboard, GENERIC_CARD, DASH_BH1750_LUX_STRING, "lx");
    }

    if (sensorMgr->has_bmeSensor) {
        bmeTemperature = new Card(&dashboard, TEMPERATURE_CARD, DASH_BME280_TEMPERATURE_STRING, "°C");
        bmeHumidity = new Card(&dashboard, HUMIDITY_CARD, DASH_BME280_HUMIDITY_STRING, "%");
        bmeAltitude = new Card(&dashboard, GENERIC_CARD, DASH_BME280_ALTITUDE_STRING, "m");
        bmePressure = new Card(&dashboard, GENERIC_CARD, DASH_BME280_PRESSURE_STRING, "hPa");
    }

    setupLoRa();

    if (has_lora_shield) {
        Serial.println("LoRa shield OK");
    } else {
        Serial.println("LoRa not found -> PWM Init");

        pixels = new Adafruit_NeoPixel(1, RGB_PIN, NEO_GRB + NEO_KHZ800);
        if (pixels) {
            pixels->begin();
            pixels->setBrightness(50);
            pixels->setPixelColor(0, pixels->Color(255, 0, 0)); pixels->show(); delay(1000);
            pixels->setPixelColor(0, pixels->Color(0, 255, 0)); pixels->show(); delay(1000);
            pixels->setPixelColor(0, pixels->Color(0, 0, 255)); pixels->show(); delay(1000);
            pixels->setPixelColor(0, 0); pixels->show();
        }

        pumpControl = new PWMControl(MOTOR_PIN, pixels);
        pumpControl->init();

        motorButton = new Card(&dashboard, BUTTON_CARD, "Pumpe EIN/AUS");
        motorButton->attachCallback([&](bool value) {
            Serial.print("Button: ");
            Serial.println(value ? "ON" : "OFF");
            
            pumpControl->setPumpState(value);
            
            motorButton->update(value);
            pumpPWMValue->update(pumpControl->getActivePWM());
            dashboard.sendUpdates();
        });

        pumpPWMSlider = new Card(&dashboard, SLIDER_CARD, "Pumpenleistung", "%", 0, 100);
        pumpPWMSlider->attachCallback([&](int value) {
            Serial.print("Slider: ");
            Serial.print(value);
            Serial.println("%");
            
            pumpControl->setTargetPWM(value);
            
            pumpPWMSlider->update(value);
            pumpPWMValue->update(pumpControl->getActivePWM());
            dashboard.sendUpdates();
        });

        pumpPWMValue = new Card(&dashboard, GENERIC_CARD, "Aktuelle PWM", "%");
        pumpPWMValue->update(pumpControl->getActivePWM());
        pumpPWMSlider->update(pumpControl->getTargetPWM());
        motorButton->update(false);
    }

    setupWiFi();
}

//*********************************
// Loop
//*********************************
void loop() {
    button.loop();
    useButton.loop();

    if (millis() - timestamp > 1000) {
        timestamp = millis();

        higrow_sensors_event_t val = {0};

        sensorMgr->getSensorEvent(SOIL_SENSOR_ID, val);
        soilValue->update(val.soli);

        sensorMgr->getSensorEvent(SALT_SENSOR_ID, val);
        saltValue->update(val.salt);

        sensorMgr->getSensorEvent(VOLTAGE_SENSOR_ID, val);
        batteryValue->update(val.voltage);

        if (sensorMgr->has_dht11) {
            sensorMgr->getSensorEvent(DHTxx_SENSOR_ID, val);
            dhtTemperature->update(val.temperature);
            dhtHumidity->update(val.humidity);
        }

        if (sensorMgr->has_lightSensor) {
            sensorMgr->getSensorEvent(BHT1750_SENSOR_ID, val);
            illumination->update(val.light);
        }

        if (sensorMgr->has_bmeSensor) {
            sensorMgr->getSensorEvent(BME280_SENSOR_ID, val);
            bmeTemperature->update(val.temperature);
            bmeHumidity->update(val.humidity);
            bmeAltitude->update(val.altitude);
            bmePressure->update(val.pressure);
        }

        if (sensorMgr->has_sht3xSensor) {
            sensorMgr->getSensorEvent(SHT3x_SENSOR_ID, val);
            sht3xTemperature->update(val.temperature);
            sht3xHumidity->update(val.humidity);
        }

        if (sensorMgr->has_ds18b20) {
            sensorMgr->getSensorEvent(DS18B20_SENSOR_ID, val);
            dsTemperature->update(val.temperature);
        }

        dashboard.sendUpdates();
        loopLoRa(&val);
    }
}

//EOF
