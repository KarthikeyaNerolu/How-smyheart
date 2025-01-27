#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#ifndef STASSID
#define STASSID "Motorola Edge 40"
#define STAPSK "motoedge40"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

const int led = 13;
const int loPlusPin = 5;  // LO+ pin
const int loMinusPin = 4; // LO- pin

// Heart rate variables
unsigned long lastTime = 0;
int heartRate = 0;
unsigned long lastHeartBeatTime = 0;

void handleRoot() {
  digitalWrite(led, 1);

  // Calculate heart rate condition
  String condition = getHeartCondition(heartRate);

  String response = "Heart Rate: " + String(heartRate) + " BPM\n";
  response += "Condition: " + condition;

  server.send(200, "application/json", response);
  digitalWrite(led, 0);
}

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

// Function to classify heart condition based on heart rate
String getHeartCondition(int bpm) {
  if (bpm < 60) {
    return "Bradycardia (Low Heart Rate)";
  } else if (bpm >= 60 && bpm <= 100) {
    return "Normal Heart Rate";
  } else {
    return "Tachycardia (High Heart Rate)";
  }
}

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  pinMode(loPlusPin, INPUT);
  pinMode(loMinusPin, INPUT);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {

  if (digitalRead(loPlusPin) == HIGH || digitalRead(loMinusPin) == HIGH) {
    Serial.println("Leads off detected!");
  } else {

    int sensorValue = analogRead(A0);
    if (millis() - lastHeartBeatTime > 1000) {
      heartRate = map(sensorValue, 0, 1023, 40, 120);
      lastHeartBeatTime = millis();
    }

    // Print heart rate to Serial Monitor
    Serial.print("Heart Rate: ");
    Serial.print(heartRate);
    Serial.println(" BPM");

    String condition = getHeartCondition(heartRate);
    Serial.println("Condition: " + condition);
  }

  server.handleClient();
  MDNS.update();

  delay(60000);
}
