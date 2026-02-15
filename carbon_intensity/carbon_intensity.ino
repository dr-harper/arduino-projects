#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <FastLED.h>

// Set web server port number to 80
WiFiServer server(80);

// Define the number of LEDs, data pin, and LED type
#define NUM_LEDS 10
#define DATA_PIN 4
#define LED_TYPE WS2812B

// Define an array to store the LED data
CRGB leds[NUM_LEDS];

// Variable to store the HTTP request
String header;

void setup()
{
  Serial.begin(115200);

  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Uncomment and run it once, if you want to erase all the stored information
  // wifiManager.resetSettings();

  // set custom ip for portal
  // wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  // Set your own AP name and password here
  wifiManager.autoConnect("CarbonLEDs", "your-password-here");
  // or use this for auto generated name ESP + ChipID
  // wifiManager.autoConnect();

  // if you get here you have connected to the WiFi
  Serial.println("Connected.");

  server.begin();
}

void loop()
{
}
