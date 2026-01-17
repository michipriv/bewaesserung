| Funktion                     | ESP32-Pin    |
| ---------------------------- | ------------ |
| Bodenfeuchte (kapazitiv)     | GPIO34 (ADC) |
| Licht (BH1750, I²C SDA)      | GPIO21       |
| Licht (BH1750, I²C SCL)      | GPIO22       |
| Temp./Feuchte (DHT11)        | GPIO16       |
| Akku-Spannung (VBAT Messung) | GPIO35 (ADC) |
| USB-UART TX                  | GPIO1        |
| USB-UART RX                  | GPIO3        |
| Reset                        | EN           |
| Boot                         | GPIO0        |


Versorgungspins:
5V: über USB-C
3V3: interner Regler (ESP32 & Sensoren)
GND

| Komponente      | Typ / Funktion                       |
| --------------- | ------------------------------------ |
| MCU             | ESP32 (Dual-Core, Wi-Fi + Bluetooth) |
| Flash           | 4 MB SPI-Flash                       |
| Bodenfeuchte    | Kapazitiver Sensor (PCB-Elektrode)   |
| Temp./Feuchte   | DHT11 (blauer Sensor)                |
| Lichtsensor     | BH1750 (I²C)                         |
| USB-UART        | CP210x oder CH9102F                  |
| Akku-Ladechip   | TP4054                               |
| Akku-Anschluss  | JST-PH 2-Pin (LiPo 3,7 V)            |
| Spannungsregler | 5 V → 3,3 V                          |
| Taster          | Reset / Boot                         |
| Magnete         | mechanische Fixierung                |


## 3) Wie die Komponenten angeschlossen sind

### Bodenfeuchte
- Direkt an eine kapazitive Leiterbahn  
- Auswertung über ADC (**GPIO34**)

### DHT11
- Datenleitung → **GPIO16**  
- Versorgung → **3,3 V**  
- Kein externer Pull-Up nötig (onboard)

### BH1750 (Licht)
- **I²C-Bus**
  - SDA → **GPIO21**
  - SCL → **GPIO22**
- Versorgung → **3,3 V**

### Akku & Laden
- LiPo → **JST-Stecker**
- Laden über **USB-C**
- **VBAT** über Spannungsteiler auf **GPIO35** messbar

### USB-Programmierung
- USB-C → USB-UART → **GPIO1 / GPIO3**

---

## 4) Kurzüberblick Signalwege

- Sensoren → ESP32 (**ADC / I²C / GPIO**)  
- ESP32 → **Wi-Fi / Bluetooth**  
- Versorgung: **USB-C oder LiPo → Lader → 3,3 V-Regler → Board**
