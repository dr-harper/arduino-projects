#ifndef CONFIG_H
#define CONFIG_H

#define FW_VERSION "0.1.0"

// ─── Hardware ───────────────────────────────────────────────────────────────
#define BUTTON_PIN      0        // BOOT button on XIAO ESP32S3
#define LED_PIN         21       // Onboard LED on XIAO ESP32S3

// ─── Camera ─────────────────────────────────────────────────────────────────
#define CAM_FRAME_SIZE  FRAMESIZE_VGA    // 640x480 for captures
#define CAM_JPEG_QUALITY  12             // 0-63, lower = higher quality
#define CAM_FB_COUNT    2                // Frame buffers in PSRAM

// Stream uses a smaller resolution for performance
#define STREAM_FRAME_SIZE  FRAMESIZE_QVGA  // 320x240 for live stream
#define STREAM_JPEG_QUALITY  15

// ─── WiFi ───────────────────────────────────────────────────────────────────
#define AP_NAME         "AICamera-Setup"
#define AP_PASS         "aicamera"
#define MDNS_HOSTNAME   "aicamera"       // http://aicamera.local

// ─── NTP ────────────────────────────────────────────────────────────────────
#define NTP_SERVER_1    "pool.ntp.org"
#define NTP_SERVER_2    "time.google.com"
#define DEFAULT_TIMEZONE  "GMT0BST,M3.5.0/1,M10.5.0"

// ─── Image Store ────────────────────────────────────────────────────────────
#define MAX_STORED_IMAGES   10           // Ring buffer capacity
#define MAX_IMAGE_SIZE      (150 * 1024) // 150KB max per JPEG

// ─── Button ─────────────────────────────────────────────────────────────────
#define BUTTON_DEBOUNCE_MS  250          // Minimum ms between presses

// ─── Authentication ─────────────────────────────────────────────────────────
#define DEFAULT_AUTH_PASSWORD  "aicamera"
#define SESSION_TOKEN_LENGTH   16

// ─── WiFi Reconnection ─────────────────────────────────────────────────────
#define WIFI_CHECK_INTERVAL_MS  30000
#define WIFI_MAX_FAILURES       10

// ─── Classification Results ─────────────────────────────────────────────────
#define MAX_TOP_RESULTS   5
#define MAX_USER_TAGS     5
#define MAX_TAG_LENGTH    32
#define MAX_LABEL_LENGTH  64

// ─── Data Structures ────────────────────────────────────────────────────────

struct ClassResult {
    uint16_t classIndex;          // ImageNet class index (0-999)
    float    confidence;          // 0.0 - 1.0
    char     label[MAX_LABEL_LENGTH];
};

struct CapturedImage {
    uint8_t  *jpegData;           // PSRAM pointer to JPEG bytes
    size_t    jpegLength;
    uint32_t  captureTimestamp;   // millis() at capture
    char      isoTime[24];       // ISO 8601 from NTP
    uint16_t  width, height;
    ClassResult topResults[MAX_TOP_RESULTS];
    uint8_t   numResults;
    char      userTags[MAX_USER_TAGS][MAX_TAG_LENGTH];
    uint8_t   numUserTags;
    bool      tagsCorrected;      // User edited the auto-classification
    bool      occupied;           // Slot in use
};

struct AiCameraConfig {
    char     timezone[48];
    char     authPassword[32];
    uint8_t  jpegQuality;         // 0-63
    bool     autoClassify;        // Run ML on capture (Phase 2)
};

#endif // CONFIG_H
