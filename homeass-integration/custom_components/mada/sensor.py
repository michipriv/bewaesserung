"""Sensor platform for MADA integration using ESP32 entity metadata."""
# V1.5 Nutzt data_path aus Entity-Metadaten - automatisches Mapping!
# V1.4 Verbessertes Mapping
# V1.3 Nutzt Entity-Metadaten vom ESP32
# V1.2 Dynamische Sensor-Erkennung
# V1.1 Initial

from __future__ import annotations

import logging

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
)
from homeassistant.core import HomeAssistant
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from . import DOMAIN

_LOGGER = logging.getLogger(__name__)

# Mapping von ESP32 device_class strings zu HA SensorDeviceClass
DEVICE_CLASS_MAP = {
    "moisture": SensorDeviceClass.MOISTURE,
    "voltage": SensorDeviceClass.VOLTAGE,
    "battery": SensorDeviceClass.BATTERY,
    "temperature": SensorDeviceClass.TEMPERATURE,
    "humidity": SensorDeviceClass.HUMIDITY,
    "illuminance": SensorDeviceClass.ILLUMINANCE,
}

# Mapping von ESP32 state_class strings zu HA SensorStateClass
STATE_CLASS_MAP = {
    "measurement": SensorStateClass.MEASUREMENT,
    "total": SensorStateClass.TOTAL,
    "total_increasing": SensorStateClass.TOTAL_INCREASING,
}


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    """Set up MADA sensors using ESP32 metadata."""
    data = hass.data[DOMAIN][entry.entry_id]
    coordinator = data["coordinator"]
    entity_metadata = data["entity_metadata"]
    
    sensors = []
    
    # Erstelle Sensoren basierend auf ESP32 Metadaten
    for entity_id, metadata in entity_metadata.items():
        entity_type = metadata.get("type")
        
        # Nur Sensoren verarbeiten
        if entity_type == "sensor":
            _LOGGER.info(
                "Creating sensor from metadata: %s (%s) at path %s",
                metadata.get("name", entity_id),
                entity_id,
                metadata.get("data_path", "unknown")
            )
            
            sensors.append(
                MadaSensorFromMetadata(
                    coordinator=coordinator,
                    entry=entry,
                    entity_id=entity_id,
                    metadata=metadata,
                )
            )
    
    _LOGGER.info("Created %d sensors from ESP32 metadata", len(sensors))
    async_add_entities(sensors)


class MadaSensorFromMetadata(CoordinatorEntity, SensorEntity):
    """Sensor created from ESP32 entity metadata."""

    def __init__(
        self,
        coordinator,
        entry: ConfigEntry,
        entity_id: str,
        metadata: dict,
    ) -> None:
        """Initialize the sensor."""
        super().__init__(coordinator)
        
        self._entity_id = entity_id
        self._metadata = metadata
        self._data_path = metadata.get("data_path", [])
        
        # Unique ID und Name
        self._attr_unique_id = f"{entry.entry_id}_{entity_id}"
        self._attr_name = f"MADA {metadata.get('name', entity_id)}"
        
        # Device info
        self._attr_device_info = {
            "identifiers": {(DOMAIN, entry.entry_id)},
            "name": "MADA Bewässerung",
            "manufacturer": "Custom",
            "model": entry.data.get("model", "HiGrow"),
            "sw_version": entry.data.get("version", "1.4"),
        }
        
        # Device Class (aus ESP32 Metadaten)
        device_class_str = metadata.get("device_class")
        if device_class_str and device_class_str in DEVICE_CLASS_MAP:
            self._attr_device_class = DEVICE_CLASS_MAP[device_class_str]
        
        # State Class (aus ESP32 Metadaten)
        state_class_str = metadata.get("state_class")
        if state_class_str and state_class_str in STATE_CLASS_MAP:
            self._attr_state_class = STATE_CLASS_MAP[state_class_str]
        
        # Unit (aus ESP32 Metadaten)
        unit = metadata.get("unit")
        if unit:
            self._attr_native_unit_of_measurement = unit
        
        # Icon (optional aus ESP32 Metadaten)
        icon = metadata.get("icon")
        if icon:
            self._attr_icon = icon

    @property
    def native_value(self):
        """Return the state of the sensor."""
        if self.coordinator.data is None:
            return None
        
        # Verwende data_path aus Metadaten
        if not self._data_path or len(self._data_path) < 2:
            _LOGGER.warning(f"No valid data_path for {self._entity_id}")
            return None
        
        try:
            # Navigate durch JSON mit data_path
            # z.B. ["soil", "moisture"] -> data["soil"]["moisture"]
            value = self.coordinator.data
            for key in self._data_path:
                value = value.get(key, {})
                if value == {}:
                    return None
            
            return value
            
        except (KeyError, TypeError, AttributeError) as e:
            _LOGGER.debug(f"Could not get value for {self._entity_id} from {self._data_path}: {e}")
            return None
