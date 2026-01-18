"""HiGrow Irrigation System Integration."""
# V1.3 Liest Entity-Metadaten vom ESP32
# V1.2 Dynamische Sensor-Erkennung
# V1.1 Initial

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

DOMAIN = "mada"
PLATFORMS = [Platform.SENSOR, Platform.SWITCH, Platform.NUMBER]

SCAN_INTERVAL = timedelta(seconds=30)


async def async_setup_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Set up MADA from a config entry."""
    host = entry.data["host"]
    
    coordinator = MadaDataUpdateCoordinator(hass, host)
    await coordinator.async_config_entry_first_refresh()

    # Lade Entity-Metadaten vom ESP32
    entity_metadata = await coordinator.fetch_entity_metadata()
    
    hass.data.setdefault(DOMAIN, {})
    hass.data[DOMAIN][entry.entry_id] = {
        "coordinator": coordinator,
        "entity_metadata": entity_metadata,
    }

    await hass.config_entries.async_forward_entry_setups(entry, PLATFORMS)

    return True


async def async_unload_entry(hass: HomeAssistant, entry: ConfigEntry) -> bool:
    """Unload a config entry."""
    unload_ok = await hass.config_entries.async_unload_platforms(entry, PLATFORMS)
    
    if unload_ok:
        hass.data[DOMAIN].pop(entry.entry_id)

    return unload_ok


class MadaDataUpdateCoordinator(DataUpdateCoordinator):
    """Class to manage fetching MADA data."""

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

    async def fetch_entity_metadata(self):
        """Fetch entity metadata from ESP32 /mada endpoint."""
        try:
            async with async_timeout.timeout(10):
                url = f"http://{self.host}/mada"
                async with self.session.get(url) as response:
                    if response.status != 200:
                        _LOGGER.warning(f"Could not fetch entity metadata: {response.status}")
                        return {}
                    
                    data = await response.json()
                    entities = data.get("entities", [])
                    
                    _LOGGER.info(f"Loaded {len(entities)} entity definitions from ESP32")
                    
                    # Konvertiere zu Dictionary für schnellen Zugriff
                    metadata = {}
                    for entity in entities:
                        entity_id = entity.get("id")
                        if entity_id:
                            metadata[entity_id] = entity
                    
                    return metadata
                    
        except asyncio.TimeoutError:
            _LOGGER.warning(f"Timeout fetching metadata from {self.host}")
            return {}
        except aiohttp.ClientError as err:
            _LOGGER.warning(f"Error fetching metadata from {self.host}: {err}")
            return {}
        except Exception as err:
            _LOGGER.error(f"Unexpected error fetching metadata: {err}")
            return {}

    async def _async_update_data(self):
        """Fetch data from MADA device."""
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
