# HiGrow → Home Assistant: 5-Minuten Setup

## Schritt 1: ESP32 flashen (2 Min)

```bash
cd C:\home\bewaesserung\LilyGo-HiGrow
python -m platformio run --target upload
```

## Schritt 2: API testen (1 Min)

```bash
# Warte bis ESP32 gebootet ist (Serial Monitor zeigt IP)
curl http://higrow.local/mada
curl http://higrow.local/rpc/mada.GetStatus
```

## Schritt 3: Home Assistant Config (2 Min)

1. Öffne `/config/configuration.yaml`
2. Kopiere kompletten Inhalt von `homeassistant_config.yaml` ans Ende
3. Speichern
4. Konfiguration prüfen: `Einstellungen → System → Konfiguration prüfen`
5. Neustart: `Einstellungen → System → Neu starten`

## Fertig! ✅

Nach dem Neustart sind alle Entities verfügbar:
- 11 Sensoren
- 1 Switch (Pumpe)
- 1 Slider (PWM)

---

## Erste Schritte

### Dashboard hinzufügen

Lovelace UI → Neue Karte:

```yaml
type: entities
title: HiGrow
entities:
  - sensor.higrow_bodenfeuchte
  - sensor.higrow_temperatur
  - sensor.higrow_batterie
  - switch.higrow_pumpe
  - input_number.higrow_pwm
```

### Automation: Automatisches Gießen

```yaml
automation:
  - alias: "Auto-Bewässerung bei trockener Erde"
    trigger:
      - platform: numeric_state
        entity_id: sensor.higrow_bodenfeuchte
        below: 30
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.higrow_pumpe
      - delay: "00:00:10"
      - service: switch.turn_off
        target:
          entity_id: switch.higrow_pumpe
```

---

## Problemlösung

**higrow.local nicht erreichbar?**
→ Verwende IP statt Hostname (im Serial Monitor ablesen)

**Sensoren "unavailable"?**
→ HA Logs prüfen: `Einstellungen → System → Protokolle`

**PWM funktioniert nicht?**
→ Automation in `configuration.yaml` vorhanden?

---

Vollständige Doku: siehe `HOMEASSISTANT_README.md`
