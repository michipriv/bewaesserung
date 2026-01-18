"""Config flow for HiGrow integration."""
import logging
from typing import Any

import aiohttp
import async_timeout
import voluptuous as vol

from homeassistant import config_entries
from homeassistant.components import zeroconf
from homeassistant.const import CONF_HOST
from homeassistant.core import HomeAssistant
from homeassistant.data_entry_flow import FlowResult
from homeassistant.helpers.aiohttp_client import async_get_clientsession

_LOGGER = logging.getLogger(__name__)

DOMAIN = "higrow"


async def validate_host(hass: HomeAssistant, host: str) -> dict[str, Any]:
    """Validate the host by connecting to the device."""
    session = async_get_clientsession(hass)
    
    try:
        async with async_timeout.timeout(10):
            url = f"http://{host}/mada"
            async with session.get(url) as response:
                if response.status != 200:
                    raise CannotConnect(f"HTTP {response.status}")
                
                data = await response.json()
                
                # Verify it's a HiGrow device
                if data.get("type") != "irrigation_controller":
                    raise InvalidDevice("Not a HiGrow device")
                
                return {
                    "title": data.get("name", "HiGrow"),
                    "model": data.get("model", "Unknown"),
                    "mac": data.get("mac", "Unknown"),
                    "version": data.get("version", "Unknown"),
                }
                
    except aiohttp.ClientError as err:
        raise CannotConnect(f"Connection error: {err}") from err
    except Exception as err:
        raise CannotConnect(f"Unexpected error: {err}") from err


class HiGrowConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """Handle a config flow for HiGrow."""

    VERSION = 1

    def __init__(self):
        """Initialize the config flow."""
        self.discovery_info = {}

    async def async_step_user(
        self, user_input: dict[str, Any] | None = None
    ) -> FlowResult:
        """Handle the initial step."""
        errors = {}

        if user_input is not None:
            host = user_input[CONF_HOST]
            
            try:
                info = await validate_host(self.hass, host)
            except CannotConnect:
                errors["base"] = "cannot_connect"
            except InvalidDevice:
                errors["base"] = "invalid_device"
            except Exception:  # pylint: disable=broad-except
                _LOGGER.exception("Unexpected exception")
                errors["base"] = "unknown"
            else:
                # Create unique ID from MAC address
                await self.async_set_unique_id(info["mac"])
                self._abort_if_unique_id_configured()
                
                return self.async_create_entry(
                    title=info["title"],
                    data={
                        CONF_HOST: host,
                        "model": info["model"],
                        "version": info["version"],
                    },
                )

        return self.async_show_form(
            step_id="user",
            data_schema=vol.Schema({
                vol.Required(CONF_HOST, default="higrow.local"): str,
            }),
            errors=errors,
        )

    async def async_step_zeroconf(
        self, discovery_info: zeroconf.ZeroconfServiceInfo
    ) -> FlowResult:
        """Handle zeroconf discovery."""
        # Extract hostname from discovery
        host = discovery_info.host
        
        # Store for later use
        self.discovery_info = {
            CONF_HOST: host,
            "name": discovery_info.name,
        }
        
        # Validate it's a HiGrow device
        try:
            info = await validate_host(self.hass, host)
        except (CannotConnect, InvalidDevice):
            return self.async_abort(reason="cannot_connect")
        
        # Create unique ID from MAC
        await self.async_set_unique_id(info["mac"])
        self._abort_if_unique_id_configured(updates={CONF_HOST: host})
        
        # Update discovery info with device details
        self.discovery_info.update(info)
        
        return await self.async_step_discovery_confirm()

    async def async_step_discovery_confirm(
        self, user_input: dict[str, Any] | None = None
    ) -> FlowResult:
        """Confirm discovery."""
        if user_input is not None:
            return self.async_create_entry(
                title=self.discovery_info["title"],
                data={
                    CONF_HOST: self.discovery_info[CONF_HOST],
                    "model": self.discovery_info["model"],
                    "version": self.discovery_info["version"],
                },
            )

        return self.async_show_form(
            step_id="discovery_confirm",
            description_placeholders={
                "name": self.discovery_info.get("title", "HiGrow"),
                "host": self.discovery_info[CONF_HOST],
                "model": self.discovery_info.get("model", "Unknown"),
            },
        )


class CannotConnect(Exception):
    """Error to indicate we cannot connect."""


class InvalidDevice(Exception):
    """Error to indicate the device is not a HiGrow."""
