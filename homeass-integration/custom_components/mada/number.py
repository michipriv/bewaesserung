"""Number platform for MADA integration using ESP32 entity metadata."""
# V1.5 Nutzt data_path aus Entity-Metadaten - automatisches Mapping!
# V1.4 Verbessertes Mapping
# V1.3 Nutzt Entity-Metadaten vom ESP32
# V1.2 Smart Number Discovery
# V1.1 Initial

from __future__ import annotations

import logging

import aiohttp
import async_timeout

from homeassistant.components.number import NumberEntity, NumberMode
from homeassistant.config_entries import ConfigEntry
from homeassistant.core import HomeAssistant
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers.entity_platform import AddEntitiesCallback
from homeassistant.helpers.update_coordinator import CoordinatorEntity

from . import DOMAIN

_LOGGER = logging.getLogger(__name__)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    """Set up MADA number entities using ESP32 metadata."""
    data = hass.data[DOMAIN][entry.entry_id]
    coordinator = data["coordinator"]
    entity_metadata = data["entity_metadata"]
    
    numbers = []
    
    # Erstelle Number-Entities basierend auf ESP32 Metadaten
    for entity_id, metadata in entity_metadata.items():
        entity_type = metadata.get("type")
        
        # Nur Numbers verarbeiten
        if entity_type == "number":
            _LOGGER.info(
                "Creating number from metadata: %s (%s) at path %s",
                metadata.get("name", entity_id),
                entity_id,
                metadata.get("data_path", "unknown")
            )
            
            numbers.append(
                MadaNumberFromMetadata(
                    coordinator=coordinator,
                    entry=entry,
                    entity_id=entity_id,
                    metadata=metadata,
                )
            )
    
    _LOGGER.info("Created %d number entities from ESP32 metadata", len(numbers))
    async_add_entities(numbers)


class MadaNumberFromMetadata(CoordinatorEntity, NumberEntity):
    """Number entity created from ESP32 entity metadata."""

    def __init__(
        self,
        coordinator,
        entry: ConfigEntry,
        entity_id: str,
        metadata: dict,
    ) -> None:
        """Initialize the number entity."""
        super().__init__(coordinator)
        
        self._entity_id = entity_id
        self._metadata = metadata
        self._data_path = metadata.get("data_path", [])
        self._host = entry.data["host"]
        self._session = async_get_clientsession(coordinator.hass)
        
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
        
        # Number-Eigenschaften (aus ESP32 Metadaten)
        self._attr_native_min_value = metadata.get("min", 0)
        self._attr_native_max_value = metadata.get("max", 100)
        self._attr_native_step = metadata.get("step", 1)
        self._attr_native_unit_of_measurement = metadata.get("unit")
        self._attr_mode = NumberMode.SLIDER
        
        # Icon (aus Metadaten oder default)
        self._attr_icon = metadata.get("icon", "mdi:numeric")

    @property
    def native_value(self) -> float | None:
        """Return the current value."""
        if self.coordinator.data is None:
            return None
        
        # Verwende data_path aus Metadaten
        if not self._data_path or len(self._data_path) < 2:
            _LOGGER.warning(f"No valid data_path for {self._entity_id}")
            return None
        
        try:
            # Navigate durch JSON mit data_path
            value = self.coordinator.data
            for key in self._data_path:
                value = value.get(key, {})
                if value == {}:
                    return None
            
            return value
            
        except (KeyError, TypeError, AttributeError) as e:
            _LOGGER.debug(f"Could not get value for {self._entity_id} from {self._data_path}: {e}")
            return None

    async def async_set_native_value(self, value: float) -> None:
        """Set new value."""
        try:
            async with async_timeout.timeout(10):
                # Endpoint basierend auf entity_id
                # pumpenleistung -> /rpc/Pump.SetPWM
                if "pumpenleistung" in self._entity_id.lower():
                    url = f"http://{self._host}/rpc/Pump.SetPWM"
                    payload_key = "pwm"
                else:
                    endpoint_name = self._entity_id.capitalize()
                    url = f"http://{self._host}/rpc/{endpoint_name}.Set"
                    payload_key = "value"
                
                # Integer oder Float basierend auf step
                if self._attr_native_step == 1:
                    payload_value = int(value)
                else:
                    payload_value = value
                
                payload = {payload_key: payload_value}
                
                _LOGGER.debug(f"Sending {payload} to {url}")
                
                async with self._session.post(
                    url,
                    json=payload,
                    headers={"Content-Type": "application/json"},
                ) as response:
                    if response.status == 200:
                        await self.coordinator.async_request_refresh()
                    else:
                        _LOGGER.error(
                            "Failed to set %s: HTTP %s",
                            self._attr_name,
                            response.status,
                        )
                        
        except aiohttp.ClientError as err:
            _LOGGER.error("Error setting %s: %s", self._attr_name, err)
        except Exception as err:
            _LOGGER.error("Unexpected error setting %s: %s", self._attr_name, err)
