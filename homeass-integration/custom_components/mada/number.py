"""Number platform for MADA integration."""
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

from . import DOMAIN, MADADataUpdateCoordinator

_LOGGER = logging.getLogger(__name__)


async def async_setup_entry(
    hass: HomeAssistant,
    entry: ConfigEntry,
    async_add_entities: AddEntitiesCallback,
) -> None:
    """Set up MADA number entities."""
    coordinator = hass.data[DOMAIN][entry.entry_id]
    
    async_add_entities([MADAPWMNumber(coordinator, entry)])


class MADAPWMNumber(CoordinatorEntity, NumberEntity):
    """Representation of MADA PWM control."""

    def __init__(
        self,
        coordinator: MADADataUpdateCoordinator,
        entry: ConfigEntry,
    ) -> None:
        """Initialize the number entity."""
        super().__init__(coordinator)
        
        self._attr_unique_id = f"{entry.entry_id}_pwm"
        self._attr_name = "MADA Pumpenleistung"
        self._attr_icon = "mdi:gauge"
        self._attr_native_min_value = 0
        self._attr_native_max_value = 100
        self._attr_native_step = 1
        self._attr_native_unit_of_measurement = "%"
        self._attr_mode = NumberMode.SLIDER
        
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
    def native_value(self) -> float | None:
        """Return the current PWM value."""
        if self.coordinator.data is None:
            return None
            
        try:
            return self.coordinator.data.get("pump", {}).get("pwm_target")
        except (KeyError, TypeError):
            return None

    async def async_set_native_value(self, value: float) -> None:
        """Set new PWM value."""
        try:
            async with async_timeout.timeout(10):
                url = f"http://{self._host}/rpc/Pump.SetPWM"
                payload = {"pwm": int(value)}
                
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
                            "Failed to set PWM: HTTP %s", response.status
                        )
                        
        except aiohttp.ClientError as err:
            _LOGGER.error("Error setting PWM: %s", err)
        except Exception as err:
            _LOGGER.error("Unexpected error setting PWM: %s", err)
