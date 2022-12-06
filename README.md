# IPad-Gimbal

Projekt im Fach Energieeffiziente Microkontroller bei Prof. Walter

Bisheriger Projektstand von Tim Ruf unter:
https://github.com/ruti1014/CamServer

Tutorial für ESP NOW unter:

https://randomnerdtutorials.com/esp-now-auto-pairing-esp32-esp8266/

programmiert in Visualstudo Code

## Features
- Automatische WIFI-Channel suche -> Router gibt channel vor
- Automatisches Pairing (ESP NOW)
- Ansteuernn mehrerer Slaves zur gleichen Zeit
- AUswahl der entsprechenden ID

## TODO´s
- Webseite / Master:
  - Anzeige mehrerer Cams zur gleichen Zeit
  - Überarbeiteung der Datenberechnung ? oder auf Slave
  - Automatische vergabe von IDs
  - Anzeigename für einen Peer / Slave
  - Listenauswahl auf Webseite
  
- SLaves:
  - Ansteuern der Motoren
  - Rücksenden der Daten
