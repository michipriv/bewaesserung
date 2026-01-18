# Home Assistant Integration - HiGrow Bewässerungssystem

## Übersicht

Das LilyGo HiGrow Bewässerungssystem ist nach dem **Shelly Gen2 Prinzip** in Home Assistant integriert:

✅ **mDNS** - Automatische Geräteerkennung im Netzwerk  
✅ **HTTP JSON-RPC API** - Einfache REST Endpoints  
✅ **Kein MQTT** - Keine Broker-Abhängigkeit  
✅ **Kein WebSocket** - Reduzierte Komplexität  

---

## Quick Start

### 1. ESP32 Firmware flashen

```bash
cd C:\home\bewaesserung\LilyGo-HiGrow
python -m platformio run --target upload
```

**Speichernutzung:**
- RAM: 14.3% (46 KB)
- Flash: 36.6% (1.15 MB)

### 2. WiFi konfigurieren

Stellen Sie sicher, dass in `src/configuration.h` die richtigen WiFi-Daten eingetragen sind:

```cpp
#define WIFI_SSID "IhrWiFiName"
#define WIFI_PASSWD "IhrPasswort"
```

Firmware flashen: 
python -m platformio run --target upload


### 3. Gerät testen

Nach dem Flashen sollte das Gerät im Serial Monitor folgende Ausgabe zeigen:

C:\Users\mmade\.platformio\penv\Scripts\platformio.exe device monitor -p COM19 -b 115200

```
=================================
  HiGrow v1.2 - Baseline Test   
=================================
Connecting to IhrWiFiName...OK
IP: 192.168.1.147

=== Home Assistant Integration ===
mDNS responder started: higrow.local
REST API endpoints registered:
  GET  /mada
  GET  /rpc/mada.GetStatus
  POST /rpc/Pump.Set
  POST /rpc/Pump.SetPWM
HA Integration ready!
Access via: http://higrow.local
=================================
```

**Testen Sie die API:**

```bash
# Geräte-Info abrufen
curl http://higrow.local/mada

# Sollte zurückgeben:
{
  "name": "HiGrow",
  "model": "LilyGo-HiGrow-v1.1",
  "version": "1.2-HA",
  "mac": "A0:B1:C2:D3:E4:F5",
  "id": "a0b1c2d3e4f5",
  "type": "irrigation_controller",
  "hostname": "higrow.local"
}
```

```bash
# Alle Sensordaten abrufen
curl http://higrow.local/rpc/mada.GetStatus

# Sollte zurückgeben:
{
  "soil": {
    "moisture": 65,
    "salt": 450
  },
  "battery": {
    "voltage": 4150,
    "percent": 94
  },
  "temperature": {
    "value": 22.5,
    "source": "DHT11"
  },
  ...
}
```

---

## Home Assistant Konfiguration

### Schritt 1: Konfiguration hinzufügen

Öffnen Sie Ihre Home Assistant Konfigurationsdatei:

**Pfad:** `/config/configuration.yaml`

Fügen Sie den kompletten Inhalt von `homeassistant_config.yaml` ein.

### Schritt 2: Konfiguration prüfen

Gehen Sie in Home Assistant zu:

```
Einstellungen → System → Konfiguration prüfen
```

Wenn alles korrekt ist, sollte keine Fehlermeldung erscheinen.

### Schritt 3: Home Assistant neu starten

```
Einstellungen → System → Neu starten
```

### Schritt 4: Entities überprüfen

Nach dem Neustart sollten folgende Entities verfügbar sein:

**Sensoren:**
- `sensor.higrow_bodenfeuchte`
- `sensor.higrow_salz`
- `sensor.higrow_batterie`
- `sensor.higrow_batterie_spannung`
- `sensor.higrow_temperatur`
- `sensor.higrow_luftfeuchtigkeit`
- `sensor.higrow_helligkeit`
- `sensor.higrow_wifi_signal`
- `sensor.higrow_uptime`
- `sensor.higrow_pumpenleistung_ziel`
- `sensor.higrow_pumpenleistung_aktiv`

**Schalter:**
- `switch.higrow_pumpe`

**Input Number:**
- `input_number.higrow_pwm`

---

## REST API Dokumentation

### GET /mada
**Geräte-Identifikation**

Gibt grundlegende Informationen über das Gerät zurück.

**Response:**
```json
{
  "name": "HiGrow",
  "model": "LilyGo-HiGrow-v1.1",
  "version": "1.2-HA",
  "mac": "A0:B1:C2:D3:E4:F5",
  "id": "a0b1c2d3e4f5",
  "type": "irrigation_controller",
  "hostname": "higrow.local"
}
```

---

### GET /rpc/mada.GetStatus
**Alle Sensordaten abrufen**

Gibt alle aktuellen Messwerte zurück. Wird von Home Assistant alle 30 Sekunden abgerufen.

**Response:**
```json
{
  "soil": {
    "moisture": 65,
    "salt": 450
  },
  "battery": {
    "voltage": 4150,
    "percent": 94
  },
  "temperature": {
    "value": 22.5,
    "source": "DHT11"
  },
  "humidity": {
    "value": 55,
    "source": "DHT11"
  },
  "light": {
    "lux": 320
  },
  "pump": {
    "running": false,
    "pwm_target": 75,
    "pwm_active": 0
  },
  "system": {
    "uptime": 3600,
    "wifi_rssi": -45,
    "free_heap": 234567
  }
}
```

---

### POST /rpc/Pump.Set
**Pumpe ein- oder ausschalten**

Schaltet die Pumpe mit dem gespeicherten PWM-Wert ein oder aus.

**Request Body:**
```json
{
  "on": true
}
```

**Response:**
```json
{
  "success": true,
  "running": true,
  "pwm_active": 75
}
```

**cURL Beispiel:**
```bash
# Pumpe einschalten
curl -X POST http://higrow.local/rpc/Pump.Set \
  -H "Content-Type: application/json" \
  -d '{"on": true}'

# Pumpe ausschalten
curl -X POST http://higrow.local/rpc/Pump.Set \
  -H "Content-Type: application/json" \
  -d '{"on": false}'
```

---

### POST /rpc/Pump.SetPWM
**PWM-Wert einstellen (0-100%)**

Speichert den Ziel-PWM-Wert im Flash. Dieser Wert wird beim nächsten Pumpenstart verwendet.

**Request Body:**
```json
{
  "pwm": 80
}
```

**Response:**
```json
{
  "success": true,
  "pwm": 80
}
```

**cURL Beispiel:**
```bash
curl -X POST http://higrow.local/rpc/Pump.SetPWM \
  -H "Content-Type: application/json" \
  -d '{"pwm": 80}'
```

---

## Datenfluss

### Beim Systemstart

```
ESP32 bootet
  ↓
WiFi-Verbindung
  ↓
mDNS Start: "higrow.local"
  ↓
HTTP Server Port 80
  ↓
Home Assistant empfängt mDNS Broadcast
  ↓
Sensoren werden erstellt
```

### Im Betrieb

**Alle 30 Sekunden (automatisch):**
```
Home Assistant
  ↓ GET /rpc/mada.GetStatus
ESP32
  ↓ JSON Response (alle Werte)
Home Assistant
  → Sensoren aktualisiert
```

**Wenn Nutzer Pumpe einschaltet:**
```
Home Assistant UI: Switch ON
  ↓ POST /rpc/Pump.Set {"on": true}
ESP32
  → Pumpe startet mit gespeichertem PWM (z.B. 75%)
  → LED grün
  ↓ Response: {"running": true, "pwm_active": 75}
Home Assistant
  → Switch-Status aktualisiert
```

**Wenn Nutzer PWM ändert:**
```
Home Assistant UI: Slider → 85%
  ↓ POST /rpc/Pump.SetPWM {"pwm": 85}
ESP32
  → Speichert 85% in Flash (NVS)
  → Falls Pumpe läuft: sofort 85% anwenden
  ↓ Response: {"success": true, "pwm": 85}
Home Assistant
  → Slider-Wert bestätigt
```

---

## Lovelace Dashboard

### Einfache Karte

Fügen Sie in der Lovelace UI eine neue Karte hinzu:

```yaml
type: entities
title: HiGrow Bewässerung
entities:
  - entity: sensor.higrow_bodenfeuchte
    name: Bodenfeuchte
  - entity: sensor.higrow_temperatur
    name: Temperatur
  - entity: sensor.higrow_luftfeuchtigkeit
    name: Luftfeuchtigkeit
  - entity: sensor.higrow_batterie
    name: Batterie
  - type: divider
  - entity: switch.higrow_pumpe
    name: Pumpe
  - entity: input_number.higrow_pwm
    name: Pumpenleistung
  - entity: sensor.higrow_pumpenleistung_aktiv
    name: Aktuelle Leistung
```

### Erweiterte Karte mit Grafiken

```yaml
type: vertical-stack
cards:
  - type: entities
    title: HiGrow Steuerung
    entities:
      - entity: switch.higrow_pumpe
      - entity: input_number.higrow_pwm
      
  - type: history-graph
    title: Bodenfeuchte Verlauf
    entities:
      - entity: sensor.higrow_bodenfeuchte
    hours_to_show: 24
    
  - type: gauge
    entity: sensor.higrow_batterie
    min: 0
    max: 100
    severity:
      green: 70
      yellow: 30
      red: 0
```

---

## Troubleshooting

### Gerät nicht erreichbar (higrow.local)

**Problem:** `curl: (6) Could not resolve host: higrow.local`

**Lösungen:**

1. **mDNS Support prüfen:**
   ```bash
   # Linux
   sudo systemctl status avahi-daemon
   
   # Windows
   # iTunes oder Bonjour installieren
   ```

2. **Fallback auf IP:**
   - IP-Adresse im Router-Interface suchen
   - Oder im Serial Monitor beim Boot ablesen
   ```bash
   curl http://192.168.1.147/mada
   ```

3. **Netzwerk prüfen:**
   - ESP32 und Home Assistant im gleichen Netzwerk?
   - VLAN-Trennung?
   - Multicast blockiert?

### Sensoren zeigen "unavailable"

**Problem:** Entities in HA bleiben "unavailable"

**Lösungen:**

1. **API-Endpoint testen:**
   ```bash
   curl http://higrow.local/rpc/mada.GetStatus
   ```

2. **YAML-Konfiguration prüfen:**
   - Richtige URL?
   - `value_template` korrekt?
   - `scan_interval` nicht zu hoch?

3. **Home Assistant Logs:**
   ```
   Einstellungen → System → Protokolle
   # Suche nach "rest" oder "higrow"
   ```

### PWM wird nicht übernommen

**Problem:** Slider-Änderung in HA ändert nichts am ESP32

**Lösungen:**

1. **REST Command testen:**
   ```bash
   curl -X POST http://higrow.local/rpc/Pump.SetPWM \
     -H "Content-Type: application/json" \
     -d '{"pwm": 50}'
   ```

2. **Automation prüfen:**
   - In `configuration.yaml` vorhanden?
   - Automation aktiviert?

3. **Serial Monitor:**
   - Zeigt ESP32 "PWM set to: XX%"?

### Pumpe schaltet nicht

**Problem:** Switch in HA schaltet, aber Pumpe reagiert nicht

**Lösungen:**

1. **Hardware prüfen:**
   - Motor-Pin richtig angeschlossen?
   - Pumpe funktioniert?

2. **Serial Monitor:**
   ```
   Pump ON
   ```
   sollte erscheinen

3. **LED prüfen:**
   - Leuchtet grün wenn Pumpe an?
   - LED funktioniert?

---

## Vorteile gegenüber MQTT

| Feature | HTTP/REST | MQTT |
|---------|-----------|------|
| **Broker benötigt** | ❌ Nein | ✅ Ja (Mosquitto) |
| **Konfiguration** | ✅ Einfach | ⚠️ Komplex |
| **Debugging** | ✅ curl/Browser | ⚠️ MQTT-Tools nötig |
| **Firewall** | ✅ HTTP (80) | ⚠️ MQTT (1883) |
| **Latenz** | ✅ ~50ms | ✅ ~30ms |
| **mDNS Discovery** | ✅ Ja | ❌ Nein |
| **Setup-Zeit** | ✅ 5 Min | ⚠️ 30+ Min |

---

## Technische Details

### mDNS (Multicast DNS)

**Was ist mDNS?**
- Ermöglicht Geräte-Namen im lokalen Netzwerk ohne DNS-Server
- `.local` Domain wird automatisch aufgelöst
- Vergleichbar mit "Bonjour" (Apple) oder "Avahi" (Linux)

**Wie funktioniert es?**
1. ESP32 sendet Multicast: "Ich bin `higrow.local` mit IP `192.168.1.147`"
2. Alle Geräte im Netzwerk empfangen diese Information
3. Bei Anfrage nach `higrow.local` wird IP zurückgegeben
4. Bei IP-Änderung sendet ESP32 automatisch Update

**Service Discovery:**
- ESP32 kündigt HTTP-Service auf Port 80 an
- TXT Records enthalten: Modell, Version, Typ
- Home Assistant kann automatisch neue Geräte finden

### ArduinoJson Version

Die Integration nutzt **ArduinoJson v6.8.0**:
- `StaticJsonDocument<SIZE>` für feste Größen
- `.is<TYPE>()` zum Typ-Prüfen
- Effiziente Speichernutzung

### Speicheroptimierung

**Flash-Speicherung (NVS):**
- PWM-Wert wird persistent gespeichert
- Überdauert Neustarts und Stromausfall
- Namespace: "pump_settings"
- Key: "pwm_target"

---

## Support & Weiterentwicklung

### Geplante Features

- [ ] Custom Home Assistant Integration (Python)
- [ ] Automatische Discovery via mDNS
- [ ] Firmware-Updates via OTA
- [ ] Erweiterte Automationen
- [ ] Grafana-Anbindung

### Bekannte Einschränkungen

- REST Sensoren verursachen mehr HTTP-Traffic als MQTT
- Keine Echtzeit-Updates (Polling alle 30s)
- Bei vielen Geräten kann MQTT effizienter sein

### Community

Bei Problemen oder Verbesserungsvorschlägen:
- GitHub Issues erstellen
- Serial Monitor Ausgabe mitschicken
- Home Assistant Logs beifügen

---

**Version:** 1.2-HA  
**Datum:** 2026-01-18  
**Autor:** Mader Michael

**Lizenz:** MIT License
