# senseBox-home-Partheland

Fork von senseBox-homeV2LoRa speziell für die im Partheland verwendeten Stationen für https://opensensemap.org, https://sensoren.makerspace-partheland.de und der Partheland-App in VS Code mit PlatformIO.

## Einleitung
Das Repo beinhaltet den Quelltext, die Konfigurationen, Bibliotheken und Dokumentationen für das senseBox-Projekt im Rahmen der SmartCity Umweltmessstationen im Partheland. 
Es ist das Ziel, das Erstellen vieler gleicher Stationen zu vereinheitlichen, zu vereinfachen und zu automatisieren und dennoch die Freiheit zu bieten leicht weitere fertige und selbstgebaute Sensoren zu integrieren und zu verwalten.

## Verwendete Sensoren:

- HDC1080 (Außentemperatur & rel. Luftfeuchte)
- Neue Stationen DPS310 - alte Stationen BMP280 (Luftdruck und Gehäusetemperatur)
- TSL45315 (Beleuchtungsstärke)
- VEML6070 (UV-Intensität)
- SDS 011 (Feinstaub PM10 und PM2.5)
- SDFRobot SEN0232 (Lautstärke)

## Sonderfall photovoltaik betriebene Stationen mit Akku:
- 12V 30 Watt monokristallines Solarmodul
- LiPo Akku 10000mAh
- Waveshare MPPT 6V~24V Solar Power Management Modul mit entfernten LEDs
- Adafruit LC709203F LiPoly / LiIon Spannungs und Ladestands Sensor
- Feuchtraumdose
- Isolationsmaterial gegen Kälte- und Wärme für LiPo
- Je nach Akku-Spannung passt sich das Sendeintervall an, die jeweils gültigen Werte (Stand 2024 verwenden wir diese seit 2 Jahren mit zwei Wintern) sind visuell unter /Dokumente/TX_INTERVAL_Spannung_PV.ods hinterlegt und als Variablen unter /include/pv.h


## Dateien und Verzeichnisse
* include/: Enthält Header-Dateien, welche als Konfigurationsvorlagen und Konfiguration für die senseBoxen dienen.
* lib/: Beinhaltet die benötigten Drittanbieter-Bibliotheken. Ggf. sind bestimmte Versionen notwendig, da ansonsten Probleme mit dem senseBoard Board auftreten.
* src/: Quellcodedateien für die Implementierung der Sensorlogik, für senseBox mit festem Stromanlschluss an 5V und an PV-Module.
* scripts/: Die Skripte dienen dazu die kompilierte Firmware definiert unter build/ im Unterordner (custom_prog_folder) des Ortes und im Format "firmware_[Verantwortlicher]_[Ort]_[Version und Variante]_[Datum].bin" abzulegen.

      Verantwortlicher kann hier entweder der Makerspace Partheland e.V., hier betreut der Verein direkt die Stationspaten oder SmartCity sein, hier werden die Paten durch die jeweiligen Multiplikatoren in den Kommunen betreut um den Makerspace Partheland e.V. zu entlasten.

## Nutzung
Konfiguriere die Sensoren, indem Du die entsprechenden Dateien in include/ anpasst. Die jeweiligen LoRa Zugangsdaten können vom Paten der Station beim Makerspace Partheland e.V. erfragt werden.

Kompiliere nach der Konfiguration deine gewünschte Station durch Auswahl der entsprechenden PlatformIO Projektumgebung und übertrage die Firmware via USB Upload aus PlatformIO oder durch Ablage auf der senseBox:home via Doppelklick-Wechseldatenträgerfreigabe-Methode; die Firmware wird nach dem Kompilieren im Unterordner build/ abgelegt.


<br></br>
# senseBox VS Code PlatformIO Template

Via https://github.com/makerspace-partheland/senseBox

Als modernere Alternative zur Arduino IDE nutzen wir im Makerspace Partheland Visual Studio Code mit PlatformIO.
Um die fehlende senseBox Board Integration in PlatformIO zu überwinden, da das Issue <https://github.com/platformio/platform-atmelsam/pull/169> seit 2021 offen ist, hier ein Ausgangsprojekt mit Board Definition, notwendigem Variants Ordner und Bibliotheken unter lib. 
Als Quelle diente https://github.com/sensebox/senseBoxMCU-core

### Board Definition hinterlegen

Analog <https://docs.platformio.org/en/stable/platforms/creating_board.html#id3> die hier im Ordner "boards" hinterlegte sensebox_mcu.json in das Core_dir Verzeichnis im Unterordner boards ablegen --> .platformio/platforms/*/boards/
Wie finde ich das core_dir: <https://docs.platformio.org/en/stable/projectconf/sections/platformio/options/directory/core_dir.html#projectconf-pio-core-dir>

Visual Code mit PlatformIO neu starten, das Board ist nun verfügbar.

!['Project Wizard mit senseBox Board'](https://github.com/makerspace-partheland/senseBox/blob/main/ProjectWizard.png)
