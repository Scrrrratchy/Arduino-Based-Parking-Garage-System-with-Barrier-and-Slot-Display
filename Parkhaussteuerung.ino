#include <Servo.h>                  // Bibliothek zur Ansteuerung von Servomotoren
#include <Wire.h>                   // I2C-Bibliothek für Kommunikation mit dem OLED
#include <Adafruit_GFX.h>          // Grafikbibliothek für grundlegende Displayfunktionen
#include <Adafruit_SSD1306.h>      // Bibliothek für das SSD1306 OLED Display

// OLED Display Einstellungen
#define SCREEN_WIDTH 128           // Breite des Displays in Pixel
#define SCREEN_HEIGHT 64           // Höhe des Displays in Pixel
#define OLED_RESET -1              // Kein Reset-Pin verwendet

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // OLED-Display Objekt

// Pin-Zuordnungen
const int trigPin   = 12;          // Trigger-Pin des Ultraschallsensors
const int echoPin   = 13;          // Echo-Pin des Ultraschallsensors
const int servoPin  = 9;           // PWM-Pin für den Servo (Schranke)

// Schranken-Parameter
const int closedAngle = 0;         // Winkel für geschlossene Schranke
const int openAngle   = 90;        // Winkel für geöffnete Schranke
const long thresholdDistance = 10; // Abstand in cm: Auto erkannt, wenn unter diesem Wert
const long leaveThreshold = 10;    // Abstand in cm: Auto ist weg, wenn über diesem Wert

// Parkhaus-Parameter
const int totalSlots = 3;          // Gesamtanzahl an Parkplätzen
int availableSlots = totalSlots;   // Aktuell freie Plätze
bool carDetected = false;          // Status: Auto erkannt?
bool barrierOpen = false;          // Status: Schranke offen?

Servo barrierServo;                // Servo-Objekt für die Schrankensteuerung

void setup() {
  Serial.begin(9600);                      // Serielle Ausgabe für Debugging
  barrierServo.attach(servoPin);          // Servo an definierten Pin anschließen
  barrierServo.write(closedAngle);        // Schranke zu Beginn schließen

  pinMode(trigPin, OUTPUT);               // Trigger als Ausgang
  pinMode(echoPin, INPUT);                // Echo als Eingang

  // OLED Display initialisieren
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED-Fehler!");
    while (true);                         // Endlosschleife bei Fehler
  }

  display.clearDisplay();                 // OLED leeren
  updateDisplay();                        // Erste Anzeige zeigen
}

// Misst den Abstand mithilfe des Ultraschallsensors
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);           // Ultraschall senden
  delayMicroseconds(5000);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH, 300000);  // Zeit messen (Timeout nach 300ms)
  float distance = duration * 0.034 / 2;           // Umrechnen in cm
  return distance;
}

// Aktualisiert das OLED Display je nach verfügbarem Platz
void updateDisplay() {
  display.clearDisplay();                // Anzeige leeren
  display.setTextSize(2);
  display.setTextColor(WHITE);

  if (availableSlots > 0) {
    display.setCursor(0, 5);
    display.print("Freie\n");
    display.print("Plaetze\n");
    display.setCursor(50, 40);
    display.setTextSize(3);
    display.print(availableSlots);       // Zeige freie Plätze
  } else {
    display.setCursor(0, 5);
    display.print("Parkhaus\n\n");
    display.setTextSize(3);
    display.print("Voll\n");             // Parkhaus voll
  }

  display.display();                     // Anzeige ausgeben
}

// Erkennt ein Auto, wenn 2 Messungen unterhalb des Schwellwerts sind
void checkForCar(double threshold) {
  float firstDistance = getDistance();   // Erste Messung
  Serial.print("Messung 1: ");
  Serial.print(firstDistance);
  Serial.println(" cm");

  delay(500);                            // 0,5s warten

  float secondDistance = getDistance();  // Zweite Messung
  Serial.print("Messung 2: ");
  Serial.print(secondDistance);
  Serial.println(" cm");

  if (firstDistance < threshold && secondDistance < threshold) {
    carDetected = true;                  // Auto erkannt
    Serial.println("Auto sicher erkannt. carDetected = true");
  } else {
    Serial.println("Auto nicht stabil erkannt.");
  }
}

// Bestätigt, dass das Auto weg ist (2 Messungen über Schwellwert)
void confirmCarGone(double threshold) {
  float firstDistance = getDistance();   // Erste Messung
  Serial.print("Weg-Messung 1: ");
  Serial.print(firstDistance);
  Serial.println(" cm");

  delay(500);                            // 0,5s warten

  float secondDistance = getDistance();  // Zweite Messung
  Serial.print("Weg-Messung 2: ");
  Serial.print(secondDistance);
  Serial.println(" cm");

  if (firstDistance > threshold && secondDistance > threshold) {
    carDetected = false;
    barrierOpen = false;

    if (availableSlots > 0) {
      availableSlots--;                 // Ein Platz wird als belegt markiert
      Serial.print("Verfügbare Plätze: ");
      Serial.println(availableSlots);
      updateDisplay();                  // Anzeige aktualisieren
    } else {
      Serial.println("Parkhaus voll!");
    }

    Serial.println("Schranke schließt.");
    barrierServo.write(closedAngle);    // Schranke schließen
    delay(500);
  } else {
    Serial.println("Auto noch in der Nähe – Schranke bleibt offen.");
  }
}

// Hauptprogramm
void loop() {
  float distance = getDistance();       // Live-Abstand messen
  Serial.print("Live-Abstand: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Auto erkennen, wenn Schranke zu und Plätze verfügbar
  if (!carDetected && !barrierOpen && availableSlots > 0) {
    checkForCar(thresholdDistance);
  }

  // Schranke öffnen, wenn Auto erkannt
  if (carDetected && !barrierOpen) {
    barrierOpen = true;
    barrierServo.write(openAngle);      // Schranke öffnen
    Serial.println("Schranke öffnet.");
    delay(100);
  }

  // Prüfen ob Auto weg ist
  if (carDetected && barrierOpen) {
    confirmCarGone(leaveThreshold);
  }

  delay(100);                           // Kurze Pause, um zu entprellen
}
