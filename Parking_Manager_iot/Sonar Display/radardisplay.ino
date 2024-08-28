
#include <ESP32Servo.h>  // Include the Servo library

const int trigPin = 16;
const int echoPin = 17;
long duration;
int distance;

const int servoPin = 23;
Servo myServo;

void setup() {
  Serial.begin(115200);  // Start the Serial communication

  // Set up the pins for the ultrasonic sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  myServo.attach(servoPin);  // Attach the servo to the servo pin
}

void loop() {
  scanRadar();
}

void scanRadar() {
  for (int i = 15; i <= 165; i++) {
    myServo.write(i);
    delay(30);
    distance = calculateDistance();
    Serial.print(i);
    Serial.print(",");
    Serial.print(distance);
    Serial.print(".");
  }
  for (int i = 165; i > 15; i--) {
    myServo.write(i);
    delay(30);
    distance = calculateDistance();
    Serial.print(i);
    Serial.print(",");
    Serial.print(distance);
    Serial.print(".");
  }
}

int calculateDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  float distance = pulseIn(echoPin, HIGH) / 58.00;
  return distance;
}

String generateData() {
  String data = "";
  for (int i = 15; i <= 165; i++) {
    data += String(i) + "," + String(distance) + ".";
  }
  return data;
}
