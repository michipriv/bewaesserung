# Home Assistant Integration nach Shelly Gen2 Prinzip

## Übersicht

Das LilyGo HiGrow Bewässerungssystem wird nach dem Vorbild von Shelly Gen2 Geräten in Home Assistant integriert. Die Integration nutzt HTTP JSON-RPC API und mDNS für automatische Geräteerkennung - ohne MQTT Broker, ohne WebSocket-Komplexität.

---

## Grundprinzip

Shelly Gen2 Geräte kommunizieren mit Home Assistant über **HTTP** und werden automatisch im Netzwerk gefunden. Es gibt **keinen MQTT Broker**, **kein WebSocket** - nur einfache HTTP-Anfragen und Antworten.

---

## Die drei Säulen der Integration

### 1. mDNS - Automatisches Finden im Netzwerk

**Was ist das Problem?**
- ESP32 bekommt vom Router eine IP-Adresse wie `192.168.1.147`
- Nach Neustart kann die IP `192.168.1.203` sein
- Manuelle IP-Verwaltung ist nervig und fehleranfällig

**Die Lösung: mDNS (Multicast DNS)**
- ESP32 meldet sich beim Boot als `higrow.local` im Netzwerk
- Alle Geräte im Netzwerk können diesen Namen auflösen
- Egal welche IP der Router vergibt - der Name bleibt gleich
- Vergleichbar mit DNS, aber im lokalen Netzwerk ohne Server

**Wie es funktioniert:**
1. ESP32 bootet und bekommt IP: `192.168.1.147`
2. ESP32 sendet Broadcast ins Netzwerk: "Hallo! Ich bin `higrow.local` und habe IP `192.168.1.147`"
3. Home Assistant hört den Broadcast und speichert: `higrow.local = 192.168.1.147`
4. Home Assistant kann jetzt `http://higrow.local` verwenden statt der IP
5. Bei IP-Änderung sendet ESP32 automatisch Update

**Service Discovery:**
- ESP32 kündigt zusätzlich an: "Ich biete HTTP Service auf Port 80"
- Home Assistant scannt Netzwerk und findet automatisch neue Geräte
- Nutzer sieht: "Neues Gerät gefunden: HiGrow" und klickt "Hinzufügen"

---

### 2. HTTP JSON-RPC API - Kommunikation

**Das Protokoll:**
- Standard HTTP (wie eine Webseite)
- JSON für Daten (strukturiertes Format)
- RPC = Remote Procedure Call (Funktionsaufruf über Netzwerk)

**Zwei Richtungen:**

**A) Home Assistant fragt ESP32:**
- Alle 30 Sekunden sendet HA: `GET http://higrow.local/rpc/Shelly.GetStatus`
- ESP32 antwortet mit allen aktuellen Werten als JSON
- HA aktualisiert alle Sensoren in der Oberfläche

**B) Home Assistant steuert ESP32:**
- Nutzer klickt Pumpe-Schalter in HA
- HA sendet: `POST http://higrow.local/rpc/Pump.Set` mit Daten `{"on": true}`
- ESP32 schaltet Pumpe ein
- ESP32 antwortet: `{"success": true}`

**Vorteil gegenüber MQTT:**
- Kein zusätzlicher Server (MQTT Broker) nötig
- Direkte Kommunikation zwischen HA und ESP32
- Einfach zu debuggen (HTTP kann man im Browser testen)
- Standard-Protokoll, von allen Tools unterstützt

---

### 3. REST API Endpoints - Die Schnittstellen

**Was sind Endpoints?**
URLs auf dem ESP32, die bestimmte Funktionen ausführen.

**Endpoint 1: Geräte-Identifikation**
- URL: `http://higrow.local/shelly`
- Zweck: Home Assistant erkennt, was für ein Gerät das ist
- Antwort: Name, Modell, MAC-Adresse, Firmware-Version

**Endpoint 2: Status abrufen**
- URL: `http://higrow.local/rpc/Shelly.GetStatus`
- Zweck: Alle Sensordaten auf einmal holen
- Antwort: Bodenfeuchte, Temperatur, Batterie, Pumpen-Status, etc.
- Wird alle 30 Sekunden von HA abgerufen

**Endpoint 3: Pumpe steuern**
- URL: `http://higrow.local/rpc/Pump.Set`
- Zweck: Pumpe ein- oder ausschalten
- HA sendet: `{"on": true}` oder `{"on": false}`
- ESP32 schaltet entsprechend

**Endpoint 4: PWM einstellen**
- URL: `http://higrow.local/rpc/Pump.SetPWM`
- Zweck: Pumpenleistung einstellen
- HA sendet: `{"pwm": 75}` (75% Leistung)
- ESP32 speichert Wert in Flash und wendet ihn an

---

## Datenfluss im Betrieb

### Beim Systemstart

1. **ESP32 bootet:**
   - Verbindet sich mit WiFi
   - Startet mDNS Service als `higrow.local`
   - Startet HTTP Server auf Port 80
   - Lädt gespeicherte PWM-Einstellung aus Flash

2. **Home Assistant erkennt:**
   - Empfängt mDNS Broadcast
   - Zeigt Benachrichtigung: "Neues Gerät gefunden"
   - Nutzer klickt "Hinzufügen"
   - HA ruft `http://higrow.local/shelly` ab zur Identifikation
   - Gerät wird in HA angelegt

### Im laufenden Betrieb

**Alle 30 Sekunden - Automatisch:**
1. HA sendet: `GET http://higrow.local/rpc/Shelly.GetStatus`
2. ESP32 antwortet mit JSON:
   ```json
   {
     "soil": {"moisture": 65},
     "battery": {"voltage": 4200},
     "pump": {"running": false, "pwm_target": 75}
   }
   ```
3. HA aktualisiert alle Sensor-Werte in der Oberfläche

**Wenn Nutzer PWM-Slider bewegt:**
1. Nutzer schiebt Slider in HA auf 80%
2. HA sendet: `POST http://higrow.local/rpc/Pump.SetPWM` mit `{"pwm": 80}`
3. ESP32 speichert 80% in Flash (NVS)
4. ESP32 antwortet: `{"success": true, "pwm": 80}`
5. Beim nächsten Status-Abruf sieht HA: `"pwm_target": 80`

**Wenn Nutzer Pumpe einschaltet:**
1. Nutzer klickt Switch in HA
2. HA sendet: `POST http://higrow.local/rpc/Pump.Set` mit `{"on": true}`
3. ESP32 startet Pumpe mit gespeichertem PWM-Wert (z.B. 80%)
4. ESP32 antwortet: `{"success": true, "running": true}`
5. LED leuchtet grün (zeigt Pumpe läuft)

**Wenn Nutzer Pumpe ausschaltet:**
1. Nutzer klickt Switch in HA
2. HA sendet: `POST http://higrow.local/rpc/Pump.Set` mit `{"on": false}`
3. ESP32 stoppt Pumpe (PWM auf 0%)
4. PWM-Zielwert bleibt gespeichert (z.B. 80%)
5. LED aus
6. Beim nächsten Einschalten läuft Pumpe wieder mit 80%

---

## Home Assistant Konfiguration

### Variante A: RESTful Integration (Manuell)

In `configuration.yaml` eintragen:

**Sensoren definieren:**
```yaml
sensor:
  - platform: rest
    name: "HiGrow Bodenfeuchte"
    resource: "http://higrow.local/rpc/Shelly.GetStatus"
    value_template: "{{ value_json.soil.moisture }}"
    unit_of_measurement: "%"
    scan_interval: 30
    
  - platform: rest
    name: "HiGrow Batterie"
    resource: "http://higrow.local/rpc/Shelly.GetStatus"
    value_template: "{{ value_json.battery.voltage }}"
    unit_of_measurement: "mV"
    scan_interval: 30
    
  - platform: rest
    name: "HiGrow Temperatur"
    resource: "http://higrow.local/rpc/Shelly.GetStatus"
    value_template: "{{ value_json.temperature.value }}"
    unit_of_measurement: "°C"
    device_class: temperature
    scan_interval: 30
```

**Pumpen-Schalter:**
```yaml
switch:
  - platform: rest
    name: "HiGrow Pumpe"
    resource: "http://higrow.local/rpc/Shelly.GetStatus"
    body_on: '{"on": true}'
    body_off: '{"on": false}'
    is_on_template: "{{ value_json.pump.running }}"
    headers:
      Content-Type: application/json
```

**PWM Slider:**
```yaml
input_number:
  higrow_pwm:
    name: "Pumpenleistung"
    min: 0
    max: 100
    step: 1
    unit_of_measurement: "%"
    
automation:
  - alias: "HiGrow PWM setzen"
    trigger:
      - platform: state
        entity_id: input_number.higrow_pwm
    action:
      - service: rest_command.higrow_set_pwm
        data:
          pwm: "{{ states('input_number.higrow_pwm') | int }}"
          
rest_command:
  higrow_set_pwm:
    url: "http://higrow.local/rpc/Pump.SetPWM"
    method: POST
    content_type: "application/json"
    payload: '{"pwm": {{ pwm }}}'
```

### Variante B: Custom Integration (Fortgeschritten)

Eine Python-Integration die wie Shelly funktioniert:
- Automatische Discovery via mDNS
- Keine YAML-Konfiguration nötig
- Alle Entities werden automatisch angelegt
- Updates und Steuerung automatisch

---

## Was bleibt vom aktuellen Code?

**Wird behalten:**
- Komplette Sensor-Logik (`SensorManager`)
- PWM-Pumpensteuerung (`PWMControl`)
- Flash-Speicher (NVS) für Einstellungen
- WiFi-Setup
- ESPDash Web-Dashboard (läuft parallel weiter)

**Wird hinzugefügt:**
- HTTP JSON-RPC Endpoints
- mDNS Service
- JSON Parser für eingehende Befehle

**Wird NICHT entfernt:**
- Dashboard bleibt voll funktionsfähig
- Alle bestehenden Features bleiben erhalten
- Nur Ergänzung, kein Ersatz

---

## Vorteile dieser Lösung

✅ **Kein MQTT Broker nötig** - Eine Komponente weniger
✅ **Kein WebSocket** - Keine komplexe Verbindungsverwaltung
✅ **Standard-Protokoll** - HTTP versteht jedes Tool
✅ **Einfaches Debugging** - HTTP Requests kann man im Browser testen
✅ **Bewährte Architektur** - Wie Shelly (tausendfach im Einsatz)
✅ **Parallel-Betrieb** - Dashboard + HA Integration gleichzeitig
✅ **Automatische Erkennung** - HA findet Gerät selbst
✅ **Robust** - Funktioniert auch bei IP-Wechsel

---

## Vergleich zu Alternativen

### vs. MQTT
- **Vorteil HA-API:** Kein zusätzlicher Broker nötig
- **Nachteil HA-API:** Kein Push (HA muss pollen)
- **Fazit:** Für Sensoren mit 30s Update-Intervall ideal

### vs. ESPHome
- **Vorteil HA-API:** Volle Code-Kontrolle, eigenes Dashboard möglich
- **Nachteil HA-API:** Mehr Programmieraufwand
- **Fazit:** Für Custom-Hardware mit speziellen Features besser

### vs. WebSocket
- **Vorteil HA-API:** Einfacher, keine Verbindungsverwaltung
- **Nachteil HA-API:** Kein Echtzeit-Push
- **Fazit:** Für nicht zeitkritische Anwendungen ausreichend

---

## Zusammenfassung

Die Integration folgt dem bewährten Shelly Gen2 Prinzip:
1. **mDNS** für automatische Geräteerkennung
2. **HTTP JSON-RPC** für Kommunikation
3. **RESTful API** für Status und Steuerung

Das Ergebnis ist eine einfache, robuste Integration ohne zusätzliche Infrastruktur. Der ESP32 bleibt eigenständig (Dashboard funktioniert), kann aber gleichzeitig von Home Assistant gesteuert und überwacht werden.

---

*Erstellt: 2025-01-17*
*Projekt: LilyGo HiGrow Bewässerungssystem*
