# 🚗 Arduino-basiertes Parkhaussystem mit Schranke und Platzanzeige

Dieses Projekt zeigt den Aufbau eines **miniaturisierten, intelligenten Parkhaussystems** mit einem **Arduino Uno**. Es erkennt Fahrzeuge an Ein- und Ausfahrt, steuert eine Schranke per Servomotor und zeigt die verfügbaren Parkplätze in Echtzeit auf einem OLED-Display an.

## 📦 Funktionen

- Fahrzeugerkennung über Ultraschallsensoren (HC-SR04)
- Automatische Schrankensteuerung mit einem SG90-Servo
- Live-Anzeige freier Parkplätze auf einem OLED-Display (I2C)
- Kapazitätsprüfung vor Schrankenöffnung
- Übersichtlicher und modularer Code

## 🔧 Verwendete Hardware

- Arduino Uno  
- 2× HC-SR04 Ultraschallsensoren  
- 1× SG90 Servomotor  
- 1× 0,96" OLED-Display (I2C)  
- Breadboard + Jumperkabel  
- USB- oder externe Stromversorgung

## 🧠 Funktionsweise

1. **Fahrzeugerkennung**  
   Die Ultraschallsensoren erfassen Fahrzeuge beim Ein- und Ausfahren.

2. **Schrankensteuerung**  
   Ein Servomotor öffnet oder schließt die Schranke abhängig von der Sensorerkennung und der Parkplatzkapazität.

3. **Anzeige aktualisieren**  
   Das OLED-Display zeigt stets die aktuelle Anzahl freier Parkplätze an.

4. **Kapazitätsmanagement**  
   Die Schranke öffnet nur, wenn noch freie Plätze verfügbar sind (`availableSlots > 0`).

## 💡 Mögliche Erweiterungen

- WLAN- oder Ethernet-Modul zur Online-Anbindung
- Anzeige über Web-Dashboard oder Smartphone-App
- Verbesserung der Sensorfilterung bei Störungen
- Erweiterung um eine LED-Ampel zur visuellen Anzeige

## 📝 Quellcode

Der Code ist in C++ geschrieben und wurde in der **Arduino IDE** entwickelt. Er ist modular aufgebaut und lässt sich leicht erweitern oder anpassen.

## 📷 Bilder

📸 Im Verzeichnis `/images` befinden sich Fotos vom Aufbau und der Schaltung.

## 👨‍💻 Projektteam

- Florian Bruchhage  
- Felix Niggemann  
- Vincent Martin Delgado  
- Maria Hoffmann

---

*Dieses Projekt wurde im Rahmen der Ausbildung zum Fachinformatiker für Systemintegration umgesetzt.*
