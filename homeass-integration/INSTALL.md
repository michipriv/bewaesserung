# HiGrow Custom Integration - Installation in 3 Schritten

## Schritt 1: Dateien kopieren (2 Min)

Kopiere den kompletten Ordner nach Home Assistant:

**Von:**
```
C:\home\bewaesserung\homeass-integration\custom_components\higrow\
```

**Nach:**
```
/config/custom_components/higrow/
```

### Wie komme ich an /config/?

**Option A: Samba Share (Windows)**
1. Samba Share Add-on installieren
2. Netzlaufwerk verbinden: `\\homeassistant.local\config`
3. Ordner `custom_components` erstellen (falls nicht vorhanden)
4. `higrow` Ordner reinkopieren

**Option B: SSH/SCP**
```bash
scp -r custom_components/higrow root@homeassistant.local:/config/custom_components/
```

**Option C: File Editor Add-on**
1. File Editor Add-on installieren
2. Dateien manuell erstellen und Code kopieren

---

## Schritt 2: Home Assistant neu starten (1 Min)

```
Einstellungen → System → Neu starten
```

Warte bis HA wieder online ist (~1-2 Min).

---

## Schritt 3: Gerät hinzufügen (1 Min)

### Automatisch (Empfohlen):

Nach Neustart erscheint Benachrichtigung:
```
"Neues Gerät gefunden: HiGrow Bewässerung"
```

→ **[Konfigurieren]** klicken  
→ **[Absenden]** klicken  
→ **Fertig!** ✅

### Manuell (falls Auto-Discovery nicht klappt):

```
Einstellungen 
  → Geräte & Dienste 
  → [+ Integration hinzufügen] 
  → Suche: "HiGrow"
  → Host eingeben: higrow.local
  → [Absenden]
```

---

## Fertig! 🎉

Gehe zu:
```
Einstellungen → Geräte & Dienste → HiGrow Bewässerung
```

Dort siehst du:
- ✅ 11 Sensoren
- ✅ 1 Pumpen-Switch
- ✅ 1 PWM-Slider

Alle automatisch angelegt!

---

## Dashboard erstellen

Lovelace UI → Neue Karte:

```yaml
type: entities
title: HiGrow
entities:
  - sensor.higrow_bodenfeuchte
  - sensor.higrow_temperatur
  - sensor.higrow_batterie
  - switch.higrow_pumpe
  - number.higrow_pumpenleistung
```

---

## Problemlösung

**Integration wird nicht angezeigt?**
→ Ordnerstruktur prüfen: `/config/custom_components/higrow/__init__.py` muss existieren

**Automatische Erkennung funktioniert nicht?**
→ Gerät manuell hinzufügen mit IP: `192.168.9.167`

**Entities bleiben "unavailable"?**
→ API testen: `curl http://higrow.local/mada`

---

Vollständige Doku: siehe `README.md`
