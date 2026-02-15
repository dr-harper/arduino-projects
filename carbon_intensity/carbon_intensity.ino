#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <FastLED.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Set web server port number to 80
WiFiServer server(80);

// Define the number of LEDs, data pin, and LED type
#define NUM_LEDS 35
#define DATA_PIN 2
#define LED_TYPE WS2812B
CRGB leds[NUM_LEDS]; // Define an array to store the LED data

// Variable to store the HTTP request
String header;
String jsonBuffer;

// HTTPS
const char*  server_name = "www.carbonintensity.org.uk";  // Server URL
const char* apiEndpoint = "https://api.carbonintensity.org.uk/intensity";

// Function to extract the carbon intensity index from the API response
const char*  extractIndex(String jsonString) {
  // Parse the JSON string into a JSON object
  DynamicJsonDocument jsonDoc(1024);
  deserializeJson(jsonDoc, jsonString);

  // Extract the "index" value from the "intensity" object in the "data" array
  const char*  index = jsonDoc["data"][0]["intensity"]["index"];

  return index;
}

// HTTP Request helper
StaticJsonDocument<1024> doc; // Change the size of the buffer as per your requirement

void setup() {
  Serial.begin(9600);
  Serial.println("Hello");

  // Initialise the FastLED library with the LED type and data pin
  FastLED.addLeds<LED_TYPE, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(40);

  // Flash the LEDs in purple to indicate that the device is not yet connected to WiFi
  Serial.println("Starting Up");
  flashLEDs(CRGB::Purple, 3);

  // WiFiManager — creates AP for configuration if no stored credentials
  Serial.println("Starting WiFi connection");
  WiFiManager wifiManager;

  // Set your own AP name and password here
  wifiManager.autoConnect("CarbonLEDs", "your-password-here");

  // If you get here you have connected to the WiFi
  Serial.println("Connected.");
  lightUpInOrderRainbow(2000); // Provide visual feedback of connection

  server.begin(); // Start to listen for clients
}



void loop() {
  Serial.println("Query API...");
  static unsigned long lastUpdate = 0;

  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currentTime = millis();

    if (currentTime - lastUpdate > 1) { // Update every 15 minutes
      delay(30000);

      Serial.println("Querying API");

      // Call API
      jsonBuffer = getCarbonIntensity();
      Serial.println(jsonBuffer);

      const char* intensityLabel = extractIndex(jsonBuffer);
      Serial.println(intensityLabel);
      CRGB color;

      if (strcmp(intensityLabel, "very low") == 0) {
        color = CRGB::Green;
      } else if (strcmp(intensityLabel, "low") == 0) {
        color = CRGB::Lime;
      } else if (strcmp(intensityLabel, "moderate") == 0) {
        color = CRGB::Yellow;
      } else if (strcmp(intensityLabel, "high") == 0) {
        color = CRGB::Orange;
      } else if (strcmp(intensityLabel, "very high") == 0) {
        color = CRGB::Red;
      }

      for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
      }

      FastLED.show();
    }
    lastUpdate = currentTime;

  }

  delay(60000); // Update every minute
}



// Function to flash the LEDs in a given colour a certain number of times
void flashLEDs(CRGB color, int numFlashes) {
  for (int i = 0; i < numFlashes; i++) {
    setColor(color);
    delay(500);
    setColor(CRGB::Black);
    delay(500);
  }
}

// Function to set all LEDs to a given colour
void setColor(CRGB color) {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void lightUpInOrder(uint32_t color, unsigned long duration) {
  unsigned long interval = duration / NUM_LEDS;
  for (int i = 0; i < NUM_LEDS; i++) {
    fill_solid(leds, i + 1, color);
    FastLED.show();
    delay(interval);
  }
}

void lightUpInOrderRainbow(unsigned long duration) {
  unsigned long interval = duration / NUM_LEDS;
  for (int i = 0; i < NUM_LEDS; i++) {
    CHSV rainbowColor = CHSV(i * (255 / NUM_LEDS), 255, 255);
    leds[i] = rainbowColor;
    FastLED.show();
    delay(interval);
  }
  delay(3000);
  FastLED.clearData();
  FastLED.show();
}

String getCarbonIntensity() {
  WiFiClientSecure client;
  HTTPClient https;
  client.setInsecure();

  Serial.println("\nStarting connection to server...");
  if (!client.connect(server_name, 443)) {
    Serial.println("Connection failed!");
  }

  // Load the API
  Serial.println("Connected to server!");
  client.println("GET https://api.carbonintensity.org.uk/intensity");
}
