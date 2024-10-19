# Druckvorlagen

## Sensoren
Hier ist der Sensorhalter hinterlegt, welcher Lautstärkesensor und Luftfeuchte-/Temperatursensor im Außengehäuse besser fixieren.
Stand Oktover 2024 müssen wir den Halter anpassen, da der Luftfeuchte-/Temperatursensor zwei weitere Steckbuchsen verpasst bekommen hat und dadurch nicht mehr von oben montiert werden kann. Issue ist erstellt.
 - mit PETG/ASA/ABS drucken

## Gateway
### Masthalterschelle
Zur Montage einer externen LoRaWAN Antenne via Mathalter eines Gateways an einem Antennenmast, haben wir Masthalterschelle1.STL und Masthalterschelle2.STL entworfen, wenn wir den Masthalter nicht mit Edelstahlschellen befestigen können. 
- mit ASA/ABS drucken

### Antennendeckel
War ein Antennenmast verschlossen und bot aber eine noch nicht verwendete Kabeldurchführung, haben wir Antennendeckel_V2.STL im Einsatz. 
- mit ASA/ABS drucken


### DinRail Gateway Platine
Das "The Things Indoor Gateway" lässt sich als kostengünstige Lösung für ein Outdoor Gateway mit Antennenkabel aus dem Gehäuse zerlegen und mittels DinRail_Gateway_Platine_Netzteil_V3.STL und DinRail_Gateway_Platine_V2.STL können die zwei Boards auf einer DIN-Schiene/Hutschiene in einem wetterfesten Gehäuse/Verteilerkasten untergebracht werden. Wir verbinden das Antennenkabel mittels N-Typ oder SMA Kabel und einem IPX/u.fl auf N Typ Buchse Pigtail-Kabel oder IPX/u.fl auf SMA Kabel, wobei der N-Typ oder SMA Aschluss mit dem Gehäuse nach außen geleitet wird. Ob wir N-Typ oder SMA Kabel verwenden entscheidet der Aufbauort.
Das Netzteil muss weiterhin mit 230V Spannung versorgt werden, wir löten entsprechende kurze Kabel an, die auf Hutschienen Verteilerblöcke/Geräteklemmen (PHOENIX CONTACT oder WAGO) im Gehäuse enden, so dass ein Elektriker vor Ort das Stromkabel nur in dem Gehäuse via Verteilerblöcke/Geräteklemmen auflegen und prüfen muss.
Das Gehäuse ist zwar wetterfest, dennoch wird es unter dem Dach montiert und nur die LoRa-Antenne nach außen zum Antennenmast geführt.

Ansonsten nutzen wir vorangig fertige wetterfeste Gateways mit LAN/WLAN und/oder SIM-Karte. Mit und ohne Antennenkabel, je nach örtlicher Gegebenheit.
