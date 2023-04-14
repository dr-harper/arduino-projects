#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h> // Library for parsing JSON
#include <FastLED.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

const char*  server = "www.carbonintensity.org.uk";  // Server URL
const char* apiEndpoint = "https://api.carbonintensity.org.uk/intensity";

// Set up LEDs
// Define the number of LEDs, data pin, and LED type
#define NUM_LEDS 35
#define DATA_PIN 2
#define LED_TYPE WS2812B

#define INTERVAL_API_UPDATE 60000 * 20 // Define the interval for brightness adjustment in milliseconds
#define INTERVAL_LED_UPDATE 10000 // Define the interval for brightness adjustment in milliseconds

CRGB leds[NUM_LEDS]; // Define an array to store the LED data
unsigned long lastUpdateBrightness = 0; // Time of the last brightness update
unsigned long lastUpdateApi = 0; // Time of the last brightness update
bool isFirstCall = true;

void setup() {

  Serial.begin(9600);
  Serial.println("Starting Up");

  // Initialize the FastLED library with the LED type and data pin
  FastLED.addLeds<LED_TYPE, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(40);

  // Quickly show all the lights
  lightUpInOrder(CRGB::Red, 2000);
  lightUpInOrder(CRGB::Brown, 2000);
  lightUpInOrder(CRGB::DarkOrange, 2000);
  lightUpInOrder(CRGB::Gold, 2000);
  lightUpInOrder(CRGB::Green, 2000);

  // Hang on this if it doesn't connect ot Wifi
  lightUpInOrderRainbow(2000);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  Serial.println("Starting WiFi connection");
  WiFiManager wifiManager;
  
  // Uncomment and run it once, if you want to erase all the stored information
  //wifiManager.resetSettings();
  
  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP" and goes into a blocking loop awaiting configuration
  wifiManager.autoConnect("CarbonLEDs", "Michaelito");
  
  // if you get here you have connected to the WiFi
  Serial.println("Connected.");
  flashLEDs(CRGB::Purple, 3);
  

  // Show Purple as it calls the API
  lightUpInOrder(CRGB::Black, 2000);
}

void loop() { 
  // Check if it's time to update the LED brightness
  if (isFirstCall || millis() - lastUpdateApi >= INTERVAL_API_UPDATE) {
    WiFiClientSecure client;
    HTTPClient https;
    client.setInsecure();
    if (!client.connect(server, 443))
      Serial.println("Connection to server failed!");
    else {
      client.println("GET https://api.carbonintensity.org.uk/intensity");
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, client);
      // access the value of the "index" key and print it:
      const char* index = doc["data"][0]["intensity"]["index"];
      Serial.print("Carbon intensity index: ");
      Serial.println(index);

      Serial.print("Colour choice: ");
      CRGB color = getColorFromIndex(index);
      client.stop();

      // Update the time of the last brightness update
      lastUpdateApi = millis();

      if (isFirstCall) {
        isFirstCall = false;
      }
    }
  }

  // Add FastLED effects
  if (millis() - lastUpdateBrightness >= INTERVAL_LED_UPDATE) {
    // Update the time of the last brightness update
    lastUpdateBrightness = millis();
}
}


CRGB getColorFromIndex(const char* index) {
  if (strcmp(index, "very low") == 0) {
    Serial.println("Green");
    lightUpInOrder(CRGB::Green, 2000);
    return CRGB::Green;
  }
  if (strcmp(index, "low") == 0) {
     Serial.println("Lime");
     lightUpInOrder(CRGB::Lime, 2000);
    return CRGB::Lime;
  }
  if (strcmp(index, "moderate") == 0) {
    Serial.println("Yellow");
    lightUpInOrder(CRGB::Yellow, 2000);
    return CRGB::Yellow;
  }
  if (strcmp(index, "high") == 0) {
    Serial.println("Red");
    lightUpInOrder(CRGB::Red, 2000);
    return CRGB::Red;
  }
  if (strcmp(index, "very high") == 0) {
    Serial.println("Brown");
    lightUpInOrder(CRGB::Brown, 2000);
    return CRGB::Brown;
  }
  // default color
  Serial.println("Purple");
  return CRGB::Purple;
}

// Function to flash the LEDs in a given color a certain number of times
void flashLEDs(CRGB color, int numFlashes) {
  for (int i = 0; i < numFlashes; i++) {
    setColor(color);
    delay(500);
    setColor(CRGB::Black);
    delay(500);
  }
}

// Function to set all LEDs to a given color
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
}
