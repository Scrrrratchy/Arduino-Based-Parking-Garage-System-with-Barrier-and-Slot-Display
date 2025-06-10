#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
// OLED Display Einstellungen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
// Pin-Zuordnungen
const int trigPin   = 12;
const int echoPin   = 13;
const int servoPin  = 9;
 
// Schranken-Parameter
const int closedAngle = 0;
const int openAngle   = 90;
const long thresholdDistance = 10; // Abstand unter dem ein Auto erkannt wird
const long leaveThreshold = 10;   // Abstand über dem ein Auto als "weg" gilt
 
// Parkhaus-Parameter
const int totalSlots = 3;
int availableSlots = totalSlots;
bool carDetected = false;
bool barrierOpen = false;
 
Servo barrierServo;
 
// Setup
void setup() {
  Serial.begin(9600);
  barrierServo.attach(servoPin);
  barrierServo.write(closedAngle);
 
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
 
  // OLED Initialisierung
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED-Fehler!");
    while (true);
  }
  display.clearDisplay();
  updateDisplay();
}
 
// Abstand messen
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
 
// Anzeige aktualisieren
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
 
// Autoerkennung mit 2 Messungen unterhalb Schwellenwert
void checkForCar(double threshold) {
  float firstDistance = getDistance();
  Serial.print("Messung 1: ");
  Serial.print(firstDistance);
  Serial.println(" cm");
 
  delay(500); // 2.5 Sekunden warten
 
  float secondDistance = getDistance();
  Serial.print("Messung 2: ");
  Serial.print(secondDistance);
  Serial.println(" cm");
 
  if (firstDistance < threshold && secondDistance < threshold) {
    carDetected = true;
    Serial.println("Auto sicher erkannt. carDetected = true");
  } else {
    Serial.println("Auto nicht stabil erkannt.");
  }
}
 
// Prüfen ob Auto wirklich weg ist (2x > leaveThreshold)
void confirmCarGone(double threshold) {
  float firstDistance = getDistance();
  Serial.print("Weg-Messung 1: ");
  Serial.print(firstDistance);
  Serial.println(" cm");
 
  delay(500); // 2.5 Sekunden warten
 
  float secondDistance = getDistance();
  Serial.print("Weg-Messung 2: ");
  Serial.print(secondDistance);
  Serial.println(" cm");
 
  if (firstDistance > threshold && secondDistance > threshold) {
    carDetected = false;
    barrierOpen = false;
 
    if (availableSlots > 0) {
      availableSlots--;
      Serial.print("Verfügbare Plätze: ");
      Serial.println(availableSlots);
      updateDisplay();
    } else {
      Serial.println("Parkhaus voll!");
    }
 
    Serial.println("Schranke schließt.");
    barrierServo.write(closedAngle);
    delay(500);
  } else {
    Serial.println("Auto noch in der Nähe – Schranke bleibt offen.");
  }
}
 
// Hauptloop
void loop() {
  float distance = getDistance();
 
  Serial.print("Live-Abstand: ");
  Serial.print(distance);
  Serial.println(" cm");
 
  // Auto erkennen wenn Schranke zu ist
  if (!carDetected && !barrierOpen && availableSlots > 0) {
    checkForCar(thresholdDistance);
  }
 
  // Schranke öffnen wenn Auto erkannt
  if (carDetected && !barrierOpen) {
    barrierOpen = true;
    barrierServo.write(openAngle);
    Serial.println("Schranke öffnet.");
    delay(100);
  }
 
  // Auto soll weg sein → 2 Messungen bestätigen das
  if (carDetected && barrierOpen) {
    confirmCarGone(leaveThreshold);
  }
 
  delay(100);
}