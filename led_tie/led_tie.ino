#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <string.h>

// -----------------------------
// Pins & config
// -----------------------------
#define LED_PIN D6    // LED strip data pin
#define BUTTON_PIN D7 // Button to GND
#define NUM_LEDS 70

#define ONBOARD_LED_PIN LED_BUILTIN // usually D4 on Wemos D1 mini
const bool USE_ONBOARD_LED = false; // set true if you want to use onboard LED

// WiFi AP config
#define MAX_SSID_LEN 32
#define MAX_PASS_LEN 32
const char *DEFAULT_AP_SSID = "Mikey";
const char *DEFAULT_AP_PASS = "hohoho123";
char wifiSSID[MAX_SSID_LEN];
char wifiPass[MAX_PASS_LEN];

// -----------------------------
// LED STRIP SETUP
// -----------------------------
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const int NUM_MODES = 14;
volatile int mode = 0;

// Button handling - interrupt-based for responsiveness
volatile bool buttonPressedISR = false;        // Set by interrupt
volatile unsigned long lastButtonISRTime = 0;  // For minimal debounce
const unsigned long MIN_BUTTON_INTERVAL = 150; // Minimum ms between presses
unsigned long buttonHoldStart = 0;             // For hold detection

// Brightness levels for double-tap cycling (value, percentage)
const uint8_t BRIGHTNESS_LEVELS[] = {25, 50, 100, 150, 200, 255};
const uint8_t BRIGHTNESS_PERCENT[] = {10, 20, 40, 60, 80, 100};
const int NUM_BRIGHTNESS_LEVELS = 6;
int currentBrightnessLevel = 3; // start at 150 (60%)

// Flirt mode detection
bool flirtModeActive = false;
const unsigned long FLIRT_HOLD_MS = 2000; // 2 seconds to activate
bool flirtHoldTriggered = false;          // prevents re-triggering while held

// Avoidant mode detection
bool avoidantModeActive = false;
const unsigned long AVOIDANT_HOLD_MS = 3000; // 3 seconds to activate

// -----------------------------
// CUSTOMISABLE PHRASES (EEPROM)
// -----------------------------
#define MAX_CUSTOM_PHRASES 8
#define MAX_PHRASE_LEN 40
#define EEPROM_SIZE 1536  // Need ~1349 bytes for phrases + WiFi
#define EEPROM_MAGIC 0xC9 // Magic byte - changed to force reload after layout change

// Default phrases
const char *DEFAULT_FLIRT[] = {
    "You're lovely",
    "Hello beautiful",
    "You caught my eye",
    "Fancy a drink?",
    "You're stunning",
    "Made me smile"};
const int NUM_DEFAULT_FLIRT = 6;

const char *DEFAULT_AVOIDANT[] = {
    "I'm married, sorry",
    "I'm a narcissist, avoid",
    "Heat pumps are my love",
    "I have strong opinions",
    "I'm emotionally unavailable",
    "Don't kiss me"};
const int NUM_DEFAULT_AVOIDANT = 6;

const char *DEFAULT_TAGLINES[] = {
    "Warning: May attract attention",
    "100% unnecessary",
    "Dress code: exceeded",
    "Compliments accepted",
    "More LEDs than sense"};
const int NUM_DEFAULT_TAGLINES = 5;

const char *DEFAULT_CHRISTMAS[] = {
    "Happy Christmas",
    "Ho Ho Ho",
    "More mulled wine please",
    "Let it Snow",
    "Santa runs on heat pumps",
    "Analytics say: be merry"};
const int NUM_DEFAULT_CHRISTMAS = 6;

// Runtime phrase storage
char flirtPhrases[MAX_CUSTOM_PHRASES][MAX_PHRASE_LEN];
int numFlirtPhrases = 0;

char avoidantPhrases[MAX_CUSTOM_PHRASES][MAX_PHRASE_LEN];
int numAvoidantPhrases = 0;

char taglinePhrases[MAX_CUSTOM_PHRASES][MAX_PHRASE_LEN];
int numTaglines = 0;

char christmasPhrases[MAX_CUSTOM_PHRASES][MAX_PHRASE_LEN];
int numChristmasPhrases = 0;

// LED animation state
uint16_t animIndex = 0;

// Auto mode progression
bool autoModeEnabled = true;
unsigned long lastModeChangeTime = 0;
const unsigned long AUTO_MODE_INTERVAL_MS = 600000; // 10 minutes

// -----------------------------
// OLED SETUP
// -----------------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Heat Geek logo bitmap (32x32 pixels) - exact conversion from pixel data
const unsigned char PROGMEM heatGeekLogo[] = {
    0x00, 0x00, 0x00, 0x00, // row 0
    0x00, 0x00, 0x00, 0x00, // row 1
    0x00, 0x00, 0x80, 0x00, // row 2:  col 16
    0x00, 0x00, 0xE0, 0x00, // row 3:  cols 16-18
    0x00, 0x01, 0xF0, 0x00, // row 4:  cols 15-19
    0x00, 0x01, 0xFE, 0x00, // row 5:  cols 15-22
    0x00, 0x01, 0xFE, 0x00, // row 6:  cols 15-22
    0x00, 0x01, 0xFF, 0x00, // row 7:  cols 15-23
    0x00, 0x03, 0xFE, 0x00, // row 8:  cols 14-23
    0x00, 0x03, 0xFE, 0x00, // row 9:  cols 14-23
    0x00, 0x07, 0xFE, 0x00, // row 10: cols 13-23
    0x00, 0x1F, 0xFE, 0x00, // row 11: cols 11-23
    0x00, 0x7F, 0xFE, 0x00, // row 12: cols 9-23
    0x00, 0x7F, 0xFE, 0x00, // row 13: cols 9-23
    0x00, 0xFF, 0xFE, 0x00, // row 14: cols 8-23
    0x01, 0xFF, 0xFE, 0x00, // row 15: cols 7-23
    0x03, 0xFF, 0xFE, 0x00, // row 16: cols 6-23
    0x07, 0xFF, 0xFC, 0x20, // row 17: cols 5-22, 26
    0x07, 0xFF, 0xF0, 0xE0, // row 18: cols 5-21, 24-26
    0x07, 0xFF, 0xF1, 0xE0, // row 19: cols 5-21, 23-26
    0x07, 0xFF, 0xFF, 0xE0, // row 20: cols 5-26
    0x07, 0xC1, 0xF0, 0x7E, // row 21: cols 5-10, 13-19, 25-30 (glasses holes)
    0x07, 0x00, 0x60, 0x3E, // row 22: cols 5-7, 17-18, 26-30
    0x03, 0x00, 0x60, 0x18, // row 23: cols 6-7, 17-18, 27-28
    0x00, 0x38, 0x0E, 0x00, // row 24: cols 10-12, 19-21 (bottom glasses)
    0x00, 0x10, 0x04, 0x00, // row 25: cols 11, 21
    0x00, 0x00, 0xC0, 0x00, // row 26: cols 15-16
    0x00, 0x01, 0xC0, 0x00, // row 27: cols 14-16
    0x00, 0x0F, 0xFC, 0x00, // row 28: cols 12-21
    0x00, 0x03, 0xC0, 0x00, // row 29: cols 14-17
    0x00, 0x00, 0x00, 0x00, // row 30
    0x00, 0x00, 0x00, 0x00  // row 31
};
#define HEATGEEK_LOGO_WIDTH 32
#define HEATGEEK_LOGO_HEIGHT 32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
bool displayOK = false;

// Display timing
unsigned long lastDisplayFrame = 0;
unsigned long modeChangeTime = 0;
const unsigned long DISPLAY_FRAME_INTERVAL = 70;    // ~14 FPS
const unsigned long DISPLAY_SPLASH_DURATION = 1000; // 1 second splash

// -----------------------------
// Config model (runtime)
// -----------------------------
struct TieConfig
{
    uint8_t ledBrightness;         // 0–255
    unsigned long idleTimeoutMs;   // ms until idle screen
    unsigned long messageRotateMs; // ms between messages

    static const int MAX_PHRASES = 8;
    String phrases[MAX_PHRASES];
    int phraseCount;
};

TieConfig cfg;
unsigned long lastInteractionTime = 0;                  // updated on button press
unsigned long displayRotationStart = 0;                 // reset on mode change for animation/message rotation
unsigned long brightnessDisplayUntil = 0;               // timestamp when brightness overlay should end
const unsigned long BRIGHTNESS_DISPLAY_DURATION = 2000; // show brightness for 2 seconds

// -----------------------------
// Web server & DNS (captive portal)
// -----------------------------
ESP8266WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

// Debug log buffer for web UI
#define LOG_SIZE 10
#define LOG_LINE_LEN 40
char logBuffer[LOG_SIZE][LOG_LINE_LEN];
int logIndex = 0;

void webLog(const char *msg)
{
    strncpy(logBuffer[logIndex], msg, LOG_LINE_LEN - 1);
    logBuffer[logIndex][LOG_LINE_LEN - 1] = '\0';
    logIndex = (logIndex + 1) % LOG_SIZE;
    Serial.println(msg);
}

void webLogInt(const char *prefix, int val)
{
    char buf[LOG_LINE_LEN];
    snprintf(buf, LOG_LINE_LEN, "%s%d", prefix, val);
    webLog(buf);
}

// Forward declarations
ICACHE_RAM_ATTR void buttonISR();
void readButton();
void displayInit();
void displayShowMode(int m);
void displayUpdate(unsigned long now);
const char *modeName(int m);

void modeRedPulse(unsigned long now);
void modeGreenPulse(unsigned long now);
void modeCandyCane(unsigned long now);
void modeTwinkle(unsigned long now);
void modeWarmGradient(unsigned long now);
void modeRainbow(unsigned long now);
void modeMeteorShower(unsigned long now);
void modeFire(unsigned long now);
void modeSparkleBurst(unsigned long now);
void modeIceShimmer(unsigned long now);
void modePlasmaWave(unsigned long now);
void modeNorthernLights(unsigned long now);
void modeCylonScanner(unsigned long now);
void modeHeatGeek(unsigned long now);
float getCylonPosition(unsigned long now);
float getSparkleBurstPhase(unsigned long now, bool *isBurst);
void modeFlirt(unsigned long now);
void modeAvoidant(unsigned long now);

uint32_t wheel(byte pos);

void initDefaultConfig();
void setupWiFiAP();
void setupWebServer();
void handleRoot();
void handleSave();
void handleCaptivePortal();
void displayShowBrightness();
void playStartupAnimation();

// -----------------------------
// EEPROM PHRASE STORAGE
// -----------------------------
// EEPROM Layout:
// Byte 0: Magic byte (0xC9)
// Byte 1: numFlirtPhrases
// Byte 2: numAvoidantPhrases
// Byte 3: numTaglines
// Byte 4: numChristmasPhrases
// Bytes 5-324: Flirt phrases (8 x 40 bytes)
// Bytes 325-644: Avoidant phrases (8 x 40 bytes)
// Bytes 645-964: Tagline phrases (8 x 40 bytes)
// Bytes 965-1284: Christmas phrases (8 x 40 bytes)
// Bytes 1285-1316: WiFi SSID (32 bytes)
// Bytes 1317-1348: WiFi Password (32 bytes)

#define EEPROM_ADDR_MAGIC 0
#define EEPROM_ADDR_NUM_FLIRT 1
#define EEPROM_ADDR_NUM_AVOIDANT 2
#define EEPROM_ADDR_NUM_TAGLINES 3
#define EEPROM_ADDR_NUM_CHRISTMAS 4
#define EEPROM_ADDR_FLIRT 5
#define EEPROM_ADDR_AVOIDANT (EEPROM_ADDR_FLIRT + MAX_CUSTOM_PHRASES * MAX_PHRASE_LEN)
#define EEPROM_ADDR_TAGLINES (EEPROM_ADDR_AVOIDANT + MAX_CUSTOM_PHRASES * MAX_PHRASE_LEN)
#define EEPROM_ADDR_CHRISTMAS (EEPROM_ADDR_TAGLINES + MAX_CUSTOM_PHRASES * MAX_PHRASE_LEN)
#define EEPROM_ADDR_WIFI_SSID (EEPROM_ADDR_CHRISTMAS + MAX_CUSTOM_PHRASES * MAX_PHRASE_LEN)
#define EEPROM_ADDR_WIFI_PASS (EEPROM_ADDR_WIFI_SSID + MAX_SSID_LEN)

void loadPhrasesFromDefaults()
{
    // Load flirt defaults
    numFlirtPhrases = NUM_DEFAULT_FLIRT;
    for (int i = 0; i < NUM_DEFAULT_FLIRT; i++)
    {
        strncpy(flirtPhrases[i], DEFAULT_FLIRT[i], MAX_PHRASE_LEN - 1);
        flirtPhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Load avoidant defaults
    numAvoidantPhrases = NUM_DEFAULT_AVOIDANT;
    for (int i = 0; i < NUM_DEFAULT_AVOIDANT; i++)
    {
        strncpy(avoidantPhrases[i], DEFAULT_AVOIDANT[i], MAX_PHRASE_LEN - 1);
        avoidantPhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Load tagline defaults
    numTaglines = NUM_DEFAULT_TAGLINES;
    for (int i = 0; i < NUM_DEFAULT_TAGLINES; i++)
    {
        strncpy(taglinePhrases[i], DEFAULT_TAGLINES[i], MAX_PHRASE_LEN - 1);
        taglinePhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Load Christmas defaults
    numChristmasPhrases = NUM_DEFAULT_CHRISTMAS;
    for (int i = 0; i < NUM_DEFAULT_CHRISTMAS; i++)
    {
        strncpy(christmasPhrases[i], DEFAULT_CHRISTMAS[i], MAX_PHRASE_LEN - 1);
        christmasPhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Load WiFi defaults
    strncpy(wifiSSID, DEFAULT_AP_SSID, MAX_SSID_LEN - 1);
    wifiSSID[MAX_SSID_LEN - 1] = '\0';
    strncpy(wifiPass, DEFAULT_AP_PASS, MAX_PASS_LEN - 1);
    wifiPass[MAX_PASS_LEN - 1] = '\0';
}

void loadPhrasesFromEEPROM()
{
    EEPROM.begin(EEPROM_SIZE);

    // Check magic byte
    if (EEPROM.read(EEPROM_ADDR_MAGIC) != EEPROM_MAGIC)
    {
        Serial.println(F("No saved phrases found, using defaults"));
        loadPhrasesFromDefaults();
        EEPROM.end();
        return;
    }

    // Read counts
    numFlirtPhrases = EEPROM.read(EEPROM_ADDR_NUM_FLIRT);
    numAvoidantPhrases = EEPROM.read(EEPROM_ADDR_NUM_AVOIDANT);
    numTaglines = EEPROM.read(EEPROM_ADDR_NUM_TAGLINES);
    numChristmasPhrases = EEPROM.read(EEPROM_ADDR_NUM_CHRISTMAS);

    // Validate counts
    if (numFlirtPhrases > MAX_CUSTOM_PHRASES)
        numFlirtPhrases = MAX_CUSTOM_PHRASES;
    if (numAvoidantPhrases > MAX_CUSTOM_PHRASES)
        numAvoidantPhrases = MAX_CUSTOM_PHRASES;
    if (numTaglines > MAX_CUSTOM_PHRASES)
        numTaglines = MAX_CUSTOM_PHRASES;
    if (numChristmasPhrases > MAX_CUSTOM_PHRASES)
        numChristmasPhrases = MAX_CUSTOM_PHRASES;

    // Read flirt phrases
    for (int i = 0; i < numFlirtPhrases; i++)
    {
        int addr = EEPROM_ADDR_FLIRT + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            flirtPhrases[i][j] = EEPROM.read(addr + j);
        }
        flirtPhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Read avoidant phrases
    for (int i = 0; i < numAvoidantPhrases; i++)
    {
        int addr = EEPROM_ADDR_AVOIDANT + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            avoidantPhrases[i][j] = EEPROM.read(addr + j);
        }
        avoidantPhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Read tagline phrases
    for (int i = 0; i < numTaglines; i++)
    {
        int addr = EEPROM_ADDR_TAGLINES + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            taglinePhrases[i][j] = EEPROM.read(addr + j);
        }
        taglinePhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Read Christmas phrases
    for (int i = 0; i < numChristmasPhrases; i++)
    {
        int addr = EEPROM_ADDR_CHRISTMAS + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            christmasPhrases[i][j] = EEPROM.read(addr + j);
        }
        christmasPhrases[i][MAX_PHRASE_LEN - 1] = '\0';
    }

    // Read WiFi credentials
    for (int i = 0; i < MAX_SSID_LEN; i++)
    {
        wifiSSID[i] = EEPROM.read(EEPROM_ADDR_WIFI_SSID + i);
    }
    wifiSSID[MAX_SSID_LEN - 1] = '\0';

    for (int i = 0; i < MAX_PASS_LEN; i++)
    {
        wifiPass[i] = EEPROM.read(EEPROM_ADDR_WIFI_PASS + i);
    }
    wifiPass[MAX_PASS_LEN - 1] = '\0';

    // Validate WiFi - if empty, use defaults
    if (wifiSSID[0] == '\0' || wifiSSID[0] == 0xFF)
    {
        strncpy(wifiSSID, DEFAULT_AP_SSID, MAX_SSID_LEN - 1);
        strncpy(wifiPass, DEFAULT_AP_PASS, MAX_PASS_LEN - 1);
    }

    EEPROM.end();
    Serial.println(F("Loaded phrases from EEPROM"));
}

void savePhrasesToEEPROM()
{
    EEPROM.begin(EEPROM_SIZE);

    // Write magic byte
    EEPROM.write(EEPROM_ADDR_MAGIC, EEPROM_MAGIC);

    // Write counts
    EEPROM.write(EEPROM_ADDR_NUM_FLIRT, numFlirtPhrases);
    EEPROM.write(EEPROM_ADDR_NUM_AVOIDANT, numAvoidantPhrases);
    EEPROM.write(EEPROM_ADDR_NUM_TAGLINES, numTaglines);
    EEPROM.write(EEPROM_ADDR_NUM_CHRISTMAS, numChristmasPhrases);

    // Write flirt phrases
    for (int i = 0; i < numFlirtPhrases; i++)
    {
        int addr = EEPROM_ADDR_FLIRT + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            EEPROM.write(addr + j, flirtPhrases[i][j]);
        }
    }

    // Write avoidant phrases
    for (int i = 0; i < numAvoidantPhrases; i++)
    {
        int addr = EEPROM_ADDR_AVOIDANT + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            EEPROM.write(addr + j, avoidantPhrases[i][j]);
        }
    }

    // Write tagline phrases
    for (int i = 0; i < numTaglines; i++)
    {
        int addr = EEPROM_ADDR_TAGLINES + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            EEPROM.write(addr + j, taglinePhrases[i][j]);
        }
    }

    // Write Christmas phrases
    for (int i = 0; i < numChristmasPhrases; i++)
    {
        int addr = EEPROM_ADDR_CHRISTMAS + i * MAX_PHRASE_LEN;
        for (int j = 0; j < MAX_PHRASE_LEN; j++)
        {
            EEPROM.write(addr + j, christmasPhrases[i][j]);
        }
    }

    // Write WiFi credentials
    for (int i = 0; i < MAX_SSID_LEN; i++)
    {
        EEPROM.write(EEPROM_ADDR_WIFI_SSID + i, wifiSSID[i]);
    }
    for (int i = 0; i < MAX_PASS_LEN; i++)
    {
        EEPROM.write(EEPROM_ADDR_WIFI_PASS + i, wifiPass[i]);
    }

    EEPROM.commit();
    EEPROM.end();
    Serial.println(F("Saved phrases to EEPROM"));
}

void resetPhrasesToDefaults()
{
    loadPhrasesFromDefaults();
    savePhrasesToEEPROM();
    Serial.println(F("Reset phrases to defaults"));
}

// -----------------------------
// SETUP
// -----------------------------
void setup()
{
    // Serial for debugging
    Serial.begin(115200);
    Serial.println();
    Serial.println(F("Christmas Tie starting..."));

    // Onboard LED config
    pinMode(ONBOARD_LED_PIN, OUTPUT);
    if (USE_ONBOARD_LED)
    {
        digitalWrite(ONBOARD_LED_PIN, LOW); // ON (active LOW)
    }
    else
    {
        digitalWrite(ONBOARD_LED_PIN, HIGH); // OFF
    }

    // Config defaults
    initDefaultConfig();

    // Load custom phrases from EEPROM
    loadPhrasesFromEEPROM();

    // LEDs
    strip.begin();
    strip.setBrightness(cfg.ledBrightness);
    strip.show();

    // Button with interrupt for responsiveness
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonISR, FALLING);

    // Random for twinkle
    randomSeed(analogRead(A0));

    // OLED
    displayInit();

    // Play startup animation
    playStartupAnimation();

    // Clear any interrupt flags from startup button press
    buttonPressedISR = false;

    // Show initial mode
    displayShowMode(mode);

    // Start interaction timer
    lastInteractionTime = millis();
    lastModeChangeTime = millis(); // start auto progression timer

    // WiFi AP + Web server
    setupWiFiAP();
    setupWebServer();
}

// -----------------------------
// MAIN LOOP
// -----------------------------
void loop()
{
    // Allow ESP8266 background tasks to run (prevents watchdog reset)
    yield();

    readButton();
    unsigned long now = millis();

    // Handle DNS requests (captive portal)
    dnsServer.processNextRequest();

    // Handle HTTP requests
    server.handleClient();

    // LED patterns - special modes override normal modes
    if (flirtModeActive)
    {
        modeFlirt(now);
    }
    else if (avoidantModeActive)
    {
        modeAvoidant(now);
    }
    else
    {
        switch (mode)
        {
        case 0:
            modeRedPulse(now);
            break;
        case 1:
            modeGreenPulse(now);
            break;
        case 2:
            modeCandyCane(now);
            break;
        case 3:
            modeTwinkle(now);
            break;
        case 4:
            modeWarmGradient(now);
            break;
        case 5:
            modeRainbow(now);
            break;
        case 6:
            modeMeteorShower(now);
            break;
        case 7:
            modeFire(now);
            break;
        case 8:
            modeSparkleBurst(now);
            break;
        case 9:
            modeIceShimmer(now);
            break;
        case 10:
            modePlasmaWave(now);
            break;
        case 11:
            modeNorthernLights(now);
            break;
        case 12:
            modeCylonScanner(now);
            break;
        case 13:
            modeHeatGeek(now);
            break;
        }
    }

    // Auto mode progression (only when not in special modes)
    if (autoModeEnabled && !flirtModeActive && !avoidantModeActive)
    {
        if (now - lastModeChangeTime >= AUTO_MODE_INTERVAL_MS)
        {
            mode = (mode + 1) % NUM_MODES;
            animIndex = 0;
            displayShowMode(mode);
            lastModeChangeTime = now;
            displayRotationStart = now;
        }
    }

    // OLED animation / idle messages
    displayUpdate(now);
}

// ========================
// CONFIG
// ========================
void initDefaultConfig()
{
    cfg.ledBrightness = 140;
    cfg.idleTimeoutMs = 10000;   // 10s
    cfg.messageRotateMs = 10000; // 10s

    cfg.phraseCount = 6;
    cfg.phrases[0] = "Happy Christmas";
    cfg.phrases[1] = "Ho Ho Ho";
    cfg.phrases[2] = "More mulled wine please";
    cfg.phrases[3] = "Let it Snow";
    cfg.phrases[4] = "Santa runs on heat pumps";
    cfg.phrases[5] = "Analytics say: be merry";
}

// ========================
// WIFI + WEB SERVER
// ========================
void setupWiFiAP()
{
    // Disable WiFi sleep to prevent disconnects
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    // Disconnect from any previous connection
    WiFi.disconnect();
    delay(100);

    // Set mode and configure AP
    WiFi.mode(WIFI_AP);

    // Configure AP with specific settings for stability
    IPAddress local_IP(192, 168, 4, 1);
    IPAddress gateway(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);

    WiFi.softAP(wifiSSID, wifiPass, 1, false, 4); // channel 1, not hidden, max 4 clients

    Serial.print(F("AP started: "));
    Serial.println(wifiSSID);
    Serial.print(F("IP address: "));
    Serial.println(WiFi.softAPIP());

    // Start DNS server for captive portal - redirect all domains to our IP
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.println(F("DNS server started"));
}

void handleSpecialMode();
void handleLedMode();
void handleBrightness();
void handleAutoMode();
void handleLogs();
void handlePhrases();
void handleSavePhrases();
void handleResetPhrases();
void handleSaveWifi();

void setupWebServer()
{
    server.on("/", HTTP_GET, handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/special", HTTP_GET, handleSpecialMode);
    server.on("/led", HTTP_GET, handleLedMode);
    server.on("/brightness", HTTP_GET, handleBrightness);
    server.on("/auto", HTTP_GET, handleAutoMode);
    server.on("/logs", HTTP_GET, handleLogs);
    server.on("/phrases", HTTP_GET, handlePhrases);
    server.on("/savephrases", HTTP_POST, handleSavePhrases);
    server.on("/resetphrases", HTTP_GET, handleResetPhrases);
    server.on("/savewifi", HTTP_POST, handleSaveWifi);

    // Captive portal detection endpoints - these trigger the "sign in" popup
    server.on("/generate_204", HTTP_GET, handleCaptivePortal);              // Android
    server.on("/gen_204", HTTP_GET, handleCaptivePortal);                   // Android
    server.on("/hotspot-detect.html", HTTP_GET, handleCaptivePortal);       // Apple
    server.on("/library/test/success.html", HTTP_GET, handleCaptivePortal); // Apple
    server.on("/ncsi.txt", HTTP_GET, handleCaptivePortal);                  // Windows
    server.on("/connecttest.txt", HTTP_GET, handleCaptivePortal);           // Windows
    server.on("/fwlink", HTTP_GET, handleCaptivePortal);                    // Windows

    // Catch-all for any other requests - redirect to root
    server.onNotFound(handleCaptivePortal);

    server.begin();
}

// Simple HTML page
void handleRoot()
{
    String page;
    page.reserve(4000);

    page += F("<!DOCTYPE html><html><head><meta charset='utf-8'><title>Christmas Tie</title>");
    page += F("<link rel='icon' href='data:image/svg+xml,<svg xmlns=%22http://www.w3.org/2000/svg%22 viewBox=%220 0 100 100%22><text y=%22.9em%22 font-size=%2290%22>🎄</text></svg>'>");
    page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<style>\
*{box-sizing:border-box}\
body{font-family:-apple-system,sans-serif;background:#0a0a0a;color:#eee;padding:12px;margin:0;}\
.card{background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:16px;margin-bottom:12px;}\
h1{font-size:1.5rem;margin:0 0 12px 0;text-align:center;}\
h2{font-size:1rem;color:#888;margin:0 0 12px 0;border-bottom:1px solid #333;padding-bottom:8px;}\
label{display:block;margin:12px 0 6px;color:#ccc;font-size:0.9rem;}\
input[type=number],textarea{width:100%;padding:10px;background:#252525;border:1px solid #444;border-radius:6px;color:#fff;font-size:1rem;}\
input[type=number]:focus,textarea:focus{outline:none;border-color:#c41e3a;}\
input[type=range]{width:100%;margin:8px 0;}\
.modes{display:grid;grid-template-columns:repeat(2,1fr);gap:8px;}\
.mbtn{padding:12px 8px;border:2px solid #444;border-radius:8px;background:#252525;color:#fff;font-size:0.85rem;cursor:pointer;transition:all 0.2s;}\
.mbtn:hover{background:#333;}\
.mbtn.active{border-color:#c41e3a;background:#2a1a1a;}\
.special{display:flex;gap:8px;flex-wrap:wrap;}\
.sbtn{flex:1;min-width:90px;padding:12px;border:none;border-radius:8px;color:#fff;font-size:0.9rem;cursor:pointer;}\
.sbtn.flirt{background:#d63384;}.sbtn.avoid{background:#dc3545;}.sbtn.exit{background:#198754;}\
.save{width:100%;padding:14px;background:#c41e3a;border:none;border-radius:8px;color:#fff;font-size:1rem;cursor:pointer;margin-top:8px;}\
.save:hover{background:#a01830;}\
.status{text-align:center;color:#888;font-size:0.85rem;margin-top:8px;}\
.status span{font-weight:bold;}\
.chk{display:flex;align-items:center;gap:8px;margin:12px 0;}\
.chk input{width:18px;height:18px;}\
#toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#198754;color:#fff;\
padding:12px 24px;border-radius:8px;opacity:0;transition:opacity 0.3s;pointer-events:none;font-size:0.9rem;}\
</style></head><body>");

    // Header
    page += F("<h1>🎄 Christmas Tie</h1>");

    // LED Modes card
    page += F("<div class='card'><h2>LED Pattern</h2><div class='modes'>");
    for (int m = 0; m < NUM_MODES; m++)
    {
        page += F("<button class='mbtn");
        if (m == mode && !flirtModeActive && !avoidantModeActive)
            page += F(" active");
        page += F("' data-m='");
        page += String(m);
        page += F("' onclick='setLed(");
        page += String(m);
        page += F(")'>");
        page += modeName(m);
        page += F("</button>");
    }
    page += F("</div>");

    // Auto-cycle checkbox
    page += F("<div class='chk'><input type='checkbox' id='auto'");
    if (autoModeEnabled)
        page += F(" checked");
    page += F(" onchange='setAuto(this.checked)'><label for='auto' style='margin:0'>Auto-cycle (10 min)</label></div>");
    page += F("</div>");

    // Special modes card
    page += F("<div class='card'><h2>Special Modes</h2>");
    page += F("<select id='specialMode' onchange='setSpecial(this.value)' style='width:100%;padding:12px;background:#252525;border:1px solid #444;border-radius:8px;color:#fff;font-size:1rem;'>");
    page += F("<option value='normal'");
    if (!flirtModeActive && !avoidantModeActive)
        page += F(" selected");
    page += F(">✓ Normal</option>");
    page += F("<option value='flirt'");
    if (flirtModeActive)
        page += F(" selected");
    page += F(">💕 Flirt Mode</option>");
    page += F("<option value='avoidant'");
    if (avoidantModeActive)
        page += F(" selected");
    page += F(">⚠️ Avoidant Mode</option>");
    page += F("</select></div>");

    // Brightness card
    page += F("<div class='card'><h2>Brightness</h2>");
    page += F("<label>Level: <span id='brVal'>");
    page += String((int)cfg.ledBrightness);
    page += F("</span></label>");
    page += F("<input type='range' id='brSlider' min='0' max='255' value='");
    page += String((int)cfg.ledBrightness);
    page += F("' oninput=\"document.getElementById('brVal').textContent=this.value\" onchange='setBr(this.value)'>");
    page += F("</div>");

    // Settings card (form)
    page += F("<div class='card'><h2>Display Settings</h2>");
    page += F("<form method='POST' action='/save'>");

    page += F("<label>Idle timeout (seconds)</label>");
    page += F("<input type='number' name='idle_timeout_s' min='1' max='300' value='");
    page += String(cfg.idleTimeoutMs / 1000);
    page += F("'>");

    page += F("<label>Message rotation (seconds)</label>");
    page += F("<input type='number' name='rotate_s' min='1' max='300' value='");
    page += String(cfg.messageRotateMs / 1000);
    page += F("'>");

    page += F("<button type='submit' class='save'>Save Display Settings</button>");
    page += F("</form></div>");

    // Custom phrases link
    page += F("<div class='card'><h2>Custom Phrases</h2>");
    page += F("<p style='color:#888;font-size:0.9rem;margin:0 0 12px 0;'>Edit Christmas, flirt, avoidant &amp; startup phrases</p>");
    page += F("<a href='/phrases' style='display:block;padding:14px;background:#6c5ce7;border:none;border-radius:8px;color:#fff;font-size:1rem;text-align:center;text-decoration:none;'>Edit All Phrases</a>");
    page += F("</div>");

    // Debug log card
    page += F("<div class='card'><h2>Debug Log</h2>");
    page += F("<pre id='logs' style='background:#000;padding:10px;border-radius:6px;font-size:0.8rem;overflow-x:auto;max-height:150px;margin:0;'>Loading...</pre>");
    page += F("<button onclick='fetchLogs()' style='margin-top:8px;padding:8px 16px;background:#444;border:none;border-radius:6px;color:#fff;cursor:pointer;'>Refresh Logs</button>");
    page += F("</div>");

    // Toast
    page += F("<div id='toast'>Saved!</div>");

    // JavaScript
    page += F("<script>\
function toast(msg,ok){var t=document.getElementById('toast');t.textContent=msg;t.style.background=ok?'#198754':'#dc3545';t.style.opacity=1;setTimeout(function(){t.style.opacity=0;},1500);}\
function setLed(m){fetch('/led?m='+m).then(r=>{if(r.ok){document.querySelectorAll('.mbtn').forEach(b=>b.classList.toggle('active',b.dataset.m==m));toast('Mode changed',true);fetchLogs();}});}\
function setSpecial(m){fetch('/special?mode='+m).then(r=>r.ok?toast(m=='normal'?'Normal mode':'Mode activated',true):toast('Error',false));}\
function setBr(v){fetch('/brightness?v='+v).then(r=>r.ok?toast('Brightness set',true):toast('Error',false));}\
function setAuto(on){fetch('/auto?on='+(on?1:0)).then(r=>r.ok?toast(on?'Auto-cycle on':'Auto-cycle off',true):toast('Error',false));}\
document.querySelector('form').onsubmit=function(e){e.preventDefault();fetch('/save',{method:'POST',body:new FormData(this)}).then(r=>toast(r.ok?'Saved!':'Error',r.ok)).catch(()=>toast('Error',false));};\
function fetchLogs(){fetch('/logs').then(r=>r.text()).then(t=>{document.getElementById('logs').textContent=t||'(empty)';});}\
fetchLogs();\
</script>");

    page += F("</body></html>");

    server.send(200, "text/html", page);
}

// Captive portal handler - redirects to main page
void handleCaptivePortal()
{
    Serial.print(F("Captive portal request: "));
    Serial.println(server.uri());

    // Use a static buffer to avoid String allocations
    static char redirectUrl[32];
    IPAddress ip = WiFi.softAPIP();
    snprintf(redirectUrl, sizeof(redirectUrl), "http://%d.%d.%d.%d/", ip[0], ip[1], ip[2], ip[3]);

    server.sendHeader("Location", redirectUrl, true);
    server.send(302, "text/plain", "");
}

void handleSave()
{
    // Brightness
    if (server.hasArg("brightness"))
    {
        int b = server.arg("brightness").toInt();
        if (b < 0)
            b = 0;
        if (b > 255)
            b = 255;
        cfg.ledBrightness = (uint8_t)b;
        strip.setBrightness(cfg.ledBrightness);
        strip.show();
    }

    // Idle timeout
    if (server.hasArg("idle_timeout_s"))
    {
        long s = server.arg("idle_timeout_s").toInt();
        if (s < 1)
            s = 1;
        if (s > 300)
            s = 300;
        cfg.idleTimeoutMs = (unsigned long)s * 1000UL;
    }

    // Message rotation
    if (server.hasArg("rotate_s"))
    {
        long s = server.arg("rotate_s").toInt();
        if (s < 1)
            s = 1;
        if (s > 300)
            s = 300;
        cfg.messageRotateMs = (unsigned long)s * 1000UL;
    }

    // LED Mode selection
    if (server.hasArg("led_mode"))
    {
        int newMode = server.arg("led_mode").toInt();
        if (newMode >= 0 && newMode < NUM_MODES)
        {
            mode = newMode;
            animIndex = 0;
            displayShowMode(mode);
            displayRotationStart = millis();
            lastModeChangeTime = millis(); // reset auto progression timer

            // Exit any special modes when manually selecting
            flirtModeActive = false;
            avoidantModeActive = false;
        }
    }

    // Auto mode toggle (checkbox - only sent if checked)
    autoModeEnabled = server.hasArg("auto_mode");
    if (autoModeEnabled)
    {
        lastModeChangeTime = millis(); // reset timer when enabled
    }

    // Simple OK response for AJAX
    server.send(200, "text/plain", "OK");
}

// Handle special mode activation from web UI
void handleSpecialMode()
{
    if (server.hasArg("mode"))
    {
        String modeArg = server.arg("mode");
        unsigned long now = millis();

        if (modeArg == "flirt")
        {
            webLog("Special: flirt");
            flirtModeActive = true;
            avoidantModeActive = false;
            lastInteractionTime = now;
            displayRotationStart = now;
        }
        else if (modeArg == "avoidant")
        {
            webLog("Special: avoidant");
            flirtModeActive = false;
            avoidantModeActive = true;
            lastInteractionTime = now;
            displayRotationStart = now;
        }
        else if (modeArg == "normal")
        {
            webLog("Special: normal");
            flirtModeActive = false;
            avoidantModeActive = false;
            displayShowMode(mode);
            lastInteractionTime = now;
            displayRotationStart = now;
        }
    }

    server.send(200, "text/plain", "OK");
}

// Handle instant LED mode change from web UI
void handleLedMode()
{
    if (server.hasArg("m"))
    {
        int newMode = server.arg("m").toInt();
        webLogInt("Req mode: ", newMode);

        // Fix off-by-one: something increments mode after we set it
        newMode = (newMode - 1 + NUM_MODES) % NUM_MODES;

        if (newMode >= 0 && newMode < NUM_MODES)
        {
            // Fade out LEDs first to prevent voltage spike on mode change
            uint8_t oldBrightness = cfg.ledBrightness;
            for (int b = oldBrightness; b >= 0; b -= 20)
            {
                strip.setBrightness(b);
                strip.show();
                delay(10);
            }
            strip.clear();
            strip.show();

            mode = newMode;
            animIndex = 0;
            flirtModeActive = false;
            avoidantModeActive = false;
            displayShowMode(mode);
            displayRotationStart = millis();
            lastModeChangeTime = millis();
            lastInteractionTime = millis();

            // Restore brightness
            strip.setBrightness(oldBrightness);

            webLogInt("Set mode: ", mode);
        }
    }
    server.send(200, "text/plain", "OK");
}

// Handle instant brightness change from web UI
void handleBrightness()
{
    if (server.hasArg("v"))
    {
        int b = server.arg("v").toInt();
        if (b < 0)
            b = 0;
        if (b > 255)
            b = 255;
        cfg.ledBrightness = (uint8_t)b;
        strip.setBrightness(cfg.ledBrightness);
        strip.show();
        webLogInt("Brightness: ", b);
    }
    server.send(200, "text/plain", "OK");
}

// Handle auto-cycle toggle from web UI
void handleAutoMode()
{
    if (server.hasArg("on"))
    {
        autoModeEnabled = (server.arg("on") == "1");
        if (autoModeEnabled)
        {
            webLog("Auto-cycle: ON");
            lastModeChangeTime = millis();
        }
        else
        {
            webLog("Auto-cycle: OFF");
        }
    }
    server.send(200, "text/plain", "OK");
}

// Return logs as plain text
void handleLogs()
{
    String logs;
    logs.reserve(LOG_SIZE * LOG_LINE_LEN);

    // Output logs in order (oldest first)
    for (int i = 0; i < LOG_SIZE; i++)
    {
        int idx = (logIndex + i) % LOG_SIZE;
        if (logBuffer[idx][0] != '\0')
        {
            logs += logBuffer[idx];
            logs += "\n";
        }
    }

    server.send(200, "text/plain", logs);
}

// Phrases editing page
void handlePhrases()
{
    String page;
    page.reserve(5000);

    page += F("<!DOCTYPE html><html><head><meta charset='utf-8'><title>Edit Phrases</title>");
    page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
    page += F("<style>\
*{box-sizing:border-box}\
body{font-family:-apple-system,sans-serif;background:#0a0a0a;color:#eee;padding:12px;margin:0;}\
.card{background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:16px;margin-bottom:12px;}\
h1{font-size:1.5rem;margin:0 0 12px 0;text-align:center;}\
h2{font-size:1rem;color:#888;margin:0 0 12px 0;border-bottom:1px solid #333;padding-bottom:8px;}\
label{display:block;margin:0 0 6px;color:#ccc;font-size:0.9rem;}\
textarea{width:100%;padding:10px;background:#252525;border:1px solid #444;border-radius:6px;color:#fff;font-size:0.95rem;font-family:inherit;resize:vertical;}\
textarea:focus{outline:none;border-color:#6c5ce7;}\
.hint{color:#666;font-size:0.8rem;margin:4px 0 12px 0;}\
.save{width:100%;padding:14px;background:#6c5ce7;border:none;border-radius:8px;color:#fff;font-size:1rem;cursor:pointer;margin-top:8px;}\
.save:hover{background:#5b4cdb;}\
.reset{width:100%;padding:12px;background:#dc3545;border:none;border-radius:8px;color:#fff;font-size:0.9rem;cursor:pointer;margin-top:8px;}\
.reset:hover{background:#c82333;}\
.back{display:inline-block;padding:8px 16px;background:#444;border-radius:6px;color:#fff;text-decoration:none;margin-bottom:12px;}\
#toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#198754;color:#fff;\
padding:12px 24px;border-radius:8px;opacity:0;transition:opacity 0.3s;pointer-events:none;font-size:0.9rem;}\
</style></head><body>");

    page += F("<a href='/' class='back'>&larr; Back</a>");
    page += F("<h1>Edit Phrases</h1>");

    page += F("<form id='phraseForm'>");

    // Flirt phrases
    page += F("<div class='card'><h2>💕 Flirt Phrases</h2>");
    page += F("<label>One phrase per line (max 8, ~40 chars each)</label>");
    page += F("<textarea name='flirt' rows='6'>");
    for (int i = 0; i < numFlirtPhrases; i++)
    {
        page += flirtPhrases[i];
        if (i < numFlirtPhrases - 1)
            page += "\n";
    }
    page += F("</textarea>");
    page += F("<p class='hint'>Shown when holding button 2 seconds</p></div>");

    // Avoidant phrases
    page += F("<div class='card'><h2>⚠️ Avoidant Phrases</h2>");
    page += F("<label>One phrase per line (max 8, ~40 chars each)</label>");
    page += F("<textarea name='avoidant' rows='6'>");
    for (int i = 0; i < numAvoidantPhrases; i++)
    {
        page += avoidantPhrases[i];
        if (i < numAvoidantPhrases - 1)
            page += "\n";
    }
    page += F("</textarea>");
    page += F("<p class='hint'>Shown when holding button 3 seconds</p></div>");

    // Taglines
    page += F("<div class='card'><h2>🎄 Startup Taglines</h2>");
    page += F("<label>One phrase per line (max 8, ~40 chars each)</label>");
    page += F("<textarea name='taglines' rows='6'>");
    for (int i = 0; i < numTaglines; i++)
    {
        page += taglinePhrases[i];
        if (i < numTaglines - 1)
            page += "\n";
    }
    page += F("</textarea>");
    page += F("<p class='hint'>Random one shown at startup</p></div>");

    // Christmas phrases
    page += F("<div class='card'><h2>🎅 Christmas Messages</h2>");
    page += F("<label>One phrase per line (max 8, ~40 chars each)</label>");
    page += F("<textarea name='christmas' rows='6'>");
    for (int i = 0; i < numChristmasPhrases; i++)
    {
        page += christmasPhrases[i];
        if (i < numChristmasPhrases - 1)
            page += "\n";
    }
    page += F("</textarea>");
    page += F("<p class='hint'>Scrolling messages on OLED display</p></div>");

    page += F("<button type='submit' class='save'>Save All Phrases</button>");
    page += F("</form>");

    // Reset button
    page += F("<button onclick='resetPhrases()' class='reset'>Reset All to Defaults</button>");

    // WiFi Settings section (separate form)
    page += F("<div class='card' style='margin-top:20px;'><h2>📶 WiFi Settings</h2>");
    page += F("<form id='wifiForm'>");
    page += F("<label>Network Name (SSID)</label>");
    page += F("<input type='text' name='ssid' maxlength='31' value='");
    page += wifiSSID;
    page += F("' style='width:100%;padding:10px;background:#252525;border:1px solid #444;border-radius:6px;color:#fff;font-size:1rem;margin-bottom:8px;'>");
    page += F("<label>Password</label>");
    page += F("<input type='text' name='pass' maxlength='31' value='");
    page += wifiPass;
    page += F("' style='width:100%;padding:10px;background:#252525;border:1px solid #444;border-radius:6px;color:#fff;font-size:1rem;'>");
    page += F("<p class='hint'>Requires restart to take effect</p>");
    page += F("<button type='submit' class='save' style='background:#0d6efd;'>Save WiFi Settings</button>");
    page += F("</form></div>");

    // Toast
    page += F("<div id='toast'>Saved!</div>");

    // JavaScript
    page += F("<script>\
function toast(msg,ok){var t=document.getElementById('toast');t.textContent=msg;t.style.background=ok?'#198754':'#dc3545';t.style.opacity=1;setTimeout(function(){t.style.opacity=0;},1500);}\
document.getElementById('phraseForm').onsubmit=function(e){\
e.preventDefault();\
fetch('/savephrases',{method:'POST',body:new FormData(this)}).then(r=>{if(r.ok){toast('Phrases saved!',true);}else{toast('Error saving',false);}}).catch(()=>toast('Error',false));\
};\
document.getElementById('wifiForm').onsubmit=function(e){\
e.preventDefault();\
fetch('/savewifi',{method:'POST',body:new FormData(this)}).then(r=>{if(r.ok){toast('WiFi saved! Restart to apply.',true);}else{toast('Error saving',false);}}).catch(()=>toast('Error',false));\
};\
function resetPhrases(){\
if(confirm('Reset all phrases to defaults?')){\
fetch('/resetphrases').then(r=>{if(r.ok){toast('Reset to defaults!',true);setTimeout(()=>location.reload(),500);}else{toast('Error',false);}});\
}}\
</script>");

    page += F("</body></html>");

    server.send(200, "text/html", page);
}

// Helper to parse phrases from textarea (newline separated)
int parsePhrases(const String &input, char dest[][MAX_PHRASE_LEN], int maxCount)
{
    int count = 0;
    int start = 0;
    int len = input.length();

    for (int i = 0; i <= len && count < maxCount; i++)
    {
        if (i == len || input[i] == '\n' || input[i] == '\r')
        {
            if (i > start)
            {
                int phraseLen = i - start;
                if (phraseLen > MAX_PHRASE_LEN - 1)
                    phraseLen = MAX_PHRASE_LEN - 1;

                // Copy phrase, skip leading/trailing whitespace
                int copyStart = start;
                while (copyStart < i && (input[copyStart] == ' ' || input[copyStart] == '\t'))
                    copyStart++;
                int copyEnd = i;
                while (copyEnd > copyStart && (input[copyEnd - 1] == ' ' || input[copyEnd - 1] == '\t' || input[copyEnd - 1] == '\r'))
                    copyEnd--;

                phraseLen = copyEnd - copyStart;
                if (phraseLen > 0 && phraseLen < MAX_PHRASE_LEN)
                {
                    for (int j = 0; j < phraseLen; j++)
                    {
                        dest[count][j] = input[copyStart + j];
                    }
                    dest[count][phraseLen] = '\0';
                    count++;
                }
            }
            start = i + 1;
        }
    }
    return count;
}

// Save phrases endpoint
void handleSavePhrases()
{
    bool changed = false;

    if (server.hasArg("flirt"))
    {
        numFlirtPhrases = parsePhrases(server.arg("flirt"), flirtPhrases, MAX_CUSTOM_PHRASES);
        if (numFlirtPhrases == 0)
        {
            // Keep at least one default
            numFlirtPhrases = 1;
            strncpy(flirtPhrases[0], DEFAULT_FLIRT[0], MAX_PHRASE_LEN - 1);
        }
        changed = true;
    }

    if (server.hasArg("avoidant"))
    {
        numAvoidantPhrases = parsePhrases(server.arg("avoidant"), avoidantPhrases, MAX_CUSTOM_PHRASES);
        if (numAvoidantPhrases == 0)
        {
            numAvoidantPhrases = 1;
            strncpy(avoidantPhrases[0], DEFAULT_AVOIDANT[0], MAX_PHRASE_LEN - 1);
        }
        changed = true;
    }

    if (server.hasArg("taglines"))
    {
        numTaglines = parsePhrases(server.arg("taglines"), taglinePhrases, MAX_CUSTOM_PHRASES);
        if (numTaglines == 0)
        {
            numTaglines = 1;
            strncpy(taglinePhrases[0], DEFAULT_TAGLINES[0], MAX_PHRASE_LEN - 1);
        }
        changed = true;
    }

    if (server.hasArg("christmas"))
    {
        numChristmasPhrases = parsePhrases(server.arg("christmas"), christmasPhrases, MAX_CUSTOM_PHRASES);
        if (numChristmasPhrases == 0)
        {
            numChristmasPhrases = 1;
            strncpy(christmasPhrases[0], DEFAULT_CHRISTMAS[0], MAX_PHRASE_LEN - 1);
        }
        changed = true;
    }

    if (changed)
    {
        savePhrasesToEEPROM();
        webLog("Phrases saved to EEPROM");
    }

    server.send(200, "text/plain", "OK");
}

// Reset phrases endpoint
void handleResetPhrases()
{
    resetPhrasesToDefaults();
    webLog("Phrases reset to defaults");
    server.send(200, "text/plain", "OK");
}

// Save WiFi settings endpoint
void handleSaveWifi()
{
    bool changed = false;

    if (server.hasArg("ssid"))
    {
        String ssid = server.arg("ssid");
        ssid.trim();
        if (ssid.length() > 0 && ssid.length() < MAX_SSID_LEN)
        {
            strncpy(wifiSSID, ssid.c_str(), MAX_SSID_LEN - 1);
            wifiSSID[MAX_SSID_LEN - 1] = '\0';
            changed = true;
        }
    }

    if (server.hasArg("pass"))
    {
        String pass = server.arg("pass");
        // Password can be empty for open networks, but we'll require at least 8 chars for WPA
        if (pass.length() >= 8 && pass.length() < MAX_PASS_LEN)
        {
            strncpy(wifiPass, pass.c_str(), MAX_PASS_LEN - 1);
            wifiPass[MAX_PASS_LEN - 1] = '\0';
            changed = true;
        }
        else if (pass.length() == 0)
        {
            // Allow empty password for open network
            wifiPass[0] = '\0';
            changed = true;
        }
    }

    if (changed)
    {
        savePhrasesToEEPROM();
        webLog("WiFi settings saved");
    }

    server.send(200, "text/plain", "OK");
}

// ========================
// BUTTON HANDLING
// ========================
// Interrupt service routine for button - ICACHE_RAM_ATTR required for ESP8266
ICACHE_RAM_ATTR void buttonISR()
{
    unsigned long now = millis();
    // Minimal debounce - ignore if too soon after last press
    if (now - lastButtonISRTime > MIN_BUTTON_INTERVAL)
    {
        buttonPressedISR = true;
        lastButtonISRTime = now;
    }
}

void readButton()
{
    unsigned long now = millis();
    bool isPressed = (digitalRead(BUTTON_PIN) == LOW);

    // Check if interrupt caught a press
    if (buttonPressedISR)
    {
        buttonPressedISR = false;

        // Start hold timer
        buttonHoldStart = now;

        // Immediate action: change mode (unless in special mode)
        if (flirtModeActive || avoidantModeActive)
        {
            // Exit special modes
            flirtModeActive = false;
            avoidantModeActive = false;
            displayShowMode(mode);
        }
        else
        {
            // Change mode immediately
            mode = (mode + 1) % NUM_MODES;
            animIndex = 0;
            displayShowMode(mode);
            webLogInt("Button mode: ", mode);
        }

        lastInteractionTime = now;
        displayRotationStart = now;
        lastModeChangeTime = now;
    }

    // Check for long hold while button is physically pressed
    static bool flirtHoldTriggered = false;
    static bool avoidantHoldTriggered = false;

    if (isPressed && buttonHoldStart > 0)
    {
        unsigned long holdDuration = now - buttonHoldStart;

        // 5 second hold -> avoidant mode
        if (holdDuration >= AVOIDANT_HOLD_MS && !avoidantHoldTriggered)
        {
            avoidantHoldTriggered = true;
            flirtHoldTriggered = true;

            flirtModeActive = false;
            avoidantModeActive = true;

            lastInteractionTime = now;
            displayRotationStart = now;
            brightnessDisplayUntil = 0;

            // Visual feedback: flash red/yellow
            for (int i = 0; i < NUM_LEDS; i++)
            {
                strip.setPixelColor(i, (i % 2 == 0) ? strip.Color(255, 50, 0) : strip.Color(255, 200, 0));
            }
            strip.show();
        }
        // 3 second hold -> flirt mode
        else if (holdDuration >= FLIRT_HOLD_MS && !flirtHoldTriggered)
        {
            flirtHoldTriggered = true;
            flirtModeActive = true;
            avoidantModeActive = false;

            lastInteractionTime = now;
            displayRotationStart = now;
            brightnessDisplayUntil = 0;

            // Visual feedback: flash pink
            for (int i = 0; i < NUM_LEDS; i++)
            {
                strip.setPixelColor(i, strip.Color(255, 100, 150));
            }
            strip.show();
        }
    }
    else if (!isPressed)
    {
        // Button released - reset hold tracking
        flirtHoldTriggered = false;
        avoidantHoldTriggered = false;
        buttonHoldStart = 0;
    }
}

// ========================
// OLED FUNCTIONS
// ========================
void displayInit()
{
    Wire.begin(); // SDA=D2, SCL=D1 on Wemos D1 mini

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        displayOK = false;
        return;
    }

    displayOK = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.display();
}

const char *modeName(int m)
{
    switch (m)
    {
    case 0:
        return "Red Glow";
    case 1:
        return "Green Glow";
    case 2:
        return "Candy Cane";
    case 3:
        return "Twinkle";
    case 4:
        return "Warm Gradient";
    case 5:
        return "Rainbow";
    case 6:
        return "Meteor Shower";
    case 7:
        return "Fire";
    case 8:
        return "Sparkle Burst";
    case 9:
        return "Ice Shimmer";
    case 10:
        return "Plasma Wave";
    case 11:
        return "Northern Lights";
    case 12:
        return "Cylon Scanner";
    case 13:
        return "Heat Geek";
    default:
        return "Unknown";
    }
}

// Helper to draw centred mode header (e.g. "3: Twinkle")
void displayModeHeader()
{
    // Build the header string: "N: ModeName" (display as 1-indexed for user)
    char header[24];
    snprintf(header, sizeof(header), "%d: %s", mode + 1, modeName(mode));

    int textWidth = strlen(header) * 6; // 6 pixels per char at size 1
    int x = (SCREEN_WIDTH - textWidth) / 2;
    if (x < 0)
        x = 0;

    display.setTextSize(1);
    display.setCursor(x, 0);
    display.print(header);
}

// Called on mode change: draw simple text splash, start timer
void displayShowMode(int m)
{
    if (!displayOK)
        return;

    modeChangeTime = millis();

    display.clearDisplay();
    displayModeHeader();
    display.display();
}

// Show brightness level feedback on display
void displayShowBrightness()
{
    if (!displayOK)
        return;

    display.clearDisplay();

    // Title
    const char *title = "Brightness";
    int titleWidth = strlen(title) * 6;
    int titleX = (SCREEN_WIDTH - titleWidth) / 2;
    display.setTextSize(1);
    display.setCursor(titleX, 4);
    display.print(title);

    // Show current brightness as percentage, large and centred
    char valStr[8];
    snprintf(valStr, sizeof(valStr), "%d%%", BRIGHTNESS_PERCENT[currentBrightnessLevel]);
    int valWidth = strlen(valStr) * 12; // size 2 = 12 pixels per char
    int valX = (SCREEN_WIDTH - valWidth) / 2;
    display.setTextSize(2);
    display.setCursor(valX, 24);
    display.print(valStr);

    // Draw a brightness bar at the bottom
    int barY = 50;
    int barHeight = 8;
    int barMaxWidth = 100;
    int barX = (SCREEN_WIDTH - barMaxWidth) / 2;
    int filledWidth = (BRIGHTNESS_PERCENT[currentBrightnessLevel] * barMaxWidth) / 100;

    // Outer frame
    display.drawRect(barX, barY, barMaxWidth, barHeight, SSD1306_WHITE);
    // Filled portion
    display.fillRect(barX + 1, barY + 1, filledWidth - 2, barHeight - 2, SSD1306_WHITE);

    display.display();

    // Set timer to hold this display for a few seconds
    brightnessDisplayUntil = millis() + BRIGHTNESS_DISPLAY_DURATION;
}

// Startup animation - sleek intro with LED sweep and text reveal
void playStartupAnimation()
{
    // Wait for button press before starting
    if (displayOK)
    {
        display.clearDisplay();
        display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1);
        const char *prompt = "Press button to start";
        int promptWidth = strlen(prompt) * 6;
        display.setCursor((SCREEN_WIDTH - promptWidth) / 2, 28);
        display.print(prompt);
        display.display();
    }

    // Wait for button press (button is active LOW)
    while (digitalRead(BUTTON_PIN) == HIGH)
    {
        yield(); // Keep ESP happy
    }
    // Wait for release
    while (digitalRead(BUTTON_PIN) == LOW)
    {
        yield();
    }
    delay(50); // Debounce

    // Pick random tagline from customisable phrases
    const char *tagline = taglinePhrases[random(numTaglines)];

    // Heat Geek colours for LED sweep
    const float colourStart[3] = {255, 115, 25}; // Pumpkin Spice
    const float colourEnd[3] = {252, 0, 123};    // Neon Pink

    // Phase 1: LED sweep up while "MIKEY" fades in (1.2s)
    for (int step = 0; step <= 30; step++)
    {
        float progress = step / 30.0f;

        // LED sweep - light up LEDs from bottom to top
        int litLeds = (int)(progress * NUM_LEDS);
        for (int i = 0; i < NUM_LEDS; i++)
        {
            if (i < litLeds)
            {
                // Gradient based on position
                float t = (float)i / NUM_LEDS;
                uint8_t r = (uint8_t)(colourStart[0] * (1 - t) + colourEnd[0] * t);
                uint8_t g = (uint8_t)(colourStart[1] * (1 - t) + colourEnd[1] * t);
                uint8_t b = (uint8_t)(colourStart[2] * (1 - t) + colourEnd[2] * t);
                strip.setPixelColor(i, strip.Color(r, g, b));
            }
            else
            {
                strip.setPixelColor(i, 0);
            }
        }
        strip.show();

        // OLED - fade in "MIKEY" (simulate fade with delayed appearance)
        if (displayOK && step > 10)
        {
            display.clearDisplay();
            display.setTextColor(SSD1306_WHITE);
            display.setTextSize(3);
            const char *name = "MIKEY";
            int nameWidth = strlen(name) * 18;
            int nameX = (SCREEN_WIDTH - nameWidth) / 2;
            display.setCursor(nameX, 20);
            display.print(name);
            display.display();
        }

        delay(40);
    }

    // Phase 2: Tagline types in letter by letter (1.5s)
    int taglineLen = strlen(tagline);

    for (int i = 0; i <= taglineLen; i++)
    {
        if (displayOK)
        {
            display.clearDisplay();

            // Name stays visible
            display.setTextSize(3);
            const char *name = "MIKEY";
            int nameWidth = strlen(name) * 18;
            int nameX = (SCREEN_WIDTH - nameWidth) / 2;
            display.setCursor(nameX, 16);
            display.print(name);

            // Tagline types in
            display.setTextSize(1);
            char partial[64];
            strncpy(partial, tagline, i);
            partial[i] = '\0';
            int tagWidth = i * 6;
            int tagX = (SCREEN_WIDTH - strlen(tagline) * 6) / 2;
            display.setCursor(tagX, 48);
            display.print(partial);

            display.display();
        }

        // Subtle LED shimmer while typing
        unsigned long now = millis();
        for (int led = 0; led < NUM_LEDS; led++)
        {
            float t = (float)led / NUM_LEDS;
            float shimmer = 0.85f + 0.15f * sin(now / 100.0f + led * 0.2f);
            uint8_t r = (uint8_t)((colourStart[0] * (1 - t) + colourEnd[0] * t) * shimmer);
            uint8_t g = (uint8_t)((colourStart[1] * (1 - t) + colourEnd[1] * t) * shimmer);
            uint8_t b = (uint8_t)((colourStart[2] * (1 - t) + colourEnd[2] * t) * shimmer);
            strip.setPixelColor(led, strip.Color(r, g, b));
        }
        strip.show();

        delay(50);
    }

    // Phase 3: Hold for a moment (0.8s)
    delay(800);

    // Phase 4: Fade out (0.5s)
    for (int step = 20; step >= 0; step--)
    {
        float brightness = step / 20.0f;

        // Fade LEDs
        for (int i = 0; i < NUM_LEDS; i++)
        {
            float t = (float)i / NUM_LEDS;
            uint8_t r = (uint8_t)((colourStart[0] * (1 - t) + colourEnd[0] * t) * brightness);
            uint8_t g = (uint8_t)((colourStart[1] * (1 - t) + colourEnd[1] * t) * brightness);
            uint8_t b = (uint8_t)((colourStart[2] * (1 - t) + colourEnd[2] * t) * brightness);
            strip.setPixelColor(i, strip.Color(r, g, b));
        }
        strip.show();

        delay(25);
    }

    // Clear display ready for normal operation
    if (displayOK)
    {
        display.clearDisplay();
        display.display();
    }
    strip.clear();
    strip.show();
}

// Per-frame update
void displayUpdate(unsigned long now)
{
    if (!displayOK)
        return;

    // If brightness overlay is active, don't update the display
    if (now < brightnessDisplayUntil)
        return;

    // Limit frame rate
    if (now - lastDisplayFrame < DISPLAY_FRAME_INTERVAL)
        return;
    lastDisplayFrame = now;

    // ---------- Flirt mode display ----------
    if (flirtModeActive)
    {
        display.clearDisplay();

        // Pulsing heart in the top right corner
        // Heartbeat timing synced with LED pattern
        float cyclePos = (now % 1200) / 1200.0f;
        float pulse = 0;
        if (cyclePos < 0.1f)
        {
            pulse = sin(cyclePos / 0.1f * 3.14159f);
        }
        else if (cyclePos >= 0.2f && cyclePos < 0.3f)
        {
            pulse = sin((cyclePos - 0.2f) / 0.1f * 3.14159f) * 0.7f;
        }

        int hx = SCREEN_WIDTH / 2; // top centre
        int hy = 12;
        int heartSize = 6 + (int)(pulse * 3); // Pulse between 6 and 9

        // Draw heart shape: two circles and a triangle
        int circleR = heartSize / 2;
        int circleOffset = circleR - 1;

        // Left circle of heart
        display.fillCircle(hx - circleOffset, hy - circleR / 2, circleR, SSD1306_WHITE);
        // Right circle of heart
        display.fillCircle(hx + circleOffset, hy - circleR / 2, circleR, SSD1306_WHITE);
        // Bottom triangle
        display.fillTriangle(
            hx - heartSize, hy - circleR / 2,
            hx + heartSize, hy - circleR / 2,
            hx, hy + heartSize,
            SSD1306_WHITE);

        // Scrolling compliment text - larger size 2 font
        static int flirtScrollX = SCREEN_WIDTH;
        static int flirtPhraseIdx = 0;
        static unsigned long lastFlirtScrollStep = 0;
        const unsigned long FLIRT_SCROLL_STEP_MS = 50;

        const char *phrase = flirtPhrases[flirtPhraseIdx];
        int phraseWidth = strlen(phrase) * 12; // size 2 = 12 pixels per char

        // Scroll text
        if (now - lastFlirtScrollStep > FLIRT_SCROLL_STEP_MS)
        {
            lastFlirtScrollStep = now;
            flirtScrollX -= 3;

            // Only advance to next phrase after current one has fully scrolled off
            if (flirtScrollX + phraseWidth < 0)
            {
                flirtScrollX = SCREEN_WIDTH;
                flirtPhraseIdx = (flirtPhraseIdx + 1) % numFlirtPhrases;
            }
        }

        display.setTextSize(2);
        display.setTextWrap(false);
        display.setCursor(flirtScrollX, 28); // centred vertically for size 2
        display.print(phrase);
        display.setTextWrap(true);

        display.display();
        return;
    }

    // ---------- Avoidant mode display ----------
    if (avoidantModeActive)
    {
        display.clearDisplay();

        // Pulsing warning triangle at top centre
        bool flashOn = ((now / 400) % 2) == 0; // flash every 400ms

        int tx = SCREEN_WIDTH / 2; // centre
        int ty = 12;
        int triSize = flashOn ? 12 : 10; // pulse size

        // Draw warning triangle
        display.drawTriangle(
            tx, ty - triSize,               // top point
            tx - triSize, ty + triSize / 2, // bottom left
            tx + triSize, ty + triSize / 2, // bottom right
            SSD1306_WHITE);

        // Exclamation mark inside
        if (flashOn)
        {
            display.drawLine(tx, ty - triSize + 4, tx, ty + triSize / 2 - 4, SSD1306_WHITE);
            display.drawPixel(tx, ty + triSize / 2 - 1, SSD1306_WHITE);
        }

        // Scrolling warning text - larger size 2 font
        static int avoidScrollX = SCREEN_WIDTH;
        static int avoidPhraseIdx = 0;
        static unsigned long lastAvoidScrollStep = 0;
        const unsigned long AVOID_SCROLL_STEP_MS = 50;

        const char *phrase = avoidantPhrases[avoidPhraseIdx];
        int phraseWidth = strlen(phrase) * 12; // size 2 = 12 pixels per char

        // Scroll text
        if (now - lastAvoidScrollStep > AVOID_SCROLL_STEP_MS)
        {
            lastAvoidScrollStep = now;
            avoidScrollX -= 3;

            // Only advance to next phrase after current one has fully scrolled off
            if (avoidScrollX + phraseWidth < 0)
            {
                avoidScrollX = SCREEN_WIDTH;
                avoidPhraseIdx = (avoidPhraseIdx + 1) % numAvoidantPhrases;
            }
        }

        display.setTextSize(2);
        display.setTextWrap(false);
        display.setCursor(avoidScrollX, 28); // centred vertically for size 2
        display.print(phrase);
        display.setTextWrap(true);

        display.display();
        return;
    }

    // Rotate between animation and messages every 15 seconds
    const unsigned long ROTATE_INTERVAL_MS = 15000;
    unsigned long timeSinceRotationStart = now - displayRotationStart;
    bool showMessage = ((timeSinceRotationStart / ROTATE_INTERVAL_MS) % 2 == 1) && (numChristmasPhrases > 0);

    // ---------- Message display phase ----------
    if (showMessage)
    {
        // Calculate which message to show based on time since rotation started
        unsigned long msgPhaseTime = (timeSinceRotationStart / ROTATE_INTERVAL_MS) / 2; // changes every 30s (full cycle)
        int msgIndex = (int)(msgPhaseTime % numChristmasPhrases);
        if (msgIndex < 0 || msgIndex >= numChristmasPhrases)
            msgIndex = 0;
        String msgStr = christmasPhrases[msgIndex];

        // Common header
        display.clearDisplay();
        displayModeHeader();

        const int textSize = 3;
        int textWidth = msgStr.length() * 6 * textSize; // approx width

        // If the text fits, show it centred, no scroll, no wrapping
        if (textWidth <= SCREEN_WIDTH)
        {
            display.setTextSize(textSize);
            display.setTextWrap(false); // avoid a second line
            int x = (SCREEN_WIDTH - textWidth) / 2;
            int y = 28; // adjusted for larger text
            display.setCursor(x, y);
            display.print(msgStr);
            display.setTextWrap(true); // restore default
            display.display();
            return;
        }

        // Otherwise, scroll sideways (no wrapping)
        static int scrollX = SCREEN_WIDTH;
        static int lastMsgIndex = -1;
        static unsigned long lastScrollStep = 0;
        const unsigned long SCROLL_STEP_MS = 50; // slightly faster for larger text

        // Reset scroll if message index changed
        if (msgIndex != lastMsgIndex)
        {
            lastMsgIndex = msgIndex;
            scrollX = SCREEN_WIDTH;
            lastScrollStep = now;
        }

        // Only move the text every SCROLL_STEP_MS
        if (now - lastScrollStep > SCROLL_STEP_MS)
        {
            lastScrollStep = now;

            scrollX -= 3; // 3 pixels per step (faster for larger text)

            if (scrollX + textWidth < 0)
            {
                scrollX = SCREEN_WIDTH; // wrap from the right again
            }
        }

        display.setTextSize(textSize);
        display.setTextWrap(false); // keep it on one line
        int y = 28;                 // adjusted for larger text
        display.setCursor(scrollX, y);
        display.print(msgStr);
        display.setTextWrap(true); // restore default

        display.display();
        return;
    }

    // ---------- Animation display phase ----------
    static uint16_t frame = 0;
    frame++;

    display.clearDisplay();
    displayModeHeader();

    // Draw mode-specific animation in lower area (y >= 16)
    switch (mode)
    {
    case 0:
    {                                         // Two swinging baubles
        float swing = sin(frame * 0.08f) * 6; // gentle swing -6 to +6 pixels

        // Left bauble (larger)
        int cx1 = SCREEN_WIDTH / 2 - 18 + (int)swing;
        int cy1 = 42;
        int r1 = 11;

        // Hanging string (from top, curves with swing)
        display.drawLine(cx1 - (int)(swing * 0.3f), 12, cx1, cy1 - r1 - 2, SSD1306_WHITE);

        // Bauble cap
        display.fillRect(cx1 - 3, cy1 - r1 - 2, 7, 4, SSD1306_WHITE);

        // Main bauble
        display.drawCircle(cx1, cy1, r1, SSD1306_WHITE);
        display.drawCircle(cx1, cy1, r1 - 1, SSD1306_WHITE);

        // Decorative pattern - vertical and horizontal bands
        display.drawLine(cx1 - r1 + 2, cy1, cx1 + r1 - 2, cy1, SSD1306_WHITE);
        display.drawLine(cx1, cy1 - r1 + 2, cx1, cy1 + r1 - 2, SSD1306_WHITE);

        // Shine
        display.drawPixel(cx1 - 3, cy1 - 4, SSD1306_WHITE);
        display.drawPixel(cx1 - 2, cy1 - 4, SSD1306_WHITE);
        display.drawPixel(cx1 - 3, cy1 - 3, SSD1306_WHITE);

        // Right bauble (smaller, swings opposite)
        int cx2 = SCREEN_WIDTH / 2 + 20 - (int)(swing * 0.7f);
        int cy2 = 38;
        int r2 = 8;

        // Hanging string
        display.drawLine(cx2 + (int)(swing * 0.2f), 12, cx2, cy2 - r2 - 2, SSD1306_WHITE);

        // Bauble cap
        display.fillRect(cx2 - 2, cy2 - r2 - 2, 5, 3, SSD1306_WHITE);

        // Main bauble
        display.drawCircle(cx2, cy2, r2, SSD1306_WHITE);
        display.drawCircle(cx2, cy2, r2 - 1, SSD1306_WHITE);

        // Simple band
        display.drawLine(cx2 - r2 + 2, cy2, cx2 + r2 - 2, cy2, SSD1306_WHITE);

        // Shine
        display.drawPixel(cx2 - 2, cy2 - 3, SSD1306_WHITE);
        display.drawPixel(cx2 - 1, cy2 - 3, SSD1306_WHITE);

        break;
    }

    case 1:
    { // Christmas tree with baubles and twinkling star
        int x0 = SCREEN_WIDTH / 2;
        int baseY = 52;
        int treeHeight = 32;
        int baseWidth = 28;
        int y0 = baseY - treeHeight; // tree top

        // Trunk
        display.fillRect(x0 - 3, baseY, 7, 6, SSD1306_WHITE);

        // Tree layers (3 triangular sections for a fuller look)
        for (int layer = 0; layer < 3; layer++)
        {
            int layerTop = y0 + layer * 8;
            int layerBottom = y0 + 12 + layer * 10;
            int layerWidth = 10 + layer * 8;

            display.drawTriangle(
                x0, layerTop,
                x0 - layerWidth / 2, layerBottom,
                x0 + layerWidth / 2, layerBottom,
                SSD1306_WHITE);
        }

        // Baubles (small circles that twinkle)
        int baublePositions[][2] = {{x0 - 6, y0 + 20}, {x0 + 5, y0 + 16}, {x0 - 3, y0 + 28}, {x0 + 7, y0 + 26}, {x0, y0 + 12}};
        for (int i = 0; i < 5; i++)
        {
            // Alternate visibility based on frame for twinkling effect
            if ((frame / 6 + i * 3) % 10 < 7)
            {
                display.fillCircle(baublePositions[i][0], baublePositions[i][1], 2, SSD1306_WHITE);
            }
        }

        // Star at top (5-pointed, animated glow)
        int starY = y0 - 4;
        bool starBright = ((frame / 8) % 3) != 0;
        // Star centre
        display.drawPixel(x0, starY, SSD1306_WHITE);
        // Star points
        display.drawLine(x0, starY - 3, x0, starY + 3, SSD1306_WHITE); // vertical
        display.drawLine(x0 - 3, starY, x0 + 3, starY, SSD1306_WHITE); // horizontal
        if (starBright)
        {
            display.drawLine(x0 - 2, starY - 2, x0 + 2, starY + 2, SSD1306_WHITE); // diagonal
            display.drawLine(x0 + 2, starY - 2, x0 - 2, starY + 2, SSD1306_WHITE); // diagonal
        }

        break;
    }

    case 2:
    { // Candy cane: thick with animated stripes on hook and stem
        int cx = SCREEN_WIDTH / 2 + 5;
        int topY = 24; // moved down to avoid header overlap
        int stemBottomY = 58;
        int hookRadius = 10; // slightly smaller to fit

        // Gentle bob
        int yOffset = (int)(sin(frame * 0.08f) * 2);

        int hookCenterY = topY + yOffset;
        int hookCenterX = cx - hookRadius;
        int stemBottom = stemBottomY + yOffset;

        // Draw thick stem (5 pixels wide)
        for (int dx = -2; dx <= 2; dx++)
        {
            display.drawLine(cx + dx, hookCenterY, cx + dx, stemBottom, SSD1306_WHITE);
        }

        // Draw thick curved hook
        for (int angle = 0; angle <= 180; angle += 5)
        {
            float rad = angle * 3.1415926f / 180.0f;
            for (int thickness = -2; thickness <= 2; thickness++)
            {
                int x = hookCenterX + (int)(cos(rad) * (hookRadius + thickness * 0.3f));
                int y = hookCenterY - (int)(sin(rad) * (hookRadius + thickness * 0.3f));
                display.drawPixel(x, y, SSD1306_WHITE);
            }
        }

        // Animated stripes on stem
        int stripePhase = (frame / 2) % 10;
        for (int sy = stemBottom - stripePhase; sy > hookCenterY + 2; sy -= 10)
        {
            if (sy <= stemBottom && sy > hookCenterY)
            {
                // Diagonal stripe across stem width
                display.drawLine(cx - 3, sy, cx + 3, sy - 4, SSD1306_WHITE);
                display.drawLine(cx - 3, sy + 1, cx + 3, sy - 3, SSD1306_WHITE);
            }
        }

        // Animated stripes on hook curve
        for (int i = 0; i < 4; i++)
        {
            int stripeAngle = ((frame * 2) + i * 45) % 180;
            float rad = stripeAngle * 3.1415926f / 180.0f;
            int x1 = hookCenterX + (int)(cos(rad) * (hookRadius - 3));
            int y1 = hookCenterY - (int)(sin(rad) * (hookRadius - 3));
            int x2 = hookCenterX + (int)(cos(rad) * (hookRadius + 3));
            int y2 = hookCenterY - (int)(sin(rad) * (hookRadius + 3));
            display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
        }

        break;
    }

    case 3:
    { // Twinkling starfield with pulsing stars
        // Fixed star positions for consistent look
        const int starPositions[][2] = {
            {15, 22}, {45, 18}, {78, 25}, {105, 20}, {30, 35}, {60, 42}, {95, 38}, {20, 52}, {50, 55}, {85, 50}, {110, 45}, {38, 28}, {70, 32}, {12, 40}, {100, 55}};
        const int numStars = 15;

        // Draw each star with varying brightness based on frame
        for (int i = 0; i < numStars; i++)
        {
            int x = starPositions[i][0];
            int y = starPositions[i][1];

            // Each star has its own phase for twinkling
            float phase = sin((frame + i * 17) * 0.15f);
            bool bright = phase > 0.3f;
            bool veryBright = phase > 0.7f;

            // Always draw centre pixel
            display.drawPixel(x, y, SSD1306_WHITE);

            // Bright stars get cross pattern
            if (bright)
            {
                display.drawPixel(x - 1, y, SSD1306_WHITE);
                display.drawPixel(x + 1, y, SSD1306_WHITE);
                display.drawPixel(x, y - 1, SSD1306_WHITE);
                display.drawPixel(x, y + 1, SSD1306_WHITE);
            }

            // Very bright stars get diagonal rays too
            if (veryBright)
            {
                display.drawPixel(x - 1, y - 1, SSD1306_WHITE);
                display.drawPixel(x + 1, y - 1, SSD1306_WHITE);
                display.drawPixel(x - 1, y + 1, SSD1306_WHITE);
                display.drawPixel(x + 1, y + 1, SSD1306_WHITE);
            }
        }

        // One special large pulsing star in the centre
        int cx = SCREEN_WIDTH / 2;
        int cy = 38;
        float mainPulse = (sin(frame * 0.1f) + 1.0f) * 0.5f;
        int rayLen = 3 + (int)(mainPulse * 4);

        // Draw 4-pointed star
        display.drawLine(cx, cy - rayLen, cx, cy + rayLen, SSD1306_WHITE);
        display.drawLine(cx - rayLen, cy, cx + rayLen, cy, SSD1306_WHITE);
        // Shorter diagonal rays
        int diagLen = rayLen / 2;
        display.drawLine(cx - diagLen, cy - diagLen, cx + diagLen, cy + diagLen, SSD1306_WHITE);
        display.drawLine(cx + diagLen, cy - diagLen, cx - diagLen, cy + diagLen, SSD1306_WHITE);

        break;
    }

    case 4:
    { // Candle with dripping wax and organic flame
        int cx = SCREEN_WIDTH / 2;
        int baseY = 58;
        int candleTop = baseY - 18;

        // Candle body with rounded edges
        display.fillRect(cx - 8, candleTop, 16, 20, SSD1306_WHITE);

        // Wax drips on sides (animated positions)
        int drip1Y = candleTop + 5 + ((frame / 4) % 8);
        int drip2Y = candleTop + 2 + ((frame / 5 + 3) % 10);
        int drip3Y = candleTop + 8 + ((frame / 3 + 7) % 6);

        // Left drips
        display.drawLine(cx - 8, candleTop + 3, cx - 9, drip1Y, SSD1306_WHITE);
        display.drawPixel(cx - 9, drip1Y + 1, SSD1306_WHITE);

        // Right drips
        display.drawLine(cx + 7, candleTop + 5, cx + 8, drip2Y, SSD1306_WHITE);
        display.drawPixel(cx + 8, drip2Y + 1, SSD1306_WHITE);
        display.drawLine(cx + 7, candleTop + 1, cx + 9, drip3Y, SSD1306_WHITE);

        // Melted wax pool at top
        display.drawLine(cx - 6, candleTop, cx + 5, candleTop, SSD1306_WHITE);
        display.drawLine(cx - 5, candleTop - 1, cx + 4, candleTop - 1, SSD1306_WHITE);

        // Wick
        display.drawLine(cx, candleTop - 1, cx, candleTop - 5, SSD1306_WHITE);

        // Organic flickering flame
        int flickerX = (int)(sin(frame * 0.3f) * 2);
        int flickerH = (int)(sin(frame * 0.2f) * 2);
        int flameBase = candleTop - 5;
        int flameTop = flameBase - 14 - flickerH;

        // Outer flame shape (curved)
        for (int y = flameBase; y > flameTop; y--)
        {
            float progress = (float)(flameBase - y) / (flameBase - flameTop);
            int width = (int)((1.0f - progress * progress) * 6); // wider at bottom
            int xOff = (int)(sin(y * 0.5f + frame * 0.2f) * progress * 2) + flickerX * progress;

            if (width > 0)
            {
                display.drawPixel(cx + xOff - width, y, SSD1306_WHITE);
                display.drawPixel(cx + xOff + width, y, SSD1306_WHITE);
            }
            // Fill middle at bottom half
            if (progress < 0.6f && width > 1)
            {
                display.drawLine(cx + xOff - width + 1, y, cx + xOff + width - 1, y, SSD1306_WHITE);
            }
        }

        // Glowing particles rising
        for (int i = 0; i < 4; i++)
        {
            int sparkY = flameTop - 3 - ((frame / 2 + i * 5) % 15);
            int sparkX = cx + (int)(sin(frame * 0.1f + i * 2) * 4);
            if (sparkY > 12 && ((frame + i * 3) % 4) != 0)
            {
                display.drawPixel(sparkX, sparkY, SSD1306_WHITE);
            }
        }

        break;
    }

    case 5:
    { // Rainbow -> wrapped present with bow
        int cx = SCREEN_WIDTH / 2;
        int boxTop = 28;
        int boxBottom = 56;
        int boxLeft = cx - 18;
        int boxRight = cx + 18;

        // Present box
        display.drawRect(boxLeft, boxTop, boxRight - boxLeft, boxBottom - boxTop, SSD1306_WHITE);

        // Vertical ribbon
        display.drawLine(cx - 2, boxTop, cx - 2, boxBottom, SSD1306_WHITE);
        display.drawLine(cx + 2, boxTop, cx + 2, boxBottom, SSD1306_WHITE);

        // Horizontal ribbon
        int ribbonY = boxTop + (boxBottom - boxTop) / 2;
        display.drawLine(boxLeft, ribbonY - 2, boxRight, ribbonY - 2, SSD1306_WHITE);
        display.drawLine(boxLeft, ribbonY + 2, boxRight, ribbonY + 2, SSD1306_WHITE);

        // Bow on top (animated wiggle)
        int wiggle = (int)(sin(frame * 0.2f) * 2);
        int bowY = boxTop - 6;

        // Left loop of bow
        display.drawCircle(cx - 8 + wiggle, bowY, 5, SSD1306_WHITE);
        // Right loop of bow
        display.drawCircle(cx + 8 - wiggle, bowY, 5, SSD1306_WHITE);
        // Centre knot
        display.fillRect(cx - 3, bowY - 3, 6, 6, SSD1306_WHITE);

        // Ribbon tails
        display.drawLine(cx - 2, bowY + 3, cx - 6, boxTop - 1, SSD1306_WHITE);
        display.drawLine(cx + 2, bowY + 3, cx + 6, boxTop - 1, SSD1306_WHITE);

        // Sparkles around present (twinkling)
        int sparklePositions[][2] = {{boxLeft - 6, boxTop + 5}, {boxRight + 4, boxTop + 8}, {boxLeft - 4, boxBottom - 8}, {boxRight + 6, boxBottom - 5}};
        for (int i = 0; i < 4; i++)
        {
            if ((frame / 5 + i * 4) % 8 < 5)
            {
                int sx = sparklePositions[i][0];
                int sy = sparklePositions[i][1];
                display.drawPixel(sx, sy, SSD1306_WHITE);
                display.drawPixel(sx - 1, sy, SSD1306_WHITE);
                display.drawPixel(sx + 1, sy, SSD1306_WHITE);
                display.drawPixel(sx, sy - 1, SSD1306_WHITE);
                display.drawPixel(sx, sy + 1, SSD1306_WHITE);
            }
        }

        break;
    }

    case 6:
    { // Meteor Shower: shooting star streaking across
        int cx = SCREEN_WIDTH / 2;

        // Shooting star position (moves diagonally)
        int starX = (frame * 3) % (SCREEN_WIDTH + 40) - 20;
        int starY = 20 + (starX / 4);

        // Draw the tail (fading trail)
        for (int t = 0; t < 12; t++)
        {
            int tx = starX - t * 2;
            int ty = starY - t / 2;
            if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 12 && ty < SCREEN_HEIGHT)
            {
                if (t < 4)
                {
                    display.drawPixel(tx, ty, SSD1306_WHITE);
                    display.drawPixel(tx, ty + 1, SSD1306_WHITE);
                }
                else if (t < 8)
                {
                    display.drawPixel(tx, ty, SSD1306_WHITE);
                }
                else if ((t + frame) % 2 == 0)
                {
                    display.drawPixel(tx, ty, SSD1306_WHITE);
                }
            }
        }

        // Bright star head
        if (starX >= 0 && starX < SCREEN_WIDTH && starY >= 12 && starY < SCREEN_HEIGHT)
        {
            display.fillCircle(starX, starY, 2, SSD1306_WHITE);
        }

        // Background stars (static)
        display.drawPixel(15, 25, SSD1306_WHITE);
        display.drawPixel(45, 18, SSD1306_WHITE);
        display.drawPixel(80, 30, SSD1306_WHITE);
        display.drawPixel(100, 22, SSD1306_WHITE);
        display.drawPixel(60, 50, SSD1306_WHITE);
        display.drawPixel(30, 45, SSD1306_WHITE);
        display.drawPixel(110, 55, SSD1306_WHITE);

        break;
    }

    case 7:
    { // Fire: animated campfire with rising embers
        int cx = SCREEN_WIDTH / 2;
        int baseY = 58;

        // Log base
        display.fillRect(cx - 20, baseY - 4, 40, 6, SSD1306_WHITE);
        display.fillRect(cx - 15, baseY - 8, 30, 5, SSD1306_WHITE);

        // Flames (multiple flickering triangles)
        int flicker1 = ((frame / 2) % 3) - 1;
        int flicker2 = ((frame / 3 + 1) % 3) - 1;
        int flicker3 = ((frame / 2 + 2) % 3) - 1;

        // Centre flame (tallest)
        int flameH1 = 24 + (frame % 4);
        display.drawTriangle(
            cx + flicker1, baseY - 10 - flameH1,
            cx - 8, baseY - 10,
            cx + 8, baseY - 10,
            SSD1306_WHITE);

        // Left flame
        int flameH2 = 16 + ((frame + 2) % 3);
        display.drawTriangle(
            cx - 10 + flicker2, baseY - 8 - flameH2,
            cx - 16, baseY - 8,
            cx - 4, baseY - 8,
            SSD1306_WHITE);

        // Right flame
        int flameH3 = 18 + ((frame + 1) % 4);
        display.drawTriangle(
            cx + 10 + flicker3, baseY - 8 - flameH3,
            cx + 4, baseY - 8,
            cx + 16, baseY - 8,
            SSD1306_WHITE);

        // Rising embers/sparks
        for (int e = 0; e < 4; e++)
        {
            int emberY = baseY - 30 - ((frame / 2 + e * 8) % 25);
            int emberX = cx - 10 + ((frame / 3 + e * 11) % 20);
            if (emberY > 12 && ((frame / 4 + e) % 3) != 0)
            {
                display.drawPixel(emberX, emberY, SSD1306_WHITE);
            }
        }

        break;
    }

    case 8:
    { // Sparkle Burst: expanding starburst pattern - synced with LEDs
        int cx = SCREEN_WIDTH / 2;
        int cy = 40;

        // Get synced phase - same as LED strip
        unsigned long now = millis();
        bool isBurst;
        float progress = getSparkleBurstPhase(now, &isBurst);

        if (!isBurst)
        {
            // Build-up: pulsing centre circle
            int pulseR = 3 + (int)(progress * 8);
            display.drawCircle(cx, cy, pulseR, SSD1306_WHITE);
            if (pulseR > 5)
            {
                display.drawCircle(cx, cy, pulseR - 3, SSD1306_WHITE);
            }

            // Small dots moving inward
            for (int i = 0; i < 8; i++)
            {
                float angle = i * 0.785f; // 45 degrees apart
                int dist = 20 - (int)(progress * 15);
                if (dist > 5)
                {
                    int dx = (int)(cos(angle) * dist);
                    int dy = (int)(sin(angle) * dist);
                    display.drawPixel(cx + dx, cy + dy, SSD1306_WHITE);
                }
            }
        }
        else
        {
            // Burst: rays shooting outward
            int rayLen = (int)(progress * 40);

            for (int i = 0; i < 8; i++)
            {
                float angle = i * 0.785f;
                int x1 = cx + (int)(cos(angle) * 3);
                int y1 = cy + (int)(sin(angle) * 3);
                int x2 = cx + (int)(cos(angle) * rayLen);
                int y2 = cy + (int)(sin(angle) * rayLen);
                display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
            }

            // Sparkles at ray ends
            if (progress > 0.2f)
            {
                for (int i = 0; i < 8; i++)
                {
                    float angle = i * 0.785f + 0.39f; // offset
                    int dist = rayLen - 2;
                    int sx = cx + (int)(cos(angle) * dist);
                    int sy = cy + (int)(sin(angle) * dist);
                    if (sx > 0 && sx < SCREEN_WIDTH && sy > 12 && sy < SCREEN_HEIGHT)
                    {
                        if ((frame + i) % 2 == 0)
                            display.drawPixel(sx, sy, SSD1306_WHITE);
                    }
                }
            }
        }

        break;
    }

    case 9:
    { // Ice Shimmer: snowflake with shimmering rays
        int cx = SCREEN_WIDTH / 2;
        int cy = 40;

        // Main snowflake arms (6 directions)
        int armLen = 16;
        for (int i = 0; i < 6; i++)
        {
            float angle = i * 1.047f; // 60 degrees
            int x2 = cx + (int)(cos(angle) * armLen);
            int y2 = cy + (int)(sin(angle) * armLen);
            display.drawLine(cx, cy, x2, y2, SSD1306_WHITE);

            // Small branches on each arm
            int branchDist = 8;
            int bx = cx + (int)(cos(angle) * branchDist);
            int by = cy + (int)(sin(angle) * branchDist);
            float branchAngle1 = angle + 0.5f;
            float branchAngle2 = angle - 0.5f;
            display.drawLine(bx, by, bx + (int)(cos(branchAngle1) * 5), by + (int)(sin(branchAngle1) * 5), SSD1306_WHITE);
            display.drawLine(bx, by, bx + (int)(cos(branchAngle2) * 5), by + (int)(sin(branchAngle2) * 5), SSD1306_WHITE);
        }

        // Centre hexagon
        display.drawCircle(cx, cy, 3, SSD1306_WHITE);

        // Shimmering sparkles around the snowflake
        for (int s = 0; s < 6; s++)
        {
            if ((frame / 4 + s * 2) % 6 < 3)
            {
                float angle = s * 1.047f + 0.52f + (frame * 0.02f);
                int dist = 22 + (frame % 4);
                int sx = cx + (int)(cos(angle) * dist);
                int sy = cy + (int)(sin(angle) * dist);
                if (sx > 0 && sx < SCREEN_WIDTH && sy > 12 && sy < SCREEN_HEIGHT)
                {
                    display.drawPixel(sx, sy, SSD1306_WHITE);
                    display.drawPixel(sx + 1, sy, SSD1306_WHITE);
                    display.drawPixel(sx, sy + 1, SSD1306_WHITE);
                }
            }
        }

        break;
    }

    case 10:
    { // Plasma Wave: flowing colour bars
        int cx = SCREEN_WIDTH / 2;
        int cy = 38;

        // Draw flowing sine wave bars
        for (int x = 0; x < SCREEN_WIDTH; x += 4)
        {
            float wave1 = sin(x * 0.08f + frame * 0.1f) * 15;
            float wave2 = sin(x * 0.12f - frame * 0.08f) * 10;
            int y1 = cy + (int)(wave1 + wave2);
            int y2 = cy - (int)(wave1 - wave2) / 2;

            // Clamp y values to screen bounds
            int y1_clamped = constrain(y1, 16, SCREEN_HEIGHT - 1);
            int y2_clamped = constrain(y2, 16, SCREEN_HEIGHT - 1);

            // Always draw - lines clipped to visible area
            display.drawLine(x, cy, x, y1_clamped, SSD1306_WHITE);
            display.drawLine(x + 2, cy, x + 2, y2_clamped, SSD1306_WHITE);
        }

        break;
    }

    case 11:
    { // Northern Lights: wavy aurora curtains
        // Draw multiple wavy vertical curtains
        for (int curtain = 0; curtain < 4; curtain++)
        {
            int baseX = 15 + curtain * 28;
            float phase = frame * 0.04f + curtain * 0.8f;

            for (int y = 18; y < SCREEN_HEIGHT - 4; y += 2)
            {
                float wave = sin(phase + y * 0.15f) * 8;
                int x = baseX + (int)wave;

                if (x > 0 && x < SCREEN_WIDTH)
                {
                    // Vary line width for depth
                    display.drawPixel(x, y, SSD1306_WHITE);
                    if ((y + frame / 3) % 6 < 4)
                    {
                        display.drawPixel(x + 1, y, SSD1306_WHITE);
                    }
                }
            }
        }

        // Twinkling stars in the background
        for (int s = 0; s < 6; s++)
        {
            if ((frame / 5 + s * 7) % 12 < 6)
            {
                int sx = (s * 23 + 10) % SCREEN_WIDTH;
                int sy = 20 + (s * 11) % 20;
                display.drawPixel(sx, sy, SSD1306_WHITE);
            }
        }

        break;
    }

    case 12:
    { // Cylon Scanner: simple bouncing dot - synced with LED position
        int dotY = 40;
        int dotRadius = 4;
        int margin = 10;

        // Get synced position (0 to 1) - same as LED strip
        unsigned long now = millis();
        float t = getCylonPosition(now);

        // Map to screen X position
        int dotX = margin + (int)(t * (SCREEN_WIDTH - 2 * margin));

        // Draw just a simple dot
        display.fillCircle(dotX, dotY, dotRadius, SSD1306_WHITE);

        break;
    }

    case 13:
    { // Heat Geek: flame logo bitmap
        // Centre the logo on screen (below header area)
        int logoX = (SCREEN_WIDTH - HEATGEEK_LOGO_WIDTH) / 2;
        int logoY = 18; // Below the mode header

        display.drawBitmap(logoX, logoY, heatGeekLogo, HEATGEEK_LOGO_WIDTH, HEATGEEK_LOGO_HEIGHT, SSD1306_WHITE);

        break;
    }
    }

    display.display();
}

// ========================
// LED PATTERN HELPERS
// ========================

// Colour wheel for rainbow LEDs
uint32_t wheel(byte pos)
{
    pos = 255 - pos;
    if (pos < 85)
    {
        return strip.Color(255 - pos * 3, 0, pos * 3);
    }
    else if (pos < 170)
    {
        pos -= 85;
        return strip.Color(0, pos * 3, 255 - pos * 3);
    }
    else
    {
        pos -= 170;
        return strip.Color(pos * 3, 255 - pos * 3, 0);
    }
}

// ========================
// LED MODES
// ========================

// 0 – Red: wave pulse travelling down with shimmer
void modeRedPulse(unsigned long now)
{
    float wavePos = now / 400.0f; // wave position

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Main travelling wave
        float wave = sin(wavePos - i * 0.15f);
        // Secondary faster shimmer
        float shimmer = sin(now / 200.0f + i * 0.4f) * 0.2f;
        // Combine waves
        float combined = (wave + shimmer + 1.2f) / 2.2f; // normalise to ~0-1
        if (combined < 0)
            combined = 0;
        if (combined > 1)
            combined = 1;

        uint8_t brightness = 30 + (uint8_t)(combined * 225);
        // Add subtle orange tint at peaks
        uint8_t green = (uint8_t)(combined * combined * 40);
        strip.setPixelColor(i, strip.Color(brightness, green, 0));
    }
    strip.show();
}

// 1 – Green: dual waves with sparkle
void modeGreenPulse(unsigned long now)
{
    float wavePos1 = now / 500.0f; // slower wave going down
    float wavePos2 = now / 350.0f; // faster wave going up

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Two opposing waves
        float wave1 = sin(wavePos1 - i * 0.12f);
        float wave2 = sin(wavePos2 + i * 0.18f) * 0.6f;
        float combined = (wave1 + wave2 + 1.6f) / 3.2f;
        if (combined < 0)
            combined = 0;
        if (combined > 1)
            combined = 1;

        uint8_t brightness = 25 + (uint8_t)(combined * 230);
        // Slight blue-green tint variation
        uint8_t blue = (uint8_t)(combined * 30);
        strip.setPixelColor(i, strip.Color(0, brightness, blue));
    }

    // Random sparkle
    if (random(100) < 20)
    {
        int sparklePos = random(NUM_LEDS);
        strip.setPixelColor(sparklePos, strip.Color(100, 255, 150));
    }

    strip.show();
}

// 2 – Candy cane: red base with chasing white sparkle dots
void modeCandyCane(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 40;

    if (now - last < interval)
        return;
    last = now;

    // Red base with slight variation
    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Gentle red wave for depth
        float wave = sin(now / 500.0f + i * 0.1f) * 0.15f + 0.85f;
        uint8_t r = (uint8_t)(255 * wave);
        strip.setPixelColor(i, strip.Color(r, 0, 0));
    }

    // Multiple white dots chasing down the strip
    int numDots = 5;
    int spacing = NUM_LEDS / numDots;

    for (int d = 0; d < numDots; d++)
    {
        // Each dot moves at slightly different phase
        int pos = ((now / 30) + d * spacing) % NUM_LEDS;

        // Bright white core
        strip.setPixelColor(pos, strip.Color(255, 255, 255));

        // Softer white trail (2 pixels behind)
        int trail1 = (pos - 1 + NUM_LEDS) % NUM_LEDS;
        int trail2 = (pos - 2 + NUM_LEDS) % NUM_LEDS;
        strip.setPixelColor(trail1, strip.Color(200, 180, 180));
        strip.setPixelColor(trail2, strip.Color(150, 100, 100));
    }

    strip.show();
}

// 3 – Multi-colour fairy lights - classic Christmas tree look
void modeTwinkle(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 100;

    // Store each LED's assigned colour (0=red, 1=green, 2=gold, 3=blue, 4=white)
    static uint8_t ledColours[NUM_LEDS];
    static bool initialised = false;

    if (!initialised)
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
            ledColours[i] = random(5);
        }
        initialised = true;
    }

    if (now - last < interval)
        return;
    last = now;

    // Occasionally reassign a colour for variety
    if (random(100) < 5)
    {
        ledColours[random(NUM_LEDS)] = random(5);
    }

    // Add new twinkles
    for (int i = 0; i < 3; i++)
    {
        int idx = random(NUM_LEDS);
        uint8_t brightness = random(180, 255);

        switch (ledColours[idx])
        {
        case 0: // Red
            strip.setPixelColor(idx, strip.Color(brightness, 0, 0));
            break;
        case 1: // Green
            strip.setPixelColor(idx, strip.Color(0, brightness, 0));
            break;
        case 2: // Gold
            strip.setPixelColor(idx, strip.Color(brightness, brightness * 0.6f, 0));
            break;
        case 3: // Blue
            strip.setPixelColor(idx, strip.Color(0, 0, brightness));
            break;
        case 4: // White
            strip.setPixelColor(idx, strip.Color(brightness, brightness, brightness));
            break;
        }
    }

    // Fade all pixels
    for (int i = 0; i < NUM_LEDS; i++)
    {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8) & 0xFF;
        uint8_t b = c & 0xFF;

        r = (r * 13) / 16; // slower fade than before
        g = (g * 13) / 16;
        b = (b * 13) / 16;

        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    strip.show();
}

// 4 – Warm gradient drift (slower)
void modeWarmGradient(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 120;

    if (now - last < interval)
        return;
    last = now;

    animIndex++;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        int pos = (i * 3 + animIndex) & 255;
        uint32_t c;

        if (pos < 85)
        {
            c = strip.Color(255, pos * 3, 0); // red -> gold
        }
        else if (pos < 170)
        {
            uint8_t p = pos - 85;
            c = strip.Color(255 - p * 3, 255, p); // warm white-ish
        }
        else
        {
            uint8_t p = pos - 170;
            c = strip.Color(255 - p * 2, 255 - p, p * 3);
        }
        strip.setPixelColor(i, c);
    }
    strip.show();
}

// 5 – Rainbow flow (slower)
void modeRainbow(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 120;

    if (now - last < interval)
        return;
    last = now;

    animIndex++;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        uint8_t pos = (i * 256 / NUM_LEDS + animIndex) & 255;
        strip.setPixelColor(i, wheel(pos));
    }
    strip.show();
}

// 6 – Meteor Shower: multi-coloured shooting stars with fading tails
void modeMeteorShower(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 25; // fast updates for smooth motion

    if (now - last < interval)
        return;
    last = now;

    // Fade all pixels
    for (int i = 0; i < NUM_LEDS; i++)
    {
        uint32_t c = strip.getPixelColor(i);
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8) & 0xFF;
        uint8_t b = c & 0xFF;

        // Fade by ~12%
        r = (r * 225) >> 8;
        g = (g * 225) >> 8;
        b = (b * 225) >> 8;

        strip.setPixelColor(i, strip.Color(r, g, b));
    }

    // Meteor data: position, speed, colour type
    static int meteorPos[5] = {0, -15, -30, -45, -60};
    static int meteorSpeed[5] = {2, 3, 2, 4, 3};
    static uint8_t meteorColour[5] = {0, 1, 2, 3, 4}; // 0=gold, 1=white, 2=blue, 3=green, 4=red

    for (int m = 0; m < 5; m++)
    {
        if (meteorPos[m] >= 0 && meteorPos[m] < NUM_LEDS)
        {
            uint8_t r, g, b;
            uint8_t r2, g2, b2; // trail colour

            switch (meteorColour[m])
            {
            case 0: // Gold
                r = 255;
                g = 200;
                b = 50;
                r2 = 255;
                g2 = 120;
                b2 = 0;
                break;
            case 1: // White
                r = 255;
                g = 255;
                b = 255;
                r2 = 180;
                g2 = 180;
                b2 = 200;
                break;
            case 2: // Blue
                r = 100;
                g = 150;
                b = 255;
                r2 = 50;
                g2 = 80;
                b2 = 200;
                break;
            case 3: // Green
                r = 50;
                g = 255;
                b = 100;
                r2 = 0;
                g2 = 180;
                b2 = 50;
                break;
            case 4: // Red
                r = 255;
                g = 80;
                b = 50;
                r2 = 200;
                g2 = 30;
                b2 = 0;
                break;
            default:
                r = 255;
                g = 255;
                b = 255;
                r2 = 150;
                g2 = 150;
                b2 = 150;
            }

            // Meteor head
            strip.setPixelColor(meteorPos[m], strip.Color(r, g, b));

            // Trail
            if (meteorPos[m] > 0)
                strip.setPixelColor(meteorPos[m] - 1, strip.Color(r2, g2, b2));
        }

        // Advance meteor
        meteorPos[m] += meteorSpeed[m];

        // Reset when off the end
        if (meteorPos[m] >= NUM_LEDS + 5)
        {
            meteorPos[m] = -random(10, 40);
            meteorSpeed[m] = random(2, 5);
            meteorColour[m] = random(5);
        }
    }

    strip.show();
}

// 7 – Fire: flickering flames effect (LED 0 = top, flames rise upward)
void modeFire(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 40;

    if (now - last < interval)
        return;
    last = now;

    // Heat array for fire simulation
    static uint8_t heat[NUM_LEDS];

    // Cool down every cell a little
    for (int i = 0; i < NUM_LEDS; i++)
    {
        int cooldown = random(0, ((55 * 10) / NUM_LEDS) + 2);
        if (cooldown > heat[i])
            heat[i] = 0;
        else
            heat[i] -= cooldown;
    }

    // Heat drifts upward (from high index to low index) and diffuses
    for (int i = 0; i < NUM_LEDS - 2; i++)
    {
        heat[i] = (heat[i + 1] + heat[i + 2] + heat[i + 2]) / 3;
    }

    // Randomly ignite new sparks near the bottom (high LED indices)
    if (random(255) < 160)
    {
        int y = NUM_LEDS - 1 - random(7); // bottom 7 LEDs
        heat[y] = heat[y] + random(160, 255);
        if (heat[y] > 255)
            heat[y] = 255;
    }

    // Convert heat to LED colours
    for (int i = 0; i < NUM_LEDS; i++)
    {
        uint8_t t192 = (heat[i] * 192) / 255;
        uint8_t heatramp = (t192 & 0x3F) << 2;

        uint8_t r, g, b;
        if (t192 > 0x80)
        { // hottest: white-yellow
            r = 255;
            g = 255;
            b = heatramp;
        }
        else if (t192 > 0x40)
        { // middle: yellow-orange
            r = 255;
            g = heatramp;
            b = 0;
        }
        else
        { // coolest: red-orange
            r = heatramp;
            g = 0;
            b = 0;
        }

        strip.setPixelColor(i, strip.Color(r, g, b));
    }

    strip.show();
}

// 8 – Sparkle Burst: builds up white then explodes in rainbow
// Shared timing for Sparkle Burst LED and OLED sync
// Returns progress 0-1 within current phase, sets isBurst to indicate which phase
const unsigned long SPARKLE_BUILD_TIME = 2000; // 2 seconds build-up
const unsigned long SPARKLE_BURST_TIME = 1500; // 1.5 second burst
const unsigned long SPARKLE_CYCLE_TIME = SPARKLE_BUILD_TIME + SPARKLE_BURST_TIME;

float getSparkleBurstPhase(unsigned long now, bool *isBurst)
{
    unsigned long phase = now % SPARKLE_CYCLE_TIME;

    if (phase < SPARKLE_BUILD_TIME)
    {
        *isBurst = false;
        return (float)phase / SPARKLE_BUILD_TIME; // 0 to 1 during build
    }
    else
    {
        *isBurst = true;
        return (float)(phase - SPARKLE_BUILD_TIME) / SPARKLE_BURST_TIME; // 0 to 1 during burst
    }
}

void modeSparkleBurst(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 20;

    if (now - last < interval)
        return;
    last = now;

    bool isBurst;
    float progress = getSparkleBurstPhase(now, &isBurst);

    if (!isBurst)
    {
        // Build-up phase: white energy gathering at centre
        int litCount = (int)(progress * (NUM_LEDS / 2));
        int centre = NUM_LEDS / 2;

        // Fade existing
        for (int i = 0; i < NUM_LEDS; i++)
        {
            uint32_t c = strip.getPixelColor(i);
            uint8_t r = ((c >> 16) & 0xFF) * 0.92f;
            uint8_t g = ((c >> 8) & 0xFF) * 0.92f;
            uint8_t b = (c & 0xFF) * 0.92f;
            strip.setPixelColor(i, strip.Color(r, g, b));
        }

        // White glow building from centre
        uint8_t brightness = 80 + (uint8_t)(progress * 175);
        for (int i = 0; i < litCount; i++)
        {
            // Intensity falls off from centre
            float falloff = 1.0f - ((float)i / (NUM_LEDS / 2)) * 0.5f;
            uint8_t b = (uint8_t)(brightness * falloff);

            if (centre + i < NUM_LEDS)
                strip.setPixelColor(centre + i, strip.Color(b, b, b));
            if (centre - i >= 0)
                strip.setPixelColor(centre - i, strip.Color(b, b, b));
        }

        // Particles rushing inward near the end
        if (progress > 0.6f)
        {
            int particleCount = (int)((progress - 0.6f) * 10);
            for (int p = 0; p < particleCount; p++)
            {
                int pos = random(NUM_LEDS);
                strip.setPixelColor(pos, strip.Color(255, 255, 255));
            }
        }
    }
    else
    {
        // Burst phase: rainbow explosion outward from centre
        int centre = NUM_LEDS / 2;

        // Fade all pixels
        for (int i = 0; i < NUM_LEDS; i++)
        {
            uint32_t c = strip.getPixelColor(i);
            uint8_t r = ((c >> 16) & 0xFF) * 0.88f;
            uint8_t g = ((c >> 8) & 0xFF) * 0.88f;
            uint8_t b = (c & 0xFF) * 0.88f;
            strip.setPixelColor(i, strip.Color(r, g, b));
        }

        // Rainbow ring expanding outward
        int ringRadius = (int)(progress * (NUM_LEDS / 2 + 10));
        int ringWidth = 8;

        for (int r = 0; r < ringWidth; r++)
        {
            int dist = ringRadius - r;
            if (dist < 0)
                continue;

            // Colour based on position in ring (rainbow)
            uint8_t hue = (r * 32) + (uint8_t)(progress * 50);
            uint32_t colour = wheel(hue);

            // Fade based on position in ring
            float fade = 1.0f - ((float)r / ringWidth);
            uint8_t cr = ((colour >> 16) & 0xFF) * fade;
            uint8_t cg = ((colour >> 8) & 0xFF) * fade;
            uint8_t cb = (colour & 0xFF) * fade;

            int posUp = centre + dist;
            int posDown = centre - dist;

            if (posUp < NUM_LEDS)
                strip.setPixelColor(posUp, strip.Color(cr, cg, cb));
            if (posDown >= 0)
                strip.setPixelColor(posDown, strip.Color(cr, cg, cb));
        }

        // Trailing sparkles
        int sparkleCount = (int)((1.0f - progress) * 6) + 1;
        for (int s = 0; s < sparkleCount; s++)
        {
            int idx = random(NUM_LEDS);
            uint32_t colour = wheel(random(256));
            strip.setPixelColor(idx, colour);
        }
    }

    strip.show();
}

// 9 – Ice Shimmer: cool blues and whites with crystalline effect
void modeIceShimmer(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 50;

    if (now - last < interval)
        return;
    last = now;

    // Base wave of cool blue
    float wavePos = now / 800.0f;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Multiple overlapping sine waves for organic shimmer
        float wave1 = sin(wavePos + i * 0.15f);
        float wave2 = sin(wavePos * 1.3f - i * 0.1f);
        float combined = (wave1 + wave2 + 2.0f) / 4.0f; // 0 to 1

        // Cool colour palette: deep blue to ice white
        uint8_t r = 20 + (combined * 100);
        uint8_t g = 60 + (combined * 150);
        uint8_t b = 150 + (combined * 105);

        strip.setPixelColor(i, strip.Color(r, g, b));
    }

    // Add occasional bright crystalline flashes
    if (random(100) < 15)
    {
        int flashPos = random(NUM_LEDS);
        int flashWidth = random(2, 5);
        for (int j = 0; j < flashWidth && flashPos + j < NUM_LEDS; j++)
        {
            strip.setPixelColor(flashPos + j, strip.Color(200, 230, 255));
        }
    }

    strip.show();
}

// Plasma Wave: hypnotic flowing colours using sine waves
void modePlasmaWave(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 30;

    if (now - last < interval)
        return;
    last = now;

    float t = now / 500.0f;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Multiple sine waves at different frequencies create plasma effect
        float v1 = sin(i * 0.1f + t);
        float v2 = sin(i * 0.15f - t * 0.7f);
        float v3 = sin((i * 0.08f + t * 0.5f) + sin(t * 0.3f));

        // Combine waves
        float plasma = (v1 + v2 + v3 + 3.0f) / 6.0f; // 0 to 1

        // Map to colour - flowing rainbow with smooth transitions
        uint8_t hue = (uint8_t)((plasma * 255.0f) + (t * 20.0f));
        uint32_t c = wheel(hue);

        strip.setPixelColor(i, c);
    }

    strip.show();
}

// Northern Lights: slow flowing greens and purples like aurora borealis
void modeNorthernLights(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 40;

    if (now - last < interval)
        return;
    last = now;

    float t = now / 2000.0f; // Slow movement

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Slow undulating waves
        float wave1 = sin(i * 0.08f + t * 2.0f) * 0.5f + 0.5f;
        float wave2 = sin(i * 0.12f - t * 1.5f + 1.0f) * 0.5f + 0.5f;
        float wave3 = sin(i * 0.05f + t * 0.8f) * 0.5f + 0.5f;

        // Aurora colours: green, teal, purple, pink
        float green = wave1 * 180;
        float blue = wave2 * 200;
        float red = wave3 * 100;

        // Add shimmering highlights
        if (random(100) < 3)
        {
            green = min(255.0f, green + 75);
            blue = min(255.0f, blue + 50);
        }

        // Intensity varies along the strip
        float intensity = (sin(i * 0.1f + t) * 0.3f + 0.7f);

        strip.setPixelColor(i, strip.Color(
                                   (uint8_t)(red * intensity),
                                   (uint8_t)(green * intensity),
                                   (uint8_t)(blue * intensity)));
    }

    strip.show();
}

// Cylon Scanner: classic red scanner sweep back and forth
// Uses time-based position so LED and OLED stay in sync
float getCylonPosition(unsigned long now)
{
    // Complete sweep cycle in ~3 seconds (1500ms each direction)
    const unsigned long cycleTime = 3000;
    unsigned long phase = now % cycleTime;

    // Triangle wave: 0 -> 1 -> 0 over the cycle
    float t;
    if (phase < cycleTime / 2)
    {
        t = (float)phase / (cycleTime / 2); // 0 to 1
    }
    else
    {
        t = 1.0f - (float)(phase - cycleTime / 2) / (cycleTime / 2); // 1 to 0
    }
    return t;
}

void modeCylonScanner(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 20;
    const int eyeWidth = 5;

    if (now - last < interval)
        return;
    last = now;

    // Clear strip
    strip.clear();

    // Get synced position (0 to 1)
    float t = getCylonPosition(now);
    float position = t * (NUM_LEDS - eyeWidth);

    int centerPos = (int)position + eyeWidth / 2;

    // Draw the eye with bright center and fading trail
    for (int i = 0; i < NUM_LEDS; i++)
    {
        float distance = abs(i - centerPos);

        if (distance < eyeWidth)
        {
            // Eye itself - bright red
            float intensity = 1.0f - (distance / eyeWidth);
            intensity = intensity * intensity; // Sharper falloff
            uint8_t r = (uint8_t)(255 * intensity);
            uint8_t g = (uint8_t)(20 * intensity);
            strip.setPixelColor(i, strip.Color(r, g, 0));
        }
        else if (distance < eyeWidth * 3)
        {
            // Fading trail
            float trailIntensity = 1.0f - ((distance - eyeWidth) / (eyeWidth * 2));
            trailIntensity = max(0.0f, trailIntensity * trailIntensity * 0.4f);
            uint8_t r = (uint8_t)(180 * trailIntensity);
            strip.setPixelColor(i, strip.Color(r, 0, 0));
        }
    }

    strip.show();
}

// Flirt mode: soft pink/red heartbeat pulses
void modeFlirt(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 30;

    if (now - last < interval)
        return;
    last = now;

    // Heartbeat pattern: two quick beats then pause
    // Full cycle is about 1.2 seconds (realistic heartbeat)
    float cyclePos = (now % 1200) / 1200.0f;
    float pulse = 0;

    if (cyclePos < 0.1f)
    {
        // First beat (0-120ms)
        pulse = sin(cyclePos / 0.1f * 3.14159f);
    }
    else if (cyclePos < 0.2f)
    {
        // Brief pause
        pulse = 0;
    }
    else if (cyclePos < 0.3f)
    {
        // Second beat (240-360ms)
        pulse = sin((cyclePos - 0.2f) / 0.1f * 3.14159f) * 0.7f;
    }
    else
    {
        // Rest period
        pulse = 0;
    }

    // Apply pulse to all LEDs with soft pink/red gradient
    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Base colour: soft pink that shifts slightly along the strip
        float posRatio = (float)i / NUM_LEDS;
        float wave = sin(now / 2000.0f + posRatio * 3.14159f) * 0.15f;

        // Base pink with pulse intensity
        uint8_t baseR = 80 + (pulse * 175);
        uint8_t baseG = 20 + (pulse * 60) + (wave * 20);
        uint8_t baseB = 40 + (pulse * 80) + (wave * 30);

        strip.setPixelColor(i, strip.Color(baseR, baseG, baseB));
    }

    // Occasional soft white sparkle for romance
    if (random(100) < 8)
    {
        int sparklePos = random(NUM_LEDS);
        strip.setPixelColor(sparklePos, strip.Color(255, 200, 220));
    }

    strip.show();
}

// Avoidant mode: amber pulse with sweeping red beacon
void modeAvoidant(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 30;

    if (now - last < interval)
        return;
    last = now;

    // Pulsing amber base
    float pulse = (sin(now / 300.0f) + 1.0f) / 2.0f; // 0 to 1
    uint8_t amberR = 180 + (uint8_t)(pulse * 75);
    uint8_t amberG = 100 + (uint8_t)(pulse * 50);

    for (int i = 0; i < NUM_LEDS; i++)
    {
        strip.setPixelColor(i, strip.Color(amberR, amberG, 0));
    }

    // Red beacon sweep - travels down the strip
    int sweepPos = (now / 20) % (NUM_LEDS + 10); // includes off-strip time

    for (int i = 0; i < 6; i++) // 6-LED wide beacon
    {
        int pos = sweepPos - i;
        if (pos >= 0 && pos < NUM_LEDS)
        {
            // Brighter in centre of beacon, fade at edges
            float intensity = 1.0f - (abs(i - 2.5f) / 3.0f);
            uint8_t r = 255;
            uint8_t g = (uint8_t)(30 * (1.0f - intensity));
            strip.setPixelColor(pos, strip.Color(r, g, 0));
        }
    }

    // Occasional full red flash for urgency
    if ((now / 1500) % 3 == 0 && (now % 1500) < 100)
    {
        for (int i = 0; i < NUM_LEDS; i++)
        {
            strip.setPixelColor(i, strip.Color(255, 0, 0));
        }
    }

    strip.show();
}

// Heat Geek mode: flowing flame gradient using brand colours
void modeHeatGeek(unsigned long now)
{
    static unsigned long last = 0;
    const unsigned long interval = 30;

    if (now - last < interval)
        return;
    last = now;

    // Heat Geek brand colours - two primary colours
    // #FF7319 Pumpkin Spice (orange) and #FC007B Neon Pink
    const float colours[][3] = {
        {255, 115, 25}, // Pumpkin Spice
        {252, 0, 123}   // Neon Pink
    };
    const int numColours = 2;

    // Slow flowing animation
    float flowOffset = now / 100.0f;

    for (int i = 0; i < NUM_LEDS; i++)
    {
        // Position in gradient (0 to 1 across the 4 colours, wrapping)
        float pos = fmod((float)i / 20.0f + flowOffset / 15.0f, 1.0f);

        // Scale to colour index range (0 to numColours)
        float scaled = pos * numColours;
        int idx1 = (int)scaled % numColours;
        int idx2 = (idx1 + 1) % numColours;
        float t = scaled - (int)scaled; // 0 to 1 blend factor

        // Linear interpolation between adjacent colours
        uint8_t r = (uint8_t)(colours[idx1][0] * (1.0f - t) + colours[idx2][0] * t);
        uint8_t g = (uint8_t)(colours[idx1][1] * (1.0f - t) + colours[idx2][1] * t);
        uint8_t b = (uint8_t)(colours[idx1][2] * (1.0f - t) + colours[idx2][2] * t);

        strip.setPixelColor(i, strip.Color(r, g, b));
    }

    strip.show();
}