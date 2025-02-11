#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Replace these with your WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// IFTTT webhook key and event name
const char* iftttWebhookKey = "bfOh-57I2CvFbJitZqX1gH";
const char* eventName = "angle_exceeded";

// Define pin numbers
const int angleSensorPin = A0;  // Analog pin for angle sensor
const int relayPin = D1;        // Digital pin for relay control

// Define threshold angle
const float thresholdAngle = 45.0;  // Threshold angle in degrees

void setup() {
  Serial.begin(9600);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("WiFi connected");

  // Set relay pin as output
  pinMode(relayPin, OUTPUT);
}

void loop() {
  // Read angle from sensor
  float angle = readAngle();

  // Check if angle exceeds threshold
  if (angle > thresholdAngle) {
    // Activate relay
    digitalWrite(relayPin, HIGH);

    // Send message via IFTTT webhook
    sendIFTTTMessage();
  } else {
    // Deactivate relay
    digitalWrite(relayPin, LOW);
  }

  delay(1000);
}

float readAngle() {
  // Read analog value from sensor
  int sensorValue = analogRead(angleSensorPin);

  // Convert analog value to angle (example: linear conversion)
  float angle = map(sensorValue, 0, 1023, 0, 360);

  return angle;
}

void sendIFTTTMessage() {
  // Create HTTPClient object
  HTTPClient http;

  // Construct URL with webhook key and event name
  String url = "http://maker.ifttt.com/trigger/";
  url += eventName;
  url += "/with/key/";
  url += iftttWebhookKey;

  // Send HTTP POST request
  http.begin(url);
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    Serial.print("IFTTT message sent, response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("Failed to send IFTTT message, error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}
