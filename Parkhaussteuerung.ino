#include <Servo.h>                    // Bibliothek zur Steuerung von Servomotoren
#include <Wire.h>                     // Bibliothek für I2C-Kommunikation
#include <Adafruit_GFX.h>            // Grafische Grundfunktionen für das OLED
#include <Adafruit_SSD1306.h>        // Treiber für das SSD1306 OLED-Display

// OLED Display Einstellungen
#define SCREEN_WIDTH 128             // Breite des OLED-Displays in Pixel
#define SCREEN_HEIGHT 64             // Höhe des OLED-Displays in Pixel
#define OLED_RESET -1                // Kein Reset-Pin verwendet
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED-Objekt initialisieren

// Pin-Zuordnungen Eingang
const int trigPin = 12;              // Trig-Pin des Ultraschallsensors am Eingang
const int echoPin = 13;              // Echo-Pin des Sensors am Eingang
const int servoPin = 9;             // Pin, an dem der Servo angeschlossen ist

// Pin-Zuordnungen Ausgang
const int trigPin2 = 7;              // Trig-Pin des Ausgangssensors
const int echoPin2 = 8;              // Echo-Pin des Ausgangssensors

// Schranken-Parameter
const int closedAngle = 0;           // Winkel für geschlossene Schranke
const int openAngle = 90;            // Winkel für geöffnete Schranke
const long thresholdDistance = 10;   // Abstand, unter dem ein Auto erkannt wird (cm)
const long leaveThreshold = 10;      // Abstand, ab dem ein Auto als "weggefahren" gilt

const int totalSlots = 3;            // Maximale Anzahl an Parkplätzen
int availableSlots = totalSlots;     // Variable für aktuelle freie Plätze
bool carDetected = false;            // Flag: Auto am Eingang erkannt
bool barrierOpen = false;            // Flag: Schranke ist offen
bool carLeavingDetected = false;     // Flag: Auto verlässt Parkplatz

Servo barrierServo;                  // Servo-Objekt zur Steuerung der Schranke

// ----------- FUNKTIONEN FÜR SLOT-ÄNDERUNGEN -----------

void decreaseAvailableSlots() {
  if (availableSlots > 0) {                           // Nur verringern, wenn noch Plätze frei sind
    availableSlots--;                                 // Freien Platz verringern
    Serial.print("Auto eingefahren. Neue freie Plätze: ");
    Serial.println(availableSlots);                   // Ausgabe im seriellen Monitor
    updateDisplay();                                  // Anzeige aktualisieren
  } else {
    Serial.println("Parkhaus voll. Kein Abzug.");     // Falls keine Plätze mehr frei sind
  }
}

void increaseAvailableSlots() {
  if (availableSlots < totalSlots) {                  // Nur erhöhen, wenn Plätze fehlen
    availableSlots++;                                 // Freien Platz erhöhen
    Serial.print("Auto ausgefahren. Neue freie Plätze: ");
    Serial.println(availableSlots);                   // Ausgabe im seriellen Monitor
    updateDisplay();                                  // Anzeige aktualisieren
  } else {
    Serial.println("Parkhaus war schon leer.");       // Falls das Parkhaus leer ist
  }
}

// -------------------------------------------------------

void setup() {
  delay(500);                                         // Kurze Wartezeit für OLED-Stabilität
  Serial.begin(9600);                                 // Serielle Kommunikation starten
  barrierServo.attach(servoPin);                      // Servo initialisieren
  barrierServo.write(closedAngle);                    // Schranke auf "geschlossen" setzen

  pinMode(trigPin, OUTPUT);                           // Trig-Pin Eingang auf Ausgang setzen
  pinMode(echoPin, INPUT);                            // Echo-Pin Eingang auf Eingang setzen
  pinMode(trigPin2, OUTPUT);                          // Trig-Pin Ausgang auf Ausgang setzen
  pinMode(echoPin2, INPUT);                           // Echo-Pin Ausgang auf Eingang setzen

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {   // OLED starten, I2C-Adresse 0x3C
    Serial.println("OLED konnte nicht initialisiert werden.");
  } else {
    display.clearDisplay();                           // OLED-Inhalt löschen
    updateDisplay();                                  // Erste Anzeige aktualisieren
  }
}

// Abstand messen (Eingang)
float getDistance() {
  digitalWrite(trigPin, LOW);                         // Sicherstellen, dass Trig LOW ist
  delayMicroseconds(2);                               // Kurze Pause
  digitalWrite(trigPin, HIGH);                        // Ultraschallimpuls senden
  delayMicroseconds(5000);                            // 5 ms Sendedauer
  digitalWrite(trigPin, LOW);                         // Impuls beenden

  long duration = pulseIn(echoPin, HIGH, 300000);     // Warte auf Echo (max 300 ms)
  float distance = duration * 0.034 / 2;              // Zeit in cm umrechnen
  return distance;                                    // Abstand zurückgeben
}

// Abstand messen (Ausgang)
float getDistanceExit() {
  digitalWrite(trigPin2, LOW);                        // Gleiches Prinzip wie oben, für Ausgang
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(5000);
  digitalWrite(trigPin2, LOW);

  long duration = pulseIn(echoPin2, HIGH, 300000);    // Echo am Ausgang messen
  float distanceExit = duration * 0.034 / 2;
  return distanceExit;
}

// Display aktualisieren
void updateDisplay() {
  display.clearDisplay();                             // Display löschen
  display.setTextSize(2);                             // Textgröße setzen
  display.setTextColor(WHITE);                        // Textfarbe: Weiß

  if (availableSlots > 0) {                           // Wenn Plätze frei sind
    display.setCursor(0, 5);                          // Cursor-Position setzen
    display.print("Freie\n");                         // Text anzeigen
    display.print("Plaetze\n");
    display.setCursor(50, 40);                        // Größere Zahl zentriert anzeigen
    display.setTextSize(3);
    display.print(availableSlots);
  } else {                                            // Wenn Parkhaus voll
    display.setCursor(0, 5);
    display.print("Parkhaus\n\n");
    display.setTextSize(3);
    display.print("Voll\n");
  }

  display.display();                                  // Anzeige aktualisieren
}

// Auto erkannt am Eingang
void checkForCar(double threshold) {
  float d1 = getDistance();                           // Erster Messwert
  delay(500);                                         // Kurze Wartezeit
  float d2 = getDistance();                           // Zweiter Messwert

  if (d1 < threshold && d2 < threshold) {             // Beide Messwerte unter Schwelle?
    carDetected = true;                               // Auto als erkannt markieren
    Serial.println("Auto erkannt (Eingang)");
  }
}

// Auto ist wirklich weg (Eingang)
void confirmCarGone(double threshold) {
  float d1 = getDistance();                           // Erster Messwert
  delay(500);
  float d2 = getDistance();                           // Zweiter Messwert

  if (d1 > threshold && d2 > threshold) {             // Auto ist entfernt
    carDetected = false;                              // Auto-Flag zurücksetzen
    barrierOpen = false;                              // Schranke wieder als geschlossen markieren

    decreaseAvailableSlots();                         // Einen Parkplatz abziehen

    Serial.println("Schranke schließt.");
    barrierServo.write(closedAngle);                  // Schranke schließen
    delay(500);                                       // Kurze Pause
  }
}

// Auto erkannt am Ausgang
void checkCarLeaving(double threshold) {
  float d1 = getDistanceExit();                       // Erster Messwert (Ausgang)
  delay(500);
  float d2 = getDistanceExit();                       // Zweiter Messwert

  if (d1 < threshold && d2 < threshold) {             // Auto am Ausgang erkannt
    if (!carLeavingDetected) {                        // Nur einmal reagieren
      carLeavingDetected = true;
      increaseAvailableSlots();                       // Parkplatz wieder freigeben
    }
  } else {
    carLeavingDetected = false;                       // Kein Auto mehr erkannt
  }
}

// Hauptloop
void loop() {
  float distance = getDistance();                     // Live-Abstand Eingang
  Serial.print("Live Abstand Eingang: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (!carDetected && !barrierOpen && availableSlots > 0) {
    checkForCar(thresholdDistance);                   // Auto prüfen, wenn Schranke zu
  }

  if (carDetected && !barrierOpen) {                  // Auto erkannt, Schranke noch zu?
    barrierOpen = true;
    barrierServo.write(openAngle);                    // Schranke öffnen
    Serial.println("Schranke öffnet.");
    delay(100);
  }

  if (carDetected && barrierOpen) {                   // Wenn Schranke offen, prüfen ob Auto weg
    confirmCarGone(leaveThreshold);
  }

  // Ausgangsüberwachung
  checkCarLeaving(thresholdDistance);                 // Autoausfahrt prüfen

  delay(100);                                         // Schleifenpause
}
