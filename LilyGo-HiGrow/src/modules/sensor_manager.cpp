// Filename: modules/sensor_manager.cpp
// V1.0
// V1.0 Initial - Sensor Verwaltung

#include "sensor_manager.h"
#include <algorithm>

//*********************************
// Konstruktor
//*********************************
SensorManager::SensorManager() 
    : has_lightSensor(false), has_bmeSensor(false), has_dht11(false), 
      has_sht3xSensor(false), has_ds18b20(false) {
    
    lightMeter = new BH1750(OB_BH1750_ADDRESS);
    dht = new DHT(DHT1x_PIN, DHTTYPE);
    bme = new Adafruit_BME280();
    sht31 = new Adafruit_SHT31();
    ds = new OneWire();
}

//*********************************
// Initialisierung
//*********************************
void SensorManager::init(TwoWire &wire, TwoWire &wire1) {
    Serial.println("-------------Devices probe-------------");
    deviceProbe(wire);
    deviceProbe(wire1);

    if (!dhtSensorProbe()) {
        has_dht11 = false;
        Serial.println("Warning: Failed to find DHT11!");
    } else {
        has_dht11 = true;
        Serial.println("DHT11 found");
    }

    if (has_sht3xSensor) {
        if (!sht31->begin(OB_SHT3X_ADDRESS)) {
            Serial.println("Warning: Failed to find SHT3x!");
        } else {
            has_sht3xSensor = false;
            Serial.println("SHT3X found");
        }
    } else {
        if (ds18b20Begin()) {
            has_ds18b20 = true;
            Serial.println("DS18B20 found");
        } else {
            has_ds18b20 = false;
            Serial.println("Warning: Failed to find DS18B20!");
        }
    }

    if (has_lightSensor) {
        if (!lightMeter->begin()) {
            has_lightSensor = false;
            Serial.println("Warning: Failed to find BH1750!");
        } else {
            Serial.println("BH1750 found");
        }
    }

    if (has_bmeSensor) {
        if (!bme->begin()) {
            Serial.println("Warning: Failed to find BME280");
        } else {
            Serial.println("BME280 found");
            has_bmeSensor = true;
        }
    }
}

//*********************************
// I2C Device Probe
//*********************************
void SensorManager::deviceProbe(TwoWire &t) {
    uint8_t err, addr;
    for (addr = 1; addr < 127; addr++) {
        t.beginTransmission(addr);
        err = t.endTransmission();
        if (err == 0) {
            switch (addr) {
            case OB_BH1750_ADDRESS:
                has_lightSensor = true;
                Serial.println("BH1750 light sensor found!");
                break;
            case OB_BME280_ADDRESS:
                has_bmeSensor = true;
                Serial.println("BME280 found!");
                break;
            case OB_SHT3X_ADDRESS:
                has_sht3xSensor = true;
                Serial.println("SHT3X found!");
                break;
            default:
                Serial.print("I2C device at 0x");
                if (addr < 16) Serial.print("0");
                Serial.print(addr, HEX);
                Serial.println(" !");
                break;
            }
        }
    }
}

//*********************************
// Sensor Event abrufen
//*********************************
bool SensorManager::getSensorEvent(sensor_id_t id, higrow_sensors_event_t &val) {
    switch (id) {
    case BME280_SENSOR_ID:
        val.temperature = bme->readTemperature();
        val.humidity = bme->readHumidity();
        val.pressure = (bme->readPressure() / 100.0F);
        val.altitude = bme->readAltitude(1013.25);
        break;

    case SHT3x_SENSOR_ID:
        val.temperature = sht31->readTemperature();
        val.humidity = sht31->readHumidity();
        break;

    case DHTxx_SENSOR_ID:
        val.temperature = dht->readTemperature();
        val.humidity = dht->readHumidity();
        if (isnan(val.temperature)) val.temperature = 0.0;
        if (isnan(val.humidity)) val.humidity = 0.0;
        break;

    case BHT1750_SENSOR_ID:
        val.light = lightMeter->readLightLevel();
        if (isnan(val.light)) val.light = 0.0;
        break;

    case SOIL_SENSOR_ID: {
        uint16_t soil = analogRead(SOIL_PIN);
        val.soli = map(soil, 0, 4095, 100, 0);
        break;
    }

    case SALT_SENSOR_ID: {
        uint8_t samples = 120;
        uint32_t humi = 0;
        uint16_t array[120];
        for (int i = 0; i < samples; i++) {
            array[i] = analogRead(SALT_PIN);
            delay(2);
        }
        std::sort(array, array + samples);
        for (int i = 1; i < samples - 1; i++) {
            humi += array[i];
        }
        humi /= samples - 2;
        val.salt = humi;
        break;
    }

    case DS18B20_SENSOR_ID:
        val.temperature = getDsTemperature();
        if (isnan(val.temperature) || val.temperature > 125.0) {
            val.temperature = 0;
        }
        break;

    case VOLTAGE_SENSOR_ID: {
        int vref = 1100;
        uint16_t volt = analogRead(BAT_ADC);
        val.voltage = ((float)volt / 4095.0) * 6.6 * (vref);
        break;
    }

    default:
        break;
    }
    return true;
}

//*********************************
// DHT Sensor Probe
//*********************************
bool SensorManager::dhtSensorProbe() {
    dht->begin();
    delay(2000);
    int i = 5;
    while (i--) {
        float h = dht->readHumidity();
        if (!isnan(h)) return true;
        delay(500);
    }
    return false;
}

//*********************************
// DS18B20 Init
//*********************************
bool SensorManager::ds18b20Begin() {
    ds->begin(DS18B20_PIN);

    if (!ds->search(ds18b20Addr)) {
        ds->reset_search();
        return false;
    }

    if (OneWire::crc8(ds18b20Addr, 7) != ds18b20Addr[7]) {
        Serial.println("CRC invalid!");
        return false;
    }

    switch (ds18b20Addr[0]) {
    case 0x10: ds18b20Type = 1; break;
    case 0x28: ds18b20Type = 0; break;
    case 0x22: ds18b20Type = 0; break;
    default: return false;
    }

    return true;
}

//*********************************
// DS18B20 Temperatur lesen
//*********************************
float SensorManager::getDsTemperature() {
    uint8_t data[9];
    ds->reset();
    ds->select(ds18b20Addr);
    ds->write(0x44, 1);
    delay(1000);

    ds->reset();
    ds->select(ds18b20Addr);
    ds->write(0xBE);

    for (int i = 0; i < 9; i++) {
        data[i] = ds->read();
    }

    int16_t raw = (data[1] << 8) | data[0];
    if (ds18b20Type) {
        raw = raw << 3;
        if (data[7] == 0x10) {
            raw = (raw & 0xFFF0) + 12 - data[6];
        }
    } else {
        byte cfg = (data[4] & 0x60);
        if (cfg == 0x00) raw = raw & ~7;
        else if (cfg == 0x20) raw = raw & ~3;
        else if (cfg == 0x40) raw = raw & ~1;
    }
    return (float)raw / 16.0;
}

//EOF
