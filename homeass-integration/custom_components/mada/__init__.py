"""HiGrow Irrigation System Integration."""
import logging
import asyncio
from datetime import timedelta

import aiohttp
import async_timeout

from homeassistant.config_entries import ConfigEntry
from homeassistant.const import Platform
from homeassistant.core import HomeAssistant
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers.update_coordinator import (
    DataUpdateCoordinator,
    UpdateFailed,
)

_LOGGER = logging.getLogger(__name__)

DOMAIN = "higrow"
PLATFORMS = [Platform.SENSOR, Platform.SWITCH, Platform.NUMBER]

SCAN_INTERVAL = timedelta(seconds=30)


async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Set up HiGrow from a config entry."""
    host = entry.data["host"]
    
    coordinator = HiGrowDataUpdateCoordinator(hass, host)
    await coordinator.async_config_entry_first_refresh()

    hass.data.setdefault(DOMAIN, {})
    hass.data[DOMAIN][entry.entry_id] = coordinator

    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)

    return True


async def async_unload_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Unload a config entry."""
    unload_ok = await hass.config_entries.async_unload_platforms(entry, PLATFORMS)
    
    if unload_ok:
        hass.data[DOMAIN].pop(entry.entry_id)

    return unload_ok


class HiGrowDataUpdateCoordinator(DataUpdateCoordinator):
    """Class to manage fetching HiGrow data."""

    def __init__(self, hass: HomeAssistant, host: str) -> None:
        """Initialize."""
        self.host = host
        self.session = async_get_clientsession(hass)
        
        super().__init__(
            hass,
            _LOGGER,
            name=DOMAIN,
            update_interval=SCAN_INTERVAL,
        )

    async def _async_update_data(self):
        """Fetch data from HiGrow device."""
        try:
            async with async_timeout.timeout(10):
                url = f"http://{self.host}/rpc/mada.GetStatus"
                async with self.session.get(url) as response:
                    if response.status != 200:
                        raise UpdateFailed(f"Error fetching data: {response.status}")
                    
                    data = await response.json()
                    return data
                    
        except asyncio.TimeoutError as err:
            raise UpdateFailed(f"Timeout fetching data from {self.host}") from err
        except aiohttp.ClientError as err:
            raise UpdateFailed(f"Error fetching data from {self.host}: {err}") from err
        except Exception as err:
            raise UpdateFailed(f"Unexpected error: {err}") from err
