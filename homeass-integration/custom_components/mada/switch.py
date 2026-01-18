"""Switch platform for MADA integration."""
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

from . import DOMAIN, MADADataUpdateCoordinator

_LOGGER = logging.getLogger(__name__)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    """Set up MADA switch."""
    coordinator = hass.data[DOMAIN][entry.entry_id]
    
    async_add_entities([MADAPumpSwitch(coordinator, entry)])


class MADAPumpSwitch(CoordinatorEntity, SwitchEntity):
    """Representation of MADA pump switch."""

    def __init__(
        self,
        coordinator: MADADataUpdateCoordinator,
        entry: ConfigEntry,
    ) -> None:
        """Initialize the switch."""
        super().__init__(coordinator)
        
        self._attr_unique_id = f"{entry.entry_id}_pump"
        self._attr_name = "MADA Pumpe"
        self._attr_icon = "mdi:water-pump"
        self._host = entry.data["host"]
        self._session = async_get_clientsession(coordinator.hass)
        
        # Device info
        self._attr_device_info = {
            "identifiers": {(DOMAIN, entry.entry_id)},
            "name": "MADA Bewässerung",
            "manufacturer": "LilyGo",
            "model": entry.data.get("model", "MADA v1.1"),
            "sw_version": entry.data.get("version", "1.2-HA"),
        }

    @property
    def is_on(self) -> bool | None:
        """Return true if pump is on."""
        if self.coordinator.data is None:
            return None
            
        try:
            return self.coordinator.data.get("pump", {}).get("running", False)
        except (KeyError, TypeError):
            return None

    async def async_turn_on(self, **kwargs: Any) -> None:
        """Turn the pump on."""
        await self._set_pump_state(True)

    async def async_turn_off(self, **kwargs: Any) -> None:
        """Turn the pump off."""
        await self._set_pump_state(False)

    async def _set_pump_state(self, state: bool) -> None:
        """Set pump state via API."""
        try:
            async with async_timeout.timeout(10):
                url = f"http://{self._host}/rpc/Pump.Set"
                payload = {"on": state}
                
                async with self._session.post(
                    url,
                    json=payload,
                    headers={"Content-Type": "application/json"},
                ) as response:
                    if response.status == 200:
                        # Request coordinator update
                        await self.coordinator.async_request_refresh()
                    else:
                        _LOGGER.error(
                            "Failed to set pump state: HTTP %s", response.status
                        )
                        
        except aiohttp.ClientError as err:
            _LOGGER.error("Error setting pump state: %s", err)
        except Exception as err:
            _LOGGER.error("Unexpected error setting pump state: %s", err)
