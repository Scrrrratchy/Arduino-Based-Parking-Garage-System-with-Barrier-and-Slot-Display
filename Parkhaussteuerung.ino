// --- Bibliotheken einbinden ---
#include <Servo.h>                    // Steuerung von Servomotoren
#include <Wire.h>                     // I2C-Kommunikation (z. B. für OLED-Display)
#include <Adafruit_GFX.h>            // Grundfunktionen für grafische Displays
#include <Adafruit_SSD1306.h>        // Bibliothek für SSD1306 OLED-Display

// --- OLED Display Konfiguration ---
#define SCREEN_WIDTH 128             // Displaybreite in Pixel
#define SCREEN_HEIGHT 64             // Displayhöhe in Pixel
#define OLED_RESET -1                // Kein Reset-Pin vorhanden
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // OLED-Objekt

// --- Pinzuweisungen für Sensorik und Aktorik ---
const int trigPinIn  = 12;           // Triggersignal Eingangssensor (Ultraschall)
const int echoPinIn  = 13;           // Echosignal Eingangssensor
const int servoPin   = 9;            // Steuerpin für Servomotor (Schranke)

const int trigPinOut = 7;            // Triggersignal Ausgangssensor
const int echoPinOut = 8;            // Echosignal Ausgangssensor

// --- Steuerwinkel der Schranke ---
const int closedAngle = 0;           // Winkel, wenn Schranke geschlossen ist
const int openAngle   = 90;          // Winkel, wenn Schranke geöffnet ist

// --- Abstandsschwellen in Zentimetern ---
const long thresholdDistance = 10;   // Schwelle für Autoerkennung an Ein-/Ausfahrt
const long leaveThreshold     = 10;  // Schwelle für "Auto hat Bereich verlassen"

// --- Parkhauslogik ---
const int totalSlots = 3;            // Maximale Anzahl an Parkplätzen
int availableSlots = totalSlots;     // Anzahl aktuell freier Plätze
bool carDetected = false;            // Status: Fahrzeug erkannt an der Einfahrt?
bool barrierOpen = false;            // Status: Schranke aktuell geöffnet?

Servo barrierServo;                  // Servoobjekt für Schranke

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);                          // Serielle Kommunikation starten (Debug)

  barrierServo.attach(servoPin);              // Servomotor initialisieren
  barrierServo.write(closedAngle);            // Schranke schließen

  // Pins für Ultraschallsensoren konfigurieren
  pinMode(trigPinIn, OUTPUT);
  pinMode(echoPinIn, INPUT);
  pinMode(trigPinOut, OUTPUT);
  pinMode(echoPinOut, INPUT);

  // OLED-Display initialisieren
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED-Fehler!");
    while (true);                              // Endlosschleife bei Fehler
  }

  display.clearDisplay();                      // Display löschen
  updateDisplay();                             // Freie Plätze anzeigen
}

// ---------- ABSTANDSMESSUNG ----------
float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);                  // Sensor vorbereiten
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);                 // Ultraschall auslösen
  delayMicroseconds(5000);
  digitalWrite(trigPin, LOW);

  // Zeit messen, bis Echo empfangen wird
  long duration = pulseIn(echoPin, HIGH, 300000);
  // Umrechnung in Distanz (Schallgeschwindigkeit 0.034 cm/μs, geteilt durch 2)
  float distance = duration * 0.034 / 2;
  return distance;
}

// ---------- DISPLAY-AKTUALISIERUNG ----------
void updateDisplay() {
  display.clearDisplay();                      // Display zurücksetzen
  display.setTextSize(2);
  display.setTextColor(WHITE);

  if (availableSlots > 0) {
    display.setCursor(0, 5);
    display.print("Freie\nPlaetze\n");         // Anzeige bei freien Plätzen
    display.setCursor(50, 40);
    display.setTextSize(3);
    display.print(availableSlots);
  } else {
    display.setCursor(0, 5);
    display.print("Parkhaus\n\n");             // Anzeige wenn voll
    display.setTextSize(3);
    display.print("Voll");
  }

  display.display();                           // Anzeige aktualisieren
}

// ---------- AUTO AN EINFAHRT ERKENNEN ----------
void checkForCar(double threshold) {
  float firstDistance = getDistance(trigPinIn, echoPinIn);
  delay(2500);                                 // kurze Wartezeit
  float secondDistance = getDistance(trigPinIn, echoPinIn);

  // Wenn beide Messungen kleiner als der Schwellenwert sind → Auto erkannt
  if (firstDistance < threshold && secondDistance < threshold) {
    carDetected = true;
    Serial.println("Auto am Eingang erkannt.");
  }
}

// ---------- AUTO IST DURCHGEFAHREN ----------
void confirmCarGone(double threshold) {
  float firstDistance = getDistance(trigPinIn, echoPinIn);
  delay(2500);
  float secondDistance = getDistance(trigPinIn, echoPinIn);

  // Wenn Fahrzeugbereich leer ist, Schranke schließen und Platz abziehen
  if (firstDistance > threshold && secondDistance > threshold) {
    carDetected = false;
    barrierOpen = false;

    if (availableSlots > 0) {
      availableSlots--;                        // Parkplatz belegen
      updateDisplay();                         // Anzeige aktualisieren
    }

    barrierServo.write(closedAngle);           // Schranke schließen
    delay(500);                                // kurze Pause für Bewegung
  }
}

// ---------- AUTO VERLÄSST PARKHAUS ----------
void checkCarLeaving(double threshold) {
  float firstDistance = getDistance(trigPinOut, echoPinOut);
  delay(2500);
  float secondDistance = getDistance(trigPinOut, echoPinOut);

  // Wenn beide Messungen kleiner als Schwelle → Auto fährt aus
  if (firstDistance < threshold && secondDistance < threshold) {
    if (availableSlots < totalSlots) {
      availableSlots++;                        // Parkplatz freigeben
      updateDisplay();                         // Anzeige aktualisieren
    }
    delay(2000);                               // Schutz gegen Mehrfachzählung
  }
}

// ---------- HAUPTSCHLEIFE ----------
void loop() {
  // Prüfen, ob Auto an Einfahrt erkannt werden soll
  if (!carDetected && !barrierOpen && availableSlots > 0) {
    checkForCar(thresholdDistance);
  }

  // Schranke öffnen, wenn Auto erkannt wurde
  if (carDetected && !barrierOpen) {
    barrierOpen = true;
    barrierServo.write(openAngle);             // Schranke öffnen
    delay(1000);                               // Zeit zum Einfahren
  }

  // Prüfen, ob Auto den Bereich verlassen hat → Schranke schließen
  if (carDetected && barrierOpen) {
    confirmCarGone(leaveThreshold);
  }

  // Prüfen, ob ein Auto das Parkhaus verlässt
  checkCarLeaving(thresholdDistance);

  delay(100);                                  // Mikrocontroller entlasten
}
