import type { HomeAssistant, LovelaceCardEditor } from 'custom-card-helpers';
import {
  html,
  LitElement,
  type PropertyValues,
  type TemplateResult,
} from 'lit';
import { customElement, property, state } from 'lit/decorators.js';
import {
  CARD_VERSION,
  DEFAULT_ENTITY_PREFIX,
  DEFAULT_ZONE_COUNT,
  DEFAULT_ZONE_NAMES,
  ENTITY_SUFFIXES,
  MAX_ZONE_COUNT,
} from './const';
import { cardStyles } from './styles';
import type { ActronAirCardConfig, CardState } from './types';

import './editor';

console.info(
  `%c ACTRON-AIR-ESPHOME-CARD %c ${CARD_VERSION} `,
  'color: white; background: #1a3a5c; font-weight: bold;',
  'color: #1a3a5c; background: white; font-weight: bold;',
);

// Register card with HA card picker
(
  window as unknown as {
    customCards: Array<{ type: string; name: string; description: string }>;
  }
).customCards =
  (
    window as unknown as {
      customCards: Array<{ type: string; name: string; description: string }>;
    }
  ).customCards || [];
(
  window as unknown as {
    customCards: Array<{ type: string; name: string; description: string }>;
  }
).customCards.push({
  type: 'actron-air-esphome-card',
  name: 'Actron Air ESPHome Card',
  description: 'Control card for Actron Air systems via ESPHome',
});

@customElement('actron-air-esphome-card')
export class ActronAirEsphomeCard extends LitElement {
  @property({ attribute: false }) public hass!: HomeAssistant;
  @state() private _config!: ActronAirCardConfig;

  public static async getConfigElement(): Promise<LovelaceCardEditor> {
    return document.createElement(
      'actron-air-esphome-card-editor',
    ) as LovelaceCardEditor;
  }

  public static getStubConfig(): Record<string, unknown> {
    return {
      entity_prefix: DEFAULT_ENTITY_PREFIX,
      zone_count: DEFAULT_ZONE_COUNT,
    };
  }

  public setConfig(config: ActronAirCardConfig): void {
    if (!config.entity_prefix) {
      throw new Error('Please define entity_prefix');
    }

    this._config = {
      show_timer: true,
      show_zones: true,
      zone_count: DEFAULT_ZONE_COUNT,
      ...config,
    };
  }

  public getCardSize(): number {
    return 5;
  }

  protected shouldUpdate(changedProps: PropertyValues): boolean {
    if (changedProps.has('_config')) {
      return true;
    }

    const oldHass = changedProps.get('hass') as HomeAssistant | undefined;
    if (!oldHass) {
      return true;
    }

    // Only update if relevant entities changed
    const prefix = this._config.entity_prefix;
    const relevantEntities = [
      `sensor.${prefix}_${ENTITY_SUFFIXES.setpointTemperature}`,
      `switch.${prefix}_${ENTITY_SUFFIXES.power}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.cool}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.heat}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.autoMode}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.run}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.fanLow}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.fanMid}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.fanHigh}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.fanCont}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.room}`,
      `binary_sensor.${prefix}_${ENTITY_SUFFIXES.timer}`,
    ];

    const zoneCount = Math.min(
      this._config.zone_count || DEFAULT_ZONE_COUNT,
      MAX_ZONE_COUNT,
    );
    for (let i = 1; i <= zoneCount; i++) {
      relevantEntities.push(`binary_sensor.${prefix}_zone_${i}`);
    }

    return relevantEntities.some(
      (entityId) => oldHass.states[entityId] !== this.hass.states[entityId],
    );
  }

  private _getState(): CardState {
    const prefix = this._config.entity_prefix;
    const getEntityState = (domain: string, suffix: string): string => {
      const entityId = `${domain}.${prefix}_${suffix}`;
      return this.hass.states[entityId]?.state || 'unknown';
    };

    const isOn = (domain: string, suffix: string): boolean => {
      return getEntityState(domain, suffix) === 'on';
    };

    // Determine mode
    let mode: CardState['mode'] = 'off';
    if (isOn('binary_sensor', ENTITY_SUFFIXES.cool)) {
      mode = 'cool';
    } else if (isOn('binary_sensor', ENTITY_SUFFIXES.heat)) {
      mode = 'heat';
    } else if (isOn('binary_sensor', ENTITY_SUFFIXES.autoMode)) {
      mode = 'auto';
    }

    // Determine fan speed
    let fanSpeed: CardState['fanSpeed'] = 'off';
    if (isOn('binary_sensor', ENTITY_SUFFIXES.fanHigh)) {
      fanSpeed = 'high';
    } else if (isOn('binary_sensor', ENTITY_SUFFIXES.fanMid)) {
      fanSpeed = 'mid';
    } else if (isOn('binary_sensor', ENTITY_SUFFIXES.fanLow)) {
      fanSpeed = 'low';
    }

    // Get zones
    const zoneCount = Math.min(
      this._config.zone_count || DEFAULT_ZONE_COUNT,
      MAX_ZONE_COUNT,
    );
    const zones: boolean[] = [];
    for (let i = 1; i <= zoneCount; i++) {
      zones.push(isOn('binary_sensor', `zone_${i}`));
    }

    // Get temperature
    const tempEntity = `sensor.${prefix}_${ENTITY_SUFFIXES.setpointTemperature}`;
    const tempState = this.hass.states[tempEntity]?.state;
    const temperature = tempState ? parseFloat(tempState) : 0;

    return {
      power: isOn('switch', ENTITY_SUFFIXES.power),
      temperature,
      mode,
      fanSpeed,
      fanContinuous: isOn('binary_sensor', ENTITY_SUFFIXES.fanCont),
      run: isOn('binary_sensor', ENTITY_SUFFIXES.run),
      timer: isOn('binary_sensor', ENTITY_SUFFIXES.timer),
      room: isOn('binary_sensor', ENTITY_SUFFIXES.room),
      inside: isOn('binary_sensor', ENTITY_SUFFIXES.inside),
      zones,
    };
  }

  private _handlePowerToggle(): void {
    const entityId = `switch.${this._config.entity_prefix}_${ENTITY_SUFFIXES.power}`;
    this.hass.callService('switch', 'toggle', { entity_id: entityId });
  }

  private _handleButtonPress(suffix: string): void {
    const entityId = `button.${this._config.entity_prefix}_${suffix}`;
    this.hass.callService('button', 'press', { entity_id: entityId });
  }

  private _handleZoneToggle(zoneNum: number): void {
    const entityId = `switch.${this._config.entity_prefix}_zone_${zoneNum}`;
    this.hass.callService('switch', 'toggle', { entity_id: entityId });
  }

  private _renderLcd(state: CardState): TemplateResult {
    const tempDisplay = state.temperature ? state.temperature.toFixed(1) : '--';

    return html`
      <div class="lcd">
        <div class="lcd-temp">${tempDisplay}&deg;</div>
        <div class="lcd-status">
          <span class="${state.power ? 'active' : ''}">ON</span>
          <span class="${!state.power ? 'active' : ''}">OFF</span>
          <span class="${state.room ? 'active' : ''}">ROOM</span>
        </div>
        <div class="lcd-indicators">
          <span class="${state.fanContinuous ? 'active' : ''}">CONT</span>
          <span class="${state.fanSpeed === 'high' ? 'active' : ''}">HIGH</span>
          <span class="${state.fanSpeed === 'mid' ? 'active' : ''}">MID</span>
          <span class="${state.fanSpeed === 'low' ? 'active' : ''}">LOW</span>
          <span class="${state.mode === 'cool' ? 'active' : ''}">COOL</span>
          <span class="${state.mode === 'auto' ? 'active' : ''}">AUTO</span>
          <span class="${state.mode === 'heat' ? 'active' : ''}">HEAT</span>
          <span class="${state.run ? 'active' : ''}">RUN</span>
          <span class="${state.timer ? 'active' : ''}">TIMER</span>
        </div>
      </div>
    `;
  }

  private _renderZones(state: CardState): TemplateResult[] {
    const zoneCount = Math.min(
      this._config.zone_count || DEFAULT_ZONE_COUNT,
      MAX_ZONE_COUNT,
    );
    const zones: TemplateResult[] = [];

    for (let i = 0; i < zoneCount; i++) {
      const zoneNum = i + 1;
      const isActive = state.zones[i] || false;
      const zoneName = this._config.zones?.[i]?.name || DEFAULT_ZONE_NAMES[i];

      zones.push(html`
        <div class="zone zone${zoneNum}">
          <button
            class="zone-btn ${isActive ? 'active' : ''}"
            @click=${() => this._handleZoneToggle(zoneNum)}
          >
            <span class="zone-num">${zoneNum}</span>
            <span class="zone-name">${zoneName}</span>
          </button>
        </div>
      `);
    }

    return zones;
  }

  protected render(): TemplateResult {
    if (!this._config || !this.hass) {
      return html``;
    }

    const state = this._getState();
    const showTimer = this._config.show_timer !== false;
    const showZones = this._config.show_zones !== false;

    return html`
      <ha-card>
        <div class="card-content">
          ${this._renderLcd(state)}

          <div class="power">
            <button @click=${this._handlePowerToggle}>
              ON<br />───<br />OFF
            </button>
          </div>

          <div class="mode">
            <button
              class="control-btn"
              @click=${() => this._handleButtonPress(ENTITY_SUFFIXES.mode)}
            >
              AUTO<br />HEAT / COOL
            </button>
          </div>

          <div class="logo">≋ ActronAir</div>

          <div class="fan">
            <button
              class="control-btn"
              @click=${() => this._handleButtonPress(ENTITY_SUFFIXES.fan)}
            >
              FAN<br />CONTROL
            </button>
          </div>

          <div class="temp-up">
            <button
              class="temp-btn up"
              @click=${() => this._handleButtonPress(ENTITY_SUFFIXES.tempUp)}
            >
              <ha-icon icon="mdi:triangle"></ha-icon>
            </button>
          </div>

          <div class="temp-down">
            <button
              class="temp-btn down"
              @click=${() => this._handleButtonPress(ENTITY_SUFFIXES.tempDown)}
            >
              <ha-icon icon="mdi:triangle-down"></ha-icon>
            </button>
          </div>

          <div class="set-temp">
            <div class="set-temp-label">
              SET<br /><br />TEMP
            </div>
          </div>

          ${
            showTimer
              ? html`
                <div class="timer">
                  <button
                    class="timer-btn"
                    @click=${() => this._handleButtonPress(ENTITY_SUFFIXES.timerButton)}
                  >
                    TIMER
                  </button>
                </div>

                <div class="timer-up">
                  <button
                    class="timer-arrow"
                    @click=${() => this._handleButtonPress(ENTITY_SUFFIXES.timerUp)}
                  >
                    <ha-icon icon="mdi:triangle"></ha-icon>
                  </button>
                </div>

                <div class="timer-down">
                  <button
                    class="timer-arrow"
                    @click=${() => this._handleButtonPress(ENTITY_SUFFIXES.timerDown)}
                  >
                    <ha-icon icon="mdi:triangle-down"></ha-icon>
                  </button>
                </div>
              `
              : ''
          }

          ${showZones ? this._renderZones(state) : ''}
        </div>
      </ha-card>
    `;
  }

  static styles = cardStyles;
}

// Explicit registration to ensure element is defined after minification
if (!customElements.get('actron-air-esphome-card')) {
  customElements.define('actron-air-esphome-card', ActronAirEsphomeCard);
}

declare global {
  interface HTMLElementTagNameMap {
    'actron-air-esphome-card': ActronAirEsphomeCard;
  }
}
