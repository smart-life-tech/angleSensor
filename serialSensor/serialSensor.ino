#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <SoftwareSerial.h>

// WiFi credentials
const char ssid[] PROGMEM = "YOUR_WIFI_SSID";
const char password[] PROGMEM = "YOUR_WIFI_PASSWORD";
const char iftttWebhookKey[] PROGMEM = "bfOh-57I2CvFbJitZqX1gH";

// Sensor objects
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
SoftwareSerial mySerial(D7, D6); // WT901C-TTL (RX=D7, TX=D6)

// Relay pin
const int relayPin = D5;

// Tilt & Temperature thresholds
const float tiltThreshold = -15.0;       // Hazardous tilt threshold
const float fallThreshold = -45.0;       // Likely pole breakage
const float tempThresholdFire = 158.0;   // Fire risk temperature
const float accelerationThreshold = 2.5; // High G-force impact (adjust based on testing)

bool relayActive = false; // Tracks relay status
WiFiClient client;        // Global WiFi client (persistent)
HTTPClient http;          // Global HTTP client (persistent)

// Function to read WT901C-TTL data (Tilt, Acceleration)
void ICACHE_FLASH_ATTR readWT901C(float &pitch, float &acceleration)
{
    uint8_t data[11];
    pitch = 0.0;
    acceleration = 1.0;

    if (mySerial.available() >= 11)
    {
        if (mySerial.read() == 0x55)
        {
            if (mySerial.read() == 0x53)
            { // Angle Data Frame
                for (int i = 0; i < 9; i++)
                {
                    data[i] = mySerial.read();
                }
                int16_t rawPitch = (data[3] << 8) | data[2];
                pitch = rawPitch / 32768.0 * 180.0; // Convert to degrees
            }
            else if (mySerial.read() == 0x51)
            { // Acceleration Data Frame
                for (int i = 0; i < 9; i++)
                {
                    data[i] = mySerial.read();
                }
                int16_t rawAccelZ = (data[5] << 8) | data[4];
                acceleration = rawAccelZ / 32768.0 * 16.0; // Convert to g-force
            }
        }
    }
}

// Function to read temperature from MLX90614
float ICACHE_FLASH_ATTR readTemperature()
{
    float tempF = mlx.readObjectTempF();
    return tempF;
}

// Function to send an IFTTT alert
void ICACHE_FLASH_ATTR sendIFTTTMessage(const char *message)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println(F("WiFi disconnected. Skipping alert."));
        return;
    }

    String url = F("http://maker.ifttt.com/trigger/alert/with/key/");
    url += iftttWebhookKey;

    http.begin(client, url);
    http.addHeader(F("Content-Type"), F("application/json"));

    char jsonData[100];
    snprintf(jsonData, sizeof(jsonData), "{\"value1\":\"%s\"}", message);

    int httpResponseCode = http.POST(jsonData);
    Serial.print(F("IFTTT alert response code: "));
    Serial.println(httpResponseCode);

    http.end();
}

// Setup function
void setup()
{
    Serial.begin(115200);
    mySerial.begin(9600); // WT901C-TTL baud rate
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW); // Ensure relay is off at startup

    // Initialize I2C sensors
    Wire.begin(D2, D1); // SDA=D2, SCL=D1
    if (!mlx.begin())
    {
        Serial.println(F("Error: MLX90614 not detected!"));
        while (1)
            ;
    }

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println(F("Connecting to WiFi..."));
    }
    Serial.println(F("WiFi connected"));
}

// Main loop function
void loop()
{
    float angle, acceleration;
    readWT901C(angle, acceleration);
    float tempF = readTemperature();

    Serial.print(F("Pitch: "));
    Serial.print(angle);
    Serial.print(F("°, Acceleration: "));
    Serial.print(acceleration);
    Serial.print(F("g, Temperature: "));
    Serial.print(tempF);
    Serial.println(F(" °F"));

    Serial.print(F("Free Heap Memory: "));
    Serial.println(ESP.getFreeHeap());

    // **Detect Electrical Pole Down**
    if (angle < tiltThreshold)
    {
        if (!relayActive)
        {
            Serial.println(F("Electrical pole tilt hazard detected! Shutting off power."));
            sendIFTTTMessage("Electrical pole tilted! Power shut off.");
            digitalWrite(relayPin, HIGH); // Shut off power
            relayActive = true;
        }
    }

    // **Detect Fire Hazard**
    if (tempF >= tempThresholdFire)
    {
        if (!relayActive)
        {
            Serial.println(F("Fire hazard detected! Shutting off power."));
            sendIFTTTMessage("Fire hazard detected! Power shut off.");
            digitalWrite(relayPin, HIGH);
            relayActive = true;
        }
    }

    // **Detect Broken Pole (Sudden Free-Fall or Impact)**
    if (angle < fallThreshold || acceleration > accelerationThreshold || acceleration < 0.2)
    {
        Serial.println(F("🚨 WARNING: Broken Pole Detected! Emergency Shutdown! 🚨"));
        sendIFTTTMessage("⚠️ Emergency! Pole broken or falling! Power shut off!");
        digitalWrite(relayPin, HIGH); // Shut off power
        relayActive = true;
    }

    // Reset relay if conditions normalize
    if (angle >= tiltThreshold && tempF < tempThresholdFire - 5 && acceleration > 0.5 && acceleration < accelerationThreshold)
    {
        if (relayActive)
        {
            Serial.println(F("Conditions normal. Restoring power."));
            sendIFTTTMessage("Conditions normal. Power restored.");
            digitalWrite(relayPin, LOW); // Restore power
            relayActive = false;
        }
    }

    delay(2000);
}
