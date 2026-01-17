// Filename: modules/sensor_manager.h
// V1.0
// V1.0 Initial - Sensor Verwaltung

#pragma once
#include <Arduino.h>
#include <BH1750.h>
#include <DHT.h>
#include <Adafruit_BME280.h>
#include <Adafruit_SHT31.h>
#include <OneWire.h>
#include "configuration.h"

typedef enum {
    BME280_SENSOR_ID,
    DHTxx_SENSOR_ID,
    SHT3x_SENSOR_ID,
    BHT1750_SENSOR_ID,
    SOIL_SENSOR_ID,
    SALT_SENSOR_ID,
    DS18B20_SENSOR_ID,
    VOLTAGE_SENSOR_ID,
} sensor_id_t;

typedef struct {
    uint32_t timestamp;
    float temperature;
    float light;
    float pressure;
    float humidity;
    float altitude;
    float voltage;
    uint8_t soli;
    uint8_t salt;
} higrow_sensors_event_t;

class SensorManager {
private:
    BH1750* lightMeter;
    DHT* dht;
    Adafruit_BME280* bme;
    Adafruit_SHT31* sht31;
    OneWire* ds;
    
    uint8_t ds18b20Addr[8];
    uint8_t ds18b20Type;

public:
    bool has_lightSensor;
    bool has_bmeSensor;
    bool has_dht11;
    bool has_sht3xSensor;
    bool has_ds18b20;

    SensorManager();
    void init(TwoWire &wire, TwoWire &wire1);
    void deviceProbe(TwoWire &t);
    bool getSensorEvent(sensor_id_t id, higrow_sensors_event_t &val);

private:
    bool dhtSensorProbe();
    bool ds18b20Begin();
    float getDsTemperature();
};

//EOF
