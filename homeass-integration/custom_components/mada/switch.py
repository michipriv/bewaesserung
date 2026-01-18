"""Switch platform for MADA integration using ESP32 entity metadata."""
# V1.5 Nutzt data_path aus Entity-Metadaten - automatisches Mapping!
# V1.4 Verbessertes Mapping
# V1.3 Nutzt Entity-Metadaten vom ESP32
# V1.2 Dynamische Switch-Erkennung
# V1.1 Initial

from __future__ import annotations

import logging
from typing import Any

import aiohttp
import async_timeout

from homeassistant.components.switch import SwitchEntity
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
    """Set up MADA switches using ESP32 metadata."""
    data = hass.data[DOMAIN][entry.entry_id]
    coordinator = data["coordinator"]
    entity_metadata = data["entity_metadata"]
    
    switches = []
    
    # Erstelle Switches basierend auf ESP32 Metadaten
    for entity_id, metadata in entity_metadata.items():
        entity_type = metadata.get("type")
        
        # Nur Switches verarbeiten
        if entity_type == "switch":
            _LOGGER.info(
                "Creating switch from metadata: %s (%s) at path %s",
                metadata.get("name", entity_id),
                entity_id,
                metadata.get("data_path", "unknown")
            )
            
            switches.append(
                MadaSwitchFromMetadata(
                    coordinator=coordinator,
                    entry=entry,
                    entity_id=entity_id,
                    metadata=metadata,
                )
            )
    
    _LOGGER.info("Created %d switches from ESP32 metadata", len(switches))
    async_add_entities(switches)


class MadaSwitchFromMetadata(CoordinatorEntity, SwitchEntity):
    """Switch created from ESP32 entity metadata."""

    def __init__(
        self,
        coordinator,
        entry: ConfigEntry,
        entity_id: str,
        metadata: dict,
    ) -> None:
        """Initialize the switch."""
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
        
        # Icon (aus Metadaten oder default)
        self._attr_icon = metadata.get("icon", "mdi:toggle-switch")

    @property
    def is_on(self) -> bool | None:
        """Return true if switch is on."""
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
            
            return bool(value)
            
        except (KeyError, TypeError, AttributeError) as e:
            _LOGGER.debug(f"Could not get value for {self._entity_id} from {self._data_path}: {e}")
            return None

    async def async_turn_on(self, **kwargs: Any) -> None:
        """Turn the switch on."""
        await self._set_state(True)

    async def async_turn_off(self, **kwargs: Any) -> None:
        """Turn the switch off."""
        await self._set_state(False)

    async def _set_state(self, state: bool) -> None:
        """Set switch state via API."""
        try:
            async with async_timeout.timeout(10):
                # Endpoint basierend auf entity_id
                # pumpe -> /rpc/Pump.Set
                endpoint_name = self._entity_id.capitalize()
                url = f"http://{self._host}/rpc/{endpoint_name}.Set"
                payload = {"on": state}
                
                _LOGGER.debug(f"Sending {state} to {url}")
                
                async with self._session.post(
                    url,
                    json=payload,
                    headers={"Content-Type": "application/json"},
                ) as response:
                    if response.status == 200:
                        await self.coordinator.async_request_refresh()
                    else:
                        _LOGGER.error(
                            "Failed to set %s state: HTTP %s",
                            self._attr_name,
                            response.status,
                        )
                        
        except aiohttp.ClientError as err:
            _LOGGER.error("Error setting %s state: %s", self._attr_name, err)
        except Exception as err:
            _LOGGER.error("Unexpected error setting %s state: %s", self._attr_name, err)
