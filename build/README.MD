# Smart-City-Partheland

Firmware für das Sensorenprojekt Smart City Partheland, sotiert nach Ort und erstellt durch Makerspace Partheland e.V.

## Änderungshistorie ##
- 12.04.2023: Initiale Bereitstellung neue Firmware basierend auf Visual Studio Code - Goodbye old opensensemap firmware
- 27.10.2023: Aktualisierung der Firmwares für Lieferung Gehäusetemperatur zur Erkennung potentieller Ausfälle durch Überhitzung der Hauptplatine (SenseBox MCU)
- 10.10.2024: Aktualisierung der Firmware für Lieferung von Median- und Maximal-Werten bei Lautstärke und Feinstaub. 
			  Die Messungen erfolgen sekündlich, wobei jeweils der Maximalwert ermittelt wird. Der Median wird aus den sekündlichen Messwerten über das Sendeintervall von einer Minute berechnet.
			  
			  **Median Messwerte** sind nützlich, um den mittleren Wert eines Datensatzes zu bestimmen, besonders wenn dieser Ausreißer enthält. Sie geben einen guten Überblick über den “typischen” Wert.

			  **Maximal-Messwerte** zeigen den höchsten Wert in einem Datensatz an. Sie sind hilfreich, um Extremwerte zu identifizieren und zu analysieren, wie weit die Daten streuen.
			  