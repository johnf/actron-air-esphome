"""Config flow for Actron Air ESPHome integration."""
from __future__ import annotations

from typing import Any

import voluptuous as vol

from homeassistant import config_entries
from homeassistant.core import HomeAssistant, callback
from homeassistant.data_entry_flow import FlowResult
from homeassistant.helpers import selector
from homeassistant.helpers.service_info.zeroconf import ZeroconfServiceInfo

from .const import (
    CONF_CURRENT_TEMP_SENSOR,
    CONF_ENTITY_PREFIX,
    CONF_HUMIDITY_SENSOR,
    CONF_ZONE_COUNT,
    DEFAULT_ENTITY_PREFIX,
    DEFAULT_ZONE_COUNT,
    DOMAIN,
    ENTITY_SUFFIXES,
    MAX_ZONE_COUNT,
)


class EntityNotFoundError(Exception):
    """Error to indicate entity was not found."""


async def validate_input(hass: HomeAssistant, data: dict[str, Any]) -> dict[str, Any]:
    """Validate the user input allows us to connect."""
    entity_prefix = data[CONF_ENTITY_PREFIX]

    # Check if the setpoint temperature sensor exists
    temp_entity = f"sensor.{entity_prefix}_{ENTITY_SUFFIXES['setpoint_temp']}"
    if temp_entity not in hass.states.async_entity_ids("sensor"):
        raise EntityNotFoundError(f"Could not find {temp_entity}")

    return {"title": f"Actron Air ESPHome ({entity_prefix})"}


class ConfigFlow(config_entries.ConfigFlow, domain=DOMAIN):
    """Handle a config flow for Actron Air ESPHome."""

    VERSION = 1

    def __init__(self) -> None:
        """Initialise config flow."""
        self._discovered_prefix: str | None = None

    async def async_step_user(
        self, user_input: dict[str, Any] | None = None
    ) -> FlowResult:
        """Handle the initial step."""
        errors: dict[str, str] = {}

        if user_input is not None:
            try:
                info = await validate_input(self.hass, user_input)
            except EntityNotFoundError:
                errors["base"] = "entity_not_found"
            else:
                # Check for duplicate entries
                await self.async_set_unique_id(user_input[CONF_ENTITY_PREFIX])
                self._abort_if_unique_id_configured()

                return self.async_create_entry(title=info["title"], data=user_input)

        return self.async_show_form(
            step_id="user",
            data_schema=vol.Schema(
                {
                    vol.Required(
                        CONF_ENTITY_PREFIX, default=DEFAULT_ENTITY_PREFIX
                    ): str,
                    vol.Optional(
                        CONF_ZONE_COUNT, default=DEFAULT_ZONE_COUNT
                    ): selector.NumberSelector(
                        selector.NumberSelectorConfig(
                            min=1,
                            max=MAX_ZONE_COUNT,
                            mode=selector.NumberSelectorMode.BOX,
                        )
                    ),
                    vol.Optional(CONF_CURRENT_TEMP_SENSOR): selector.EntitySelector(
                        selector.EntitySelectorConfig(domain="sensor")
                    ),
                    vol.Optional(CONF_HUMIDITY_SENSOR): selector.EntitySelector(
                        selector.EntitySelectorConfig(domain="sensor")
                    ),
                }
            ),
            errors=errors,
        )

    async def async_step_zeroconf(
        self, discovery_info: ZeroconfServiceInfo
    ) -> FlowResult:
        """Handle zeroconf discovery."""
        # Extract device name from TXT record or hostname
        name = discovery_info.properties.get("name")
        if not name:
            # Fallback to hostname (strip .local suffix)
            name = discovery_info.hostname.rstrip(".")
            if name.endswith(".local"):
                name = name[:-6]

        # Convert to entity prefix format (replace - with _)
        entity_prefix = name.replace("-", "_")

        # Set unique ID and abort if already configured
        await self.async_set_unique_id(entity_prefix)
        self._abort_if_unique_id_configured()

        # Store for later steps
        self.context["title_placeholders"] = {"name": name}
        self._discovered_prefix = entity_prefix

        return await self.async_step_discovery_confirm()

    async def async_step_discovery_confirm(
        self, user_input: dict[str, Any] | None = None
    ) -> FlowResult:
        """Confirm discovery and configure options."""
        errors: dict[str, str] = {}

        if user_input is not None:
            data = {
                CONF_ENTITY_PREFIX: self._discovered_prefix,
                **user_input,
            }
            try:
                info = await validate_input(self.hass, data)
            except EntityNotFoundError:
                errors["base"] = "entity_not_found"
            else:
                return self.async_create_entry(title=info["title"], data=data)

        return self.async_show_form(
            step_id="discovery_confirm",
            data_schema=vol.Schema(
                {
                    vol.Optional(
                        CONF_ZONE_COUNT, default=DEFAULT_ZONE_COUNT
                    ): selector.NumberSelector(
                        selector.NumberSelectorConfig(
                            min=1,
                            max=MAX_ZONE_COUNT,
                            mode=selector.NumberSelectorMode.BOX,
                        )
                    ),
                    vol.Optional(CONF_CURRENT_TEMP_SENSOR): selector.EntitySelector(
                        selector.EntitySelectorConfig(domain="sensor")
                    ),
                    vol.Optional(CONF_HUMIDITY_SENSOR): selector.EntitySelector(
                        selector.EntitySelectorConfig(domain="sensor")
                    ),
                }
            ),
            errors=errors,
            description_placeholders={"name": self._discovered_prefix},
        )

    @staticmethod
    @callback
    def async_get_options_flow(
        config_entry: config_entries.ConfigEntry,
    ) -> config_entries.OptionsFlow:
        """Create the options flow."""
        return OptionsFlowHandler(config_entry)


class OptionsFlowHandler(config_entries.OptionsFlow):
    """Handle options flow for Actron Air ESPHome."""

    def __init__(self, config_entry: config_entries.ConfigEntry) -> None:
        """Initialise options flow."""
        self.config_entry = config_entry

    async def async_step_init(
        self, user_input: dict[str, Any] | None = None
    ) -> FlowResult:
        """Manage the options."""
        if user_input is not None:
            return self.async_create_entry(title="", data=user_input)

        return self.async_show_form(
            step_id="init",
            data_schema=vol.Schema(
                {
                    vol.Optional(
                        CONF_ZONE_COUNT,
                        default=self.config_entry.data.get(
                            CONF_ZONE_COUNT, DEFAULT_ZONE_COUNT
                        ),
                    ): selector.NumberSelector(
                        selector.NumberSelectorConfig(
                            min=1,
                            max=MAX_ZONE_COUNT,
                            mode=selector.NumberSelectorMode.BOX,
                        )
                    ),
                    vol.Optional(
                        CONF_CURRENT_TEMP_SENSOR,
                        default=self.config_entry.data.get(CONF_CURRENT_TEMP_SENSOR),
                    ): selector.EntitySelector(
                        selector.EntitySelectorConfig(domain="sensor")
                    ),
                    vol.Optional(
                        CONF_HUMIDITY_SENSOR,
                        default=self.config_entry.data.get(CONF_HUMIDITY_SENSOR),
                    ): selector.EntitySelector(
                        selector.EntitySelectorConfig(domain="sensor")
                    ),
                }
            ),
        )
