# HiGrow Custom Integration für Home Assistant

Diese Custom Integration ermöglicht die **automatische Erkennung** und **GUI-basierte Konfiguration** von HiGrow Bewässerungssystemen in Home Assistant - genau wie bei Shelly-Geräten!

## Features

✅ **Automatische Erkennung** via mDNS/Zeroconf  
✅ **GUI-Konfiguration** - keine YAML-Dateien nötig  
✅ **11 Sensoren** automatisch angelegt  
✅ **Pumpen-Switch** zum Ein/Ausschalten  
✅ **PWM-Slider** zur Leistungsregelung  
✅ **Gerätekarte** mit allen Informationen  
✅ **Polling** alle 30 Sekunden  

---

## Installation

### Methode 1: Manuell (empfohlen)

1. **Kopiere den Ordner** `custom_components/higrow` nach:
   ```
   /config/custom_components/higrow/
   ```

2. **Verzeichnisstruktur** sollte so aussehen:
   ```
   /config/
   └── custom_components/
       └── higrow/
           ├── __init__.py
           ├── config_flow.py
           ├── manifest.json
           ├── sensor.py
           ├── switch.py
           ├── number.py
           ├── strings.json
           └── translations/
               └── de.json
   ```

3. **Home Assistant neu starten:**
   ```
   Einstellungen → System → Neu starten
   ```

### Methode 2: Via HACS (wenn verfügbar)

1. HACS → Integrationen → 3-Punkte-Menü → Benutzerdefinierte Repositories
2. Repository URL einfügen
3. Kategorie: Integration
4. Herunterladen

---

## Konfiguration

### Automatische Erkennung (Empfohlen)

Nach der Installation und Neustart von Home Assistant:

1. Gerät wird **automatisch erkannt**
2. Benachrichtigung erscheint: **"Neues Gerät gefunden: HiGrow Bewässerung"**
3. Klicke auf **"Konfigurieren"**
4. Bestätige die automatisch erkannten Daten
5. Klicke **"Absenden"**
6. **Fertig!** ✅

### Manuelle Hinzufügung

Falls automatische Erkennung nicht funktioniert:

1. Gehe zu:
   ```
   Einstellungen → Geräte & Dienste → Integration hinzufügen
   ```

2. Suche nach **"HiGrow"**

3. Gib den Hostnamen oder IP ein:
   ```
   higrow.local
   ```
   oder
   ```
   192.168.9.167
   ```

4. Klicke **"Absenden"**

5. **Fertig!** ✅

---

## Entities

Nach der Konfiguration werden folgende Entities automatisch angelegt:

### Sensoren (11 Stück)

| Entity ID | Name | Einheit | Beschreibung |
|-----------|------|---------|--------------|
| `sensor.higrow_bodenfeuchte` | Bodenfeuchte | % | Feuchtigkeit im Boden |
| `sensor.higrow_salz` | Salz | µS/cm | Leitfähigkeit/Salzgehalt |
| `sensor.higrow_batterie_spannung` | Batterie Spannung | mV | Batteriespannung |
| `sensor.higrow_batterie` | Batterie | % | Batterieladung in % |
| `sensor.higrow_temperatur` | Temperatur | °C | Umgebungstemperatur |
| `sensor.higrow_luftfeuchtigkeit` | Luftfeuchtigkeit | % | Luftfeuchte |
| `sensor.higrow_helligkeit` | Helligkeit | lx | Lichtstärke |
| `sensor.higrow_wifi_signal` | WiFi Signal | dBm | WLAN-Signalstärke |
| `sensor.higrow_uptime` | Uptime | s | Betriebszeit |
| `sensor.higrow_pumpenleistung_ziel` | Pumpenleistung Ziel | % | Ziel-PWM |
| `sensor.higrow_pumpenleistung_aktiv` | Pumpenleistung Aktiv | % | Aktive PWM |

### Schalter

| Entity ID | Name | Typ | Beschreibung |
|-----------|------|-----|--------------|
| `switch.higrow_pumpe` | Pumpe | switch | Pumpe Ein/Aus |

### Number

| Entity ID | Name | Typ | Beschreibung |
|-----------|------|-----|--------------|
| `number.higrow_pumpenleistung` | Pumpenleistung | slider | PWM 0-100% |

---

## Dashboard

### Einfache Karte

```yaml
type: entities
title: HiGrow Bewässerung
entities:
  - sensor.higrow_bodenfeuchte
  - sensor.higrow_temperatur
  - sensor.higrow_batterie
  - switch.higrow_pumpe
  - number.higrow_pumpenleistung
```

### Erweiterte Karte mit Grafik

```yaml
type: vertical-stack
cards:
  - type: glance
    title: HiGrow Übersicht
    entities:
      - sensor.higrow_bodenfeuchte
      - sensor.higrow_temperatur
      - sensor.higrow_batterie
      
  - type: entities
    title: Pumpensteuerung
    entities:
      - switch.higrow_pumpe
      - number.higrow_pumpenleistung
      - sensor.higrow_pumpenleistung_aktiv
      
  - type: history-graph
    title: Bodenfeuchte 24h
    entities:
      - sensor.higrow_bodenfeuchte
    hours_to_show: 24
```

---

## Automation Beispiele

### Automatisches Gießen bei trockener Erde

```yaml
automation:
  - alias: "Auto-Bewässerung"
    trigger:
      - platform: numeric_state
        entity_id: sensor.higrow_bodenfeuchte
        below: 30
    condition:
      - condition: time
        after: "06:00:00"
        before: "22:00:00"
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.higrow_pumpe
      - delay: "00:00:10"
      - service: switch.turn_off
        target:
          entity_id: switch.higrow_pumpe
```

### Pumpenleistung basierend auf Trockenheit

```yaml
automation:
  - alias: "PWM je nach Trockenheit"
    trigger:
      - platform: state
        entity_id: sensor.higrow_bodenfeuchte
    action:
      - choose:
          - conditions:
              - condition: numeric_state
                entity_id: sensor.higrow_bodenfeuchte
                below: 20
            sequence:
              - service: number.set_value
                target:
                  entity_id: number.higrow_pumpenleistung
                data:
                  value: 100
          - conditions:
              - condition: numeric_state
                entity_id: sensor.higrow_bodenfeuchte
                below: 40
            sequence:
              - service: number.set_value
                target:
                  entity_id: number.higrow_pumpenleistung
                data:
                  value: 75
        default:
          - service: number.set_value
            target:
              entity_id: number.higrow_pumpenleistung
            data:
              value: 50
```

---

## Troubleshooting

### Integration erscheint nicht

1. Prüfe, ob Ordner korrekt kopiert wurde
2. Home Assistant neu starten
3. Logs prüfen: `Einstellungen → System → Protokolle`

### Automatische Erkennung funktioniert nicht

1. Prüfe, ob ESP32 läuft: `http://higrow.local/mada`
2. Stelle sicher, dass mDNS im Netzwerk funktioniert
3. Füge Gerät manuell hinzu

### Entities bleiben "unavailable"

1. Prüfe IP/Hostname in der Integration
2. Teste API: `curl http://higrow.local/rpc/mada.GetStatus`
3. Prüfe Logs in HA

### Pumpe schaltet nicht

1. Prüfe Hardware-Verbindung
2. Teste direkt: `curl -X POST http://higrow.local/rpc/Pump.Set -d '{"on":true}'`
3. Prüfe Serial Monitor vom ESP32

---

## Vergleich: YAML vs Custom Integration

| Feature | YAML Config | Custom Integration |
|---------|-------------|-------------------|
| **Setup** | Manuell, fehleranfällig | Automatisch, GUI |
| **Erkennung** | Keine | Automatisch via mDNS |
| **Entities** | Manuell definieren | Automatisch angelegt |
| **Gerätekarte** | Nein | Ja, vollständig |
| **Updates** | YAML bearbeiten | GUI-Button |
| **User-Friendly** | ❌ | ✅ |

---

## Technische Details

### Polling-Intervall

Die Integration ruft alle **30 Sekunden** die Sensordaten ab:
- Endpoint: `GET http://higrow.local/rpc/mada.GetStatus`
- Timeout: 10 Sekunden
- Bei Fehler: Automatischer Retry

### mDNS/Zeroconf Discovery

- Service-Typ: `_http._tcp.local.`
- Name-Pattern: `higrow*`
- Automatische IP-Aktualisierung

### API-Aufrufe

**Pumpe steuern:**
```
POST http://higrow.local/rpc/Pump.Set
{"on": true/false}
```

**PWM setzen:**
```
POST http://higrow.local/rpc/Pump.SetPWM
{"pwm": 0-100}
```

---

## Support

Bei Problemen:
1. Logs prüfen: `Einstellungen → System → Protokolle`
2. GitHub Issues erstellen
3. Debug-Level aktivieren:
   ```yaml
   logger:
     default: warning
     logs:
       custom_components.higrow: debug
   ```

---

## Lizenz

MIT License

## Autor

Mader Michael - 2026
