#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
// OLED Display Einstellungen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
// Pin-Zuordnungen Eingang
const int trigPin = 12;
const int echoPin = 13;
const int servoPin = 9;
 
// Pin-Zuordnungen Ausgang
const int trigPin2 = 7;
const int echoPin2 = 8;
 
// Schranken-Parameter
const int closedAngle = 0;
const int openAngle = 90;
const long thresholdDistance = 10; // Erkennungsschwelle in cm
const long leaveThreshold = 10;
 
const int totalSlots = 3;
int availableSlots = totalSlots;
bool carDetected = false;
bool barrierOpen = false;
bool carLeavingDetected = false;
 
Servo barrierServo;
 
// ----------- FUNKTIONEN FÜR SLOT-ÄNDERUNGEN -----------
 
void decreaseAvailableSlots() {
  if (availableSlots > 0) {
    availableSlots--;
    Serial.print("Auto eingefahren. Neue freie Plätze: ");
    Serial.println(availableSlots);
    updateDisplay();
  } else {
    Serial.println("Parkhaus voll. Kein Abzug.");
  }
}
 
void increaseAvailableSlots() {
  if (availableSlots < totalSlots) {
    availableSlots++;
    Serial.print("Auto ausgefahren. Neue freie Plätze: ");
    Serial.println(availableSlots);
    updateDisplay();
  } else {
    Serial.println("Parkhaus war schon leer.");
  }
}
 
// -------------------------------------------------------
 
void setup() {
  delay(500); // Für OLED-Stabilität
  Serial.begin(9600);
  barrierServo.attach(servoPin);
  barrierServo.write(closedAngle);
 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
 
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED konnte nicht initialisiert werden.");
  } else {
    display.clearDisplay();
    updateDisplay();
  }
}
 
// Abstand messen (Eingang)
float getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(5000);
  digitalWrite(trigPin, LOW);
 
  long duration = pulseIn(echoPin, HIGH, 300000);
  float distance = duration * 0.034 / 2;
  return distance;
}
 
// Abstand messen (Ausgang)
float getDistanceExit() {
  digitalWrite(trigPin2, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin2, HIGH);
  delayMicroseconds(5000);
  digitalWrite(trigPin2, LOW);
 
  long duration = pulseIn(echoPin2, HIGH, 300000);
  float distanceExit = duration * 0.034 / 2;
  return distanceExit;
}
 
// Display aktualisieren
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
 
  if (availableSlots > 0) {
    display.setCursor(0, 5);
    display.print("Freie\n");
    display.print("Plaetze\n");
    display.setCursor(50, 40);
    display.setTextSize(3);
    display.print(availableSlots);
  } else {
    display.setCursor(0, 5);
    display.print("Parkhaus\n\n");
    display.setTextSize(3);
    display.print("Voll\n");
  }
 
  display.display();
}
 
// Auto erkannt am Eingang
void checkForCar(double threshold) {
  float d1 = getDistance();
  delay(500);
  float d2 = getDistance();
 
  if (d1 < threshold && d2 < threshold) {
    carDetected = true;
    Serial.println("Auto erkannt (Eingang)");
  }
}
 
// Auto ist wirklich weg (Eingang)
void confirmCarGone(double threshold) {
  float d1 = getDistance();
  delay(500);
  float d2 = getDistance();
 
  if (d1 > threshold && d2 > threshold) {
    carDetected = false;
    barrierOpen = false;
 
    decreaseAvailableSlots(); //  SLOT -1
 
    Serial.println("Schranke schließt.");
    barrierServo.write(closedAngle);
    delay(500);
  }
}
 
// Auto erkannt am Ausgang
void checkCarLeaving(double threshold) {
  float d1 = getDistanceExit();
  delay(500);
  float d2 = getDistanceExit();
 
  if (d1 < threshold && d2 < threshold) {
    if (!carLeavingDetected) {
      carLeavingDetected = true;
      increaseAvailableSlots(); //  SLOT +1
    }
  } else {
    carLeavingDetected = false;
  }
}
 
// Hauptloop
void loop() {
  float distance = getDistance();
  Serial.print("Live Abstand Eingang: ");
  Serial.print(distance);
  Serial.println(" cm");
 
  if (!carDetected && !barrierOpen && availableSlots > 0) {
    checkForCar(thresholdDistance);
  }
 
  if (carDetected && !barrierOpen) {
    barrierOpen = true;
    barrierServo.write(openAngle);
    Serial.println("Schranke öffnet.");
    delay(100);
  }
 
  if (carDetected && barrierOpen) {
    confirmCarGone(leaveThreshold);
  }
 
  // Ausgangsüberwachung
  checkCarLeaving(thresholdDistance);
 
  delay(100);
}
