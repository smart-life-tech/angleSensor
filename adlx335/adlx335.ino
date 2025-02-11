#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ADXL335.h> // Include the ADXL335 library

// Replace these with your WiFi credentials
const char *ssid = "YOUR_WIFI_SSID";
const char *password = "YOUR_WIFI_PASSWORD";

// IFTTT webhook key
const char *iftttWebhookKey = "bfOh-57I2CvFbJitZqX1gH";

// Define pins for ADXL335
const int xPin = D0; // Analog pin for X-axis
const int yPin = D1; // Analog pin for Y-axis
const int zPin = D2; // Analog pin for Z-axis
int relayPin = D3;
// Define threshold angle
const float thresholdAngle = 45.0; // Threshold angle in degrees
// Variable to track if SMS has been sent
bool smsSent = false;

// Create an instance of the ADXL335 class
ADXL335 accelerometer(xPin, yPin, zPin);

void setup()
{
    Serial.begin(9600);
    pinMode(relayPin, OUTPUT);
    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("WiFi connected");
}

void loop()
{
    // Read acceleration values from ADXL335
    float xAccel = accelerometer.getX();
    float yAccel = accelerometer.getY();
    float zAccel = accelerometer.getZ();

    // Convert acceleration values to angles (example: using atan2 function)
    float xAngle = atan2(yAccel, zAccel) * 180.0 / PI;
    float yAngle = atan2(-xAccel, sqrt(yAccel * yAccel + zAccel * zAccel)) * 180.0 / PI;
    float zAngle = atan2(sqrt(xAccel * xAccel + yAccel * yAccel), zAccel) * 180.0 / PI;

    // Print angles
    Serial.print("X Angle: ");
    Serial.print(xAngle);
    Serial.print(" degrees, Y Angle: ");
    Serial.print(yAngle);
    Serial.print(" degrees, Z Angle: ");
    Serial.println(zAngle);

    // Check if any angle exceeds threshold
    if (abs(xAngle) > thresholdAngle || abs(yAngle) > thresholdAngle || abs(zAngle) > thresholdAngle)
    {
        // If SMS has not been sent yet, send it
        if (!smsSent)
        {
            // Send message via IFTTT webhook
            sendIFTTTMessage();

            // Set flag to indicate SMS has been sent
            smsSent = true;

            // Activate relay
            digitalWrite(relayPin, HIGH);
        }
    }
    else
    {
        // Reset SMS sent flag and deactivate relay
        smsSent = false;
        digitalWrite(relayPin, LOW);
    }

    delay(1000);
}

void sendIFTTTMessage()
{
    // Create HTTPClient object
    HTTPClient http;

    // Construct JSON data
    String jsonData = "{\"value1\":\"Angle exceeded 45 degrees!\"}";

    // Construct URL with webhook key
    String url = "http://maker.ifttt.com/trigger/sms/with/key/";
    url += iftttWebhookKey;

    // Send HTTP POST request
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonData);
    if (httpResponseCode > 0)
    {
        Serial.print("IFTTT message sent, response code: ");
        Serial.println(httpResponseCode);
    }
    else
    {
        Serial.print("Failed to send IFTTT message, error code: ");
        Serial.println(httpResponseCode);
    }
    http.end();
}
