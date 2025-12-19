import type { HomeAssistant, LovelaceCardEditor } from 'custom-card-helpers';
import { css, html, LitElement, type TemplateResult } from 'lit';
import { customElement, property, state } from 'lit/decorators.js';
import {
  DEFAULT_ENTITY_PREFIX,
  DEFAULT_ZONE_COUNT,
  DEFAULT_ZONE_NAMES,
  MAX_ZONE_COUNT,
} from './const';
import type { ActronAirCardConfig } from './types';

@customElement('actron-air-esphome-card-editor')
export class ActronAirEsphomeCardEditor
  extends LitElement
  implements LovelaceCardEditor
{
  @property({ attribute: false }) public hass!: HomeAssistant;
  @state() private _config!: ActronAirCardConfig;

  public setConfig(config: ActronAirCardConfig): void {
    this._config = config;
  }

  private _valueChanged(ev: Event): void {
    if (!this._config || !this.hass) {
      return;
    }

    const target = ev.target as HTMLInputElement;
    const configValue = target.dataset.configValue;

    if (!configValue) {
      return;
    }

    let value: string | number | boolean = target.value;

    if (target.type === 'checkbox') {
      value = target.checked;
    } else if (target.type === 'number') {
      value = parseInt(target.value, 10);
    }

    const newConfig = {
      ...this._config,
      [configValue]: value,
    };

    this._fireConfigChanged(newConfig);
  }

  private _zoneNameChanged(ev: Event, index: number): void {
    if (!this._config) {
      return;
    }

    const target = ev.target as HTMLInputElement;
    const zones = [...(this._config.zones || [])];

    // Ensure array is long enough
    while (zones.length <= index) {
      zones.push({ name: DEFAULT_ZONE_NAMES[zones.length] });
    }

    zones[index] = { name: target.value };

    const newConfig = {
      ...this._config,
      zones,
    };

    this._fireConfigChanged(newConfig);
  }

  private _fireConfigChanged(config: ActronAirCardConfig): void {
    const event = new CustomEvent('config-changed', {
      detail: { config },
      bubbles: true,
      composed: true,
    });
    this.dispatchEvent(event);
  }

  protected render(): TemplateResult {
    if (!this.hass || !this._config) {
      return html``;
    }

    const zoneCount = this._config.zone_count || DEFAULT_ZONE_COUNT;

    return html`
      <div class="card-config">
        <div class="field">
          <label for="entity_prefix">Entity Prefix *</label>
          <input
            id="entity_prefix"
            type="text"
            .value=${this._config.entity_prefix || DEFAULT_ENTITY_PREFIX}
            data-config-value="entity_prefix"
            @input=${this._valueChanged}
          />
          <small>e.g., actron_air (creates sensor.actron_air_setpoint_temperature)</small>
        </div>

        <div class="field">
          <label for="name">Card Name</label>
          <input
            id="name"
            type="text"
            .value=${this._config.name || ''}
            data-config-value="name"
            @input=${this._valueChanged}
          />
        </div>

        <div class="field checkbox">
          <input
            id="show_timer"
            type="checkbox"
            .checked=${this._config.show_timer !== false}
            data-config-value="show_timer"
            @change=${this._valueChanged}
          />
          <label for="show_timer">Show Timer Controls</label>
        </div>

        <div class="field checkbox">
          <input
            id="show_zones"
            type="checkbox"
            .checked=${this._config.show_zones !== false}
            data-config-value="show_zones"
            @change=${this._valueChanged}
          />
          <label for="show_zones">Show Zone Controls</label>
        </div>

        <div class="field">
          <label for="zone_count">Number of Zones (1-${MAX_ZONE_COUNT})</label>
          <input
            id="zone_count"
            type="number"
            min="1"
            max="${MAX_ZONE_COUNT}"
            .value=${String(zoneCount)}
            data-config-value="zone_count"
            @input=${this._valueChanged}
          />
        </div>

        <div class="zone-names">
          <label>Zone Names</label>
          ${Array.from({ length: zoneCount }, (_, i) => {
            const zoneName =
              this._config.zones?.[i]?.name || DEFAULT_ZONE_NAMES[i];

            return html`
              <div class="zone-name-field">
                <span>Zone ${i + 1}:</span>
                <input
                  type="text"
                  .value=${zoneName}
                  @input=${(ev: Event) => this._zoneNameChanged(ev, i)}
                />
              </div>
            `;
          })}
        </div>
      </div>
    `;
  }

  static styles = css`
    .card-config {
      padding: 16px;
    }

    .field {
      margin-bottom: 16px;
    }

    .field label {
      display: block;
      font-weight: 500;
      margin-bottom: 4px;
    }

    .field input[type='text'],
    .field input[type='number'] {
      width: 100%;
      padding: 8px;
      border: 1px solid var(--divider-color, #ccc);
      border-radius: 4px;
      font-size: 14px;
    }

    .field small {
      display: block;
      color: var(--secondary-text-color, #666);
      margin-top: 4px;
      font-size: 12px;
    }

    .field.checkbox {
      display: flex;
      align-items: center;
      gap: 8px;
    }

    .field.checkbox label {
      display: inline;
      margin-bottom: 0;
    }

    .zone-names {
      margin-top: 16px;
    }

    .zone-names > label {
      display: block;
      font-weight: 500;
      margin-bottom: 8px;
    }

    .zone-name-field {
      display: flex;
      align-items: center;
      gap: 8px;
      margin-bottom: 8px;
    }

    .zone-name-field span {
      min-width: 60px;
    }

    .zone-name-field input {
      flex: 1;
      padding: 6px;
      border: 1px solid var(--divider-color, #ccc);
      border-radius: 4px;
    }
  `;
}

// Explicit registration to ensure element is defined after minification
if (!customElements.get('actron-air-esphome-card-editor')) {
  customElements.define(
    'actron-air-esphome-card-editor',
    ActronAirEsphomeCardEditor,
  );
}

declare global {
  interface HTMLElementTagNameMap {
    'actron-air-esphome-card-editor': ActronAirEsphomeCardEditor;
  }
}
