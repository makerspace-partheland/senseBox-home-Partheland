# senseBox-home-Partheland

Firmware für senseBox:home-Stationen im Partheland, gebaut mit Visual Studio Code und PlatformIO. Die Firmware ist auf Umweltmessstationen mit LoRaWAN ausgelegt und veröffentlicht Messwerte unter anderem für openSenseMap, die Sensoren-Infrastruktur des Makerspace Partheland und darauf aufbauende Anwendungen.

Dieses Repository enthält Quellcode, Bibliotheken, Board-Dateien und Beispielkonfigurationen.

## Firmwarestand

Firmwarestand: `2.20`.

## Verwendete Sensoren

- HDC1080: Außentemperatur und relative Luftfeuchte
- DPS310 oder BMP280: Luftdruck und Gehäusetemperatur
- TSL45315: Beleuchtungsstärke
- VEML6070: UV-Intensität
- SDS011: Feinstaub PM10 und PM2.5
- DFRobot SEN0232: Lautstärke
- LC709203F: Akku-Spannung und Ladezustand bei PV-Stationen

## PV-Stationen

Photovoltaikbetriebene Stationen nutzen zusätzlich:

- 12 V / 30 W monokristallines Solarmodul
- LiPo-Akku mit 10000 mAh
- Waveshare MPPT 6V~24V Solar Power Management Modul
- Adafruit LC709203F LiPoly / LiIon Fuel Gauge
- Feuchtraumdose
- Isolationsmaterial gegen Kälte und Wärme

Je nach Akku-Spannung passt die Firmware das Sendeintervall an. Die Grenzwerte liegen in `include/pv.h`; die zugehörige Übersicht liegt in `Dokumente/TX_INTERVAL_Spannung_PV.ods`.

## Messwertberechnung

### PV-Stationen (`src/main-pv.cpp`)

- Bei `TxInterval == MAX_INTERVAL_S` werden im Sendeintervall 30 Messwerte gesammelt. Daraus werden Median und Maximum berechnet.
- Bei allen anderen Intervallen wird nur der aktuelle Messwert verwendet. Messwerte werden nach dem Senden geleert.

### Stationen mit fester Stromversorgung (`src/main-default.cpp`)

- Im Sendeintervall werden 30 Messwerte gesammelt. Daraus werden Median und Maximum berechnet.

## LED-Blinkcodes

Die Blinkcodes sind auf zuverlässig erkennbare Fehlerquellen begrenzt. Werte-Heuristiken werden nicht verwendet.

Timing:

- 800 ms an / 500 ms aus pro Blink
- 3000 ms Pause zwischen Wiederholungen
- mehrere Fehler gleichzeitig: dauerhaft rot
- bei Fehlern werden Messungen und Sendungen gestoppt
- bei dauerhaftem Fehlerzustand erfolgt nach 1 Stunde ein automatischer Neustart

| LED-Zustand | Bedeutung |
| :---------- | :-------- |
| Grün blinkend (1x) | LoRa-Join läuft |
| Grün dauerhaft | Fehlerfreier Betrieb |
| Rot blinkend (1x) | SDS011: keine neuen Daten nach mehreren Versuchen |
| Rot blinkend (2x) | LC709203F nicht verbunden oder 0 V gemeldet, nur PV |
| Rot blinkend (3x) | LoRa-Chip nicht erkannt |
| Rot dauerhaft | Mehrere Fehler gleichzeitig |

## Dateien und Verzeichnisse

- `include/`: Konfigurationsvorlagen und globale Header
- `include/**/*.dist`: Beispielkonfigurationen für eigene Stationen
- `lib/`: eingebundene Bibliotheken
- `src/`: Firmwarecode für Standard- und PV-Stationen
- `scripts/`: Build-Skripte für PlatformIO
- `boards/` und `variants/`: PlatformIO-Boarddefinition für senseBox MCU
- `Dokumente/`: ergänzende technische Unterlagen
- `Druckvorlagen/`: 3D-Druckvorlagen

## Konfiguration

Kopiere eine passende `.dist`-Datei aus `include/` auf eine lokale `.h`-Datei und trage dort die Werte für die eigene Station ein.

Für LoRa kann der XBee-Port über `LORA_XBEE_PORT` gewählt werden:

- `1`: XBEE1
- `2`: XBEE2

Mit `LORA_POWER_OFF_UNUSED_PORT` kann der nicht genutzte XBee-Port abgeschaltet werden, um Strom zu sparen. Das gilt für `src/main-default.cpp` und `src/main-pv.cpp`.

Die angeschlossenen Sensoren werden über Sensor-Flags aktiviert, zum Beispiel:

- `HDC1080_CONNECTED`
- `TSL45315_CONNECTED`
- `VEML6070_CONNECTED`
- `SDS011_CONNECTED`
- `SOUNDLEVELMETER_CONNECTED`
- `DPS310_CONNECTED` oder `BMP280_CONNECTED`

Für den SDS011 wird der UART über `SDS_UART_PORT` festgelegt, zum Beispiel `Serial1` oder `Serial2`.

Für den Soundlevel-Sensor wird der Analogeingang über `SOUNDMETERPIN` gesetzt:

- Digital Port 1: Pins 1 oder 2
- Digital Port 2: Pins 3 oder 4
- Digital Port 3: Pins 5 oder 6

`TX_INTERVAL` sollte nicht kleiner als 60 Sekunden sein. Längere Intervalle sind empfehlenswert, um LoRaWAN-Fair-Use und Duty-Cycle einzuhalten.

## Build

Standard-Build:

```sh
pio run -e default
```

Build einer konkreten PlatformIO-Umgebung:

```sh
pio run -e <env>
```

Die erzeugte Firmware wird über die Build-Skripte im Unterordner `build/` abgelegt.

## senseBox MCU mit PlatformIO

Dieses Projekt enthält eine PlatformIO-Boarddefinition für die senseBox MCU, weil die senseBox-Board-Integration in `platform-atmelsam` nicht direkt verfügbar ist.

Die Datei `boards/sensebox_mcu.json` kann bei Bedarf in das PlatformIO-Core-Verzeichnis unter `platforms/*/boards/` kopiert werden. Das Core-Verzeichnis ist in der PlatformIO-Dokumentation unter `core_dir` beschrieben.

Quelle für Boarddefinition und Variantenordner ist `sensebox/senseBoxMCU-core`.
