"""Sensor platform for MADA integration."""
from __future__ import annotations

from homeassistant.components.sensor import (
    SensorDeviceClass,
    SensorEntity,
    SensorStateClass,
)
from homeassistant.config_entries import ConfigEntry
from homeassistant.const import (
    PERCENTAGE,
    UnitOfElectricPotential,
    UnitOfTemperature,
    SIGNAL_STRENGTH_DECIBELS_MILLIWATT,
    UnitOfTime,
)
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from . import DOMAIN, MADADataUpdateCoordinator


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    """Set up MADA sensors."""
    coordinator = hass.data[DOMAIN][entry.entry_id]
    
    sensors = [
        MADASensor(coordinator, entry, "soil_moisture", "Bodenfeuchte", "soil", "moisture"),
        MADASensor(coordinator, entry, "soil_salt", "Salz", "soil", "salt", "µS/cm"),
        MADASensor(coordinator, entry, "battery_voltage", "Batterie Spannung", "battery", "voltage"),
        MADASensor(coordinator, entry, "battery_percent", "Batterie", "battery", "percent"),
        MADASensor(coordinator, entry, "temperature", "Temperatur", "temperature", "value"),
        MADASensor(coordinator, entry, "humidity", "Luftfeuchtigkeit", "humidity", "value"),
        MADASensor(coordinator, entry, "light", "Helligkeit", "light", "lux", "lx"),
        MADASensor(coordinator, entry, "wifi_rssi", "WiFi Signal", "system", "wifi_rssi"),
        MADASensor(coordinator, entry, "uptime", "Uptime", "system", "uptime"),
        MADASensor(coordinator, entry, "pwm_target", "Pumpenleistung Ziel", "pump", "pwm_target"),
        MADASensor(coordinator, entry, "pwm_active", "Pumpenleistung Aktiv", "pump", "pwm_active"),
    ]
    
    async_add_entities(sensors)


class MADASensor(CoordinatorEntity, SensorEntity):
    """Representation of a MADA sensor."""

    def __init__(
        self,
        coordinator: MADADataUpdateCoordinator,
        entry: ConfigEntry,
        sensor_id: str,
        name: str,
        data_key: str,
        data_subkey: str,
        unit: str | None = None,
    ) -> None:
        """Initialize the sensor."""
        super().__init__(coordinator)
        
        self._attr_unique_id = f"{entry.entry_id}_{sensor_id}"
        self._attr_name = f"MADA {name}"
        self._data_key = data_key
        self._data_subkey = data_subkey
        
        # Device info
        self._attr_device_info = {
            "identifiers": {(DOMAIN, entry.entry_id)},
            "name": "MADA Bewässerung",
            "manufacturer": "LilyGo",
            "model": entry.data.get("model", "MADA v1.1"),
            "sw_version": entry.data.get("version", "1.2-HA"),
        }
        
        # Sensor-specific configuration
        if sensor_id == "soil_moisture":
            self._attr_device_class = SensorDeviceClass.MOISTURE
            self._attr_native_unit_of_measurement = PERCENTAGE
            self._attr_state_class = SensorStateClass.MEASUREMENT
            self._attr_icon = "mdi:water-percent"
            
        elif sensor_id == "soil_salt":
            self._attr_native_unit_of_measurement = unit or "µS/cm"
            self._attr_state_class = SensorStateClass.MEASUREMENT
            self._attr_icon = "mdi:shaker"
            
        elif sensor_id == "battery_voltage":
            self._attr_device_class = SensorDeviceClass.VOLTAGE
            self._attr_native_unit_of_measurement = UnitOfElectricPotential.MILLIVOLT
            self._attr_state_class = SensorStateClass.MEASUREMENT
            
        elif sensor_id == "battery_percent":
            self._attr_device_class = SensorDeviceClass.BATTERY
            self._attr_native_unit_of_measurement = PERCENTAGE
            self._attr_state_class = SensorStateClass.MEASUREMENT
            
        elif sensor_id == "temperature":
            self._attr_device_class = SensorDeviceClass.TEMPERATURE
            self._attr_native_unit_of_measurement = UnitOfTemperature.CELSIUS
            self._attr_state_class = SensorStateClass.MEASUREMENT
            
        elif sensor_id == "humidity":
            self._attr_device_class = SensorDeviceClass.HUMIDITY
            self._attr_native_unit_of_measurement = PERCENTAGE
            self._attr_state_class = SensorStateClass.MEASUREMENT
            
        elif sensor_id == "light":
            self._attr_device_class = SensorDeviceClass.ILLUMINANCE
            self._attr_native_unit_of_measurement = unit or "lx"
            self._attr_state_class = SensorStateClass.MEASUREMENT
            
        elif sensor_id == "wifi_rssi":
            self._attr_device_class = SensorDeviceClass.SIGNAL_STRENGTH
            self._attr_native_unit_of_measurement = SIGNAL_STRENGTH_DECIBELS_MILLIWATT
            self._attr_state_class = SensorStateClass.MEASUREMENT
            self._attr_entity_registry_enabled_default = False
            
        elif sensor_id == "uptime":
            self._attr_device_class = SensorDeviceClass.DURATION
            self._attr_native_unit_of_measurement = UnitOfTime.SECONDS
            self._attr_state_class = SensorStateClass.TOTAL_INCREASING
            self._attr_entity_registry_enabled_default = False
            
        elif sensor_id in ("pwm_target", "pwm_active"):
            self._attr_native_unit_of_measurement = PERCENTAGE
            self._attr_state_class = SensorStateClass.MEASUREMENT
            self._attr_icon = "mdi:gauge"

    @property
    def native_value(self):
        """Return the state of the sensor."""
        if self.coordinator.data is None:
            return None
            
        try:
            value = self.coordinator.data.get(self._data_key, {}).get(self._data_subkey)
            return value
        except (KeyError, TypeError):
            return None
