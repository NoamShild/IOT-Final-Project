/* Final Project
In this project, we will develop a smart parking lot system that not only allows users to reserve
parking slots in real-time but also helps them save time by indicating available slots instantly.
This means no more circling around looking for an open spot in crowded lots.
Our system utilizes an ESP32 to manage the entry and exit gates, perform scans of the parking lot
to identify available slots, and communicate this data to an HTML-based server for user access. 

  sensors:
  1. radar servo motor - pin 23
  2. gate servo motor - pin 22
  3. LED strip - pin 18
  4. radar distance sensor - pins 16 , 17
  5. gate distance sensor - pins 32 , 33

  Instructable link: https://www.instructables.com/Smart-Parking-Lot-System-With-Arduino-and-HTML/

  Created by:
  1. Nimrod Boazi - 208082735
  2. Noam Shildekraut - 315031864
*/
#include <Arduino.h>
#include <ESP32Servo.h>  // Include the Servo library
#include <WiFi.h>
#include <HTTPClient.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>  // Include the NeoPixel library
#include <BlynkSimpleEsp32.h>

#define radarServoPin 23 // Pin for radar servo
#define gateServoPin 22 // Pin for gate servo
#define LED_PIN 18           // Pin connected to the NeoPixel strip
#define NUM_LEDS 12          // Total number of LEDs on the strip
#define BLYNK_TEMPLATE_ID "TMPL6Pd6Pw10v" // Blynk authentication token
#define BLYNK_TEMPLATE_NAME "MyDevice" 
#define BLYNK_PRINT Serial

//blynk authorization
char auth[] = "zxW5oD8QA05piWQHBBimIRgOTjVhgzRZ";

AsyncWebServer server(80); // Initialize web server on port 80
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);  // Initialize the LED strip
BlynkTimer timer;

const char* ssid = "matrix"; // wifi name
const char* password = "abcdeabcde"; // wifi password

// variable and method declarations
int pendingSlotId = 0;  // Global variable to store the slot ID to be freed
void openGate();
void scanRadar();
String processor(const String &var);
void updateLEDs();  // Function to update the LEDs based on slot status
int calculateDistance(int trigPinParam, int echoPinParam);

//Pins and variables for radar distance sensor
const int radarTrigPin = 16; 
const int radarEchoPin = 17;
int distance; // distance of object from radar
bool parkingArray[3] = {false, false, false}; // Current garage status
bool isSlotFree = true; 
const int scanInterval = 30; //interval between radar movements
unsigned long previousRadarMillis = 0; //track time from last radar movement
int radarPosition = 15;
int radarDirection = 1; // 1 = moving forward, -1 moving backward
int slotIndex = 0; // Current slot being checked by radar
bool scanning = true; //track scanning status 

// Pins and variables for gate distance sensor
const int gateTrigPin = 32;
const int gateEchoPin = 33;
unsigned long previousGateMillis = 0; // Track time since last gate movement
const long gateInterval = 1; // time between gate movements
int gateServoPosition = 0;
int gateState = 0;
bool gateOpening = false;
bool checkForTrigger = true;

//Servo declarations
Servo radarServo;
Servo gateServo;

//flag to keep gate state
int gateFlag = 0;
// old state variables
int gatePrevious = 0;

//Blynk scenario setup
BLYNK_WRITE(V0)
{
  gateFlag = param.asInt();
}

void setup() {
  Serial.begin(115200);

  //Spiffs init
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //distance sensor's init
  pinMode(radarTrigPin, OUTPUT);
  pinMode(radarEchoPin, INPUT);
  pinMode(gateTrigPin, OUTPUT);
  pinMode(gateEchoPin, INPUT);

  //servo's init
  radarServo.attach(radarServoPin);
  gateServo.attach(gateServoPin);

  //LED init
  strip.begin();           // Initialize the LED strip
  strip.show();            // Turn off all LEDs initially
  strip.setBrightness(50); // Set brightness (optional)

  //Wifi init
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while(WiFi.waitForConnectResult() != WL_CONNECTED){
      Serial.print(".");
  }
    
  Serial.println("Connected to WiFi");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Server web data declarations
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  server.on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.css", "text/css");
  });

  server.on("/index.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.js", "application/javascript");
  });

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest *request){
  request->send(SPIFFS, "/admin.html", String(), false, processor);
  });

  server.on("/admin.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/admin.css", "text/css");
  });

  server.on("/admin.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/admin.js", "application/javascript");
  });

  //if a slot has been freed, send to website upon request
  server.on("/freeSlotCheck", HTTP_GET, [](AsyncWebServerRequest *request){
    if (pendingSlotId != 0) {
        String response = "{\"slotId\":" + String(pendingSlotId) + "}";
        request->send(200, "application/json", response);
        Serial.println("freeSlotCheck Response: " + response); // Debugging output
        pendingSlotId = 0;  // Reset after sending
    } else {
        request->send(200, "application/json", "{}");  // No slot to free
    }
  });

  // send parking slots status upon request
  server.on("/getSensorStatus", HTTP_GET, [](AsyncWebServerRequest *request){
    String slotStatus = "[";
    for (int i = 0; i < 3; i++) {
        slotStatus += (parkingArray[i] ? "true" : "false");
        if (i < 2) slotStatus += ",";
    }
    slotStatus += "]";
    request->send(200, "application/json", slotStatus);
  });

  server.on("/steeringWheel.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/steeringWheel.png", "image/png");
  });

  server.on("/car.png", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/car.png", "image/png");
  });

  server.on("/openGate", HTTP_GET, [](AsyncWebServerRequest *request){
    openGate();  // Trigger the openGate function
    request->send(200, "text/plain", "Gate is opening");  // Send a response to confirm the action
    Serial.println("Received request to open gate");
});

  //start server
  server.begin();
  Serial.println("HTTP server started");

  //start blynk
  Blynk.begin(auth, ssid, password, "blynk.cloud", 80);
  // timer.setInterval(1000L, senddata);
}

void loop() {
  delay(1);
  
  Blynk.run();
  timer.run();

  //Servo click
  if (gateFlag != gatePrevious)
  {
    Serial.println("Move gate");
    openGate();
    gatePrevious = gateFlag;
  }
  
  scanRadar();
  int distanceFromGate = calculateDistance(gateTrigPin, gateEchoPin);
  
  //Code for working gate without using delays
  // if (distanceFromGate < 20 && checkForTrigger) {
  //   Serial.println("gate sensor triggered: Opening Gate");
  //   gateOpening = true;
  //   checkForTrigger = false;
  //   previousGateMillis = millis();
  // }
  // Serial.println(distanceFromGate);

  //If a car is within 20 cm's of gate, open
  if (distanceFromGate < 20) {
    openGate();
  }
}

//Open gate function without delays (did not work for us due to power supply issues)
// void openGate() {
//   unsigned long currentGateMillis = millis();

//   switch (gateState) {
//     case 0:
//       if (currentGateMillis - previousGateMillis >= gateInterval) {
//         previousGateMillis = currentGateMillis;
//         gateServo.write(gateServoPosition);
//         gateServoPosition++;
//         if (gateServoPosition >= 90) {
//           gateServoPosition = 90;
//           gateState = 1;
//           previousGateMillis = currentGateMillis;
//         }
//       }
//       break;

//     case 1:
//       if (currentGateMillis - previousGateMillis >= 5000) {
//         gateState = 2;
//         previousGateMillis = currentGateMillis;
//       }
//       break;

//     case 2:
//       if (currentGateMillis - previousGateMillis >= gateInterval) {
//         previousGateMillis = currentGateMillis;
//         gateServo.write(gateServoPosition);
//         gateServoPosition--;
//         if (gateServoPosition <= 0) {
//           gateServoPosition = 0;
//           gateState = 0;
//           gateOpening = false;
//           checkForTrigger = true;
//           Serial.println("Gate cycle complete. Waiting for the next trigger.");
//         }
//       }
//       break;
//   }
// }

//Open gate function with delays
void openGate() {
  // Gradually move the servo to 90 degrees
  for (int pos = 105; pos >= 5; pos--) {
    gateServo.write(pos);  // Move to the next degree
    delay(5);              // Wait for 2 milliseconds
  }

  delay(5000);  // Wait for 5 seconds

  // Gradually move the servo back to 0 degrees
  for (int pos = 5; pos <= 105; pos++) {
    gateServo.write(pos);  // Move back to the previous degree
    delay(5);              // Wait for 2 milliseconds
  }
}

//calculate distance from distance sensor
int calculateDistance(int trigPinParam, int echoPinParam) {
    digitalWrite(trigPinParam, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPinParam, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPinParam, LOW);
    float distance = pulseIn(echoPinParam, HIGH) / 58.00;
    return distance;
}

//radar movement and scan function
void scanRadar() {
    const int thresholdDistance = 15;  // Threshold distance to consider the slot occupied
    const int requiredReadings = 5;    // Number of readings below the threshold to consider the slot occupied

    static int detectionCounts[3] = {0, 0, 0};  // Counters for readings below the threshold for each slot

    unsigned long currentRadarMillis = millis();

    if (scanning && currentRadarMillis - previousRadarMillis >= scanInterval) {
        previousRadarMillis = currentRadarMillis;

        radarServo.write(radarPosition);
        int distance = calculateDistance(radarTrigPin, radarEchoPin);

        // Check if the current radar position is within the window for a specific slot
        if ((radarPosition >= 25 && radarPosition <= 35 && slotIndex == 0) ||
            (radarPosition >= 135 && radarPosition <= 145 && slotIndex == 2)) {
            
            // If the distance is less than the threshold, increment the detection counter for the slot
            if (distance < 20) {
                detectionCounts[slotIndex]++;
            }
        }

        else if(radarPosition >= 80 && radarPosition <= 100 && slotIndex == 1){
            // If the distance is less than the threshold, increment the detection counter for the slot
            if (distance < 15) {
                detectionCounts[slotIndex]++;
            }
        }

        // Move the radar to the next position
        radarPosition += radarDirection;

        // Change direction when reaching the end of the sweep range
        if (radarPosition >= 150) {
            radarPosition = 150;
            radarDirection = -1;
        } else if (radarPosition <= 20) {
            radarPosition = 20;
            radarDirection = 1;
        }

        // At the end of the sweep for a slot, update the slot's status
        if (radarPosition == 150 || radarPosition == 20) {
            bool isOccupied = (detectionCounts[slotIndex] >= requiredReadings);
            
            // If the status has changed, update the parking array and send a status update if necessary
            if (parkingArray[slotIndex] != isOccupied) {
                parkingArray[slotIndex] = isOccupied;

                if (!isOccupied) {
                    // Send free slot status update if the slot becomes free
                    Serial.println("Slot " + String(slotIndex + 1) + " is now free");
                    pendingSlotId = slotIndex + 1;
                    // Add your code to handle the free slot status update here
                }
            }

            // Reset the detection counter for the next sweep
            detectionCounts[slotIndex] = 0;
            
            slotIndex++;
            if (slotIndex > 2) {
                slotIndex = 0;
            }

            updateLEDs();  // Update the LEDs after the scan
        }
    }
}

//LED update function
void updateLEDs() {
    for (int i = 0; i < 3; i++) {
        int led1 = i * 4 + 1;
        int led2 = i * 4 + 2;

        if (parkingArray[i]) {
            strip.setPixelColor(led1, strip.Color(255, 0, 0));  // Red if occupied
            strip.setPixelColor(led2, strip.Color(255, 0, 0));  // Red if occupied
        } else {
            strip.setPixelColor(led1, strip.Color(0, 255, 0));  // Green if free
            strip.setPixelColor(led2, strip.Color(0, 255, 0));  // Green if free
        }
    }
    strip.show();  // Update the LEDs with the new colors
}

//String for html sensor status
String processor(const String &var) {
  String val_list;
  if (var == "SLOTSSTATUS") {
    for (int i = 0; i < 3; i++) {
      val_list += "<p>Slot ";
      val_list += String(i + 1);
      val_list += " is ";
      if(parkingArray[i]){
        val_list += " Occupied</p>";
      } else{
        val_list += " free</p>";
      }
    }
    return val_list;
  }

  return String();
}
