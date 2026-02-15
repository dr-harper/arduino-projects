#include "image_store.h"
#include "wifi_manager_setup.h"
#include <esp_heap_caps.h>

static CapturedImage images[MAX_STORED_IMAGES];
static int nextSlot = 0;   // Next slot to write (ring buffer head)
static int imageCount = 0;

void initImageStore() {
    for (int i = 0; i < MAX_STORED_IMAGES; i++) {
        memset(&images[i], 0, sizeof(CapturedImage));
        images[i].jpegData = nullptr;
    }
    nextSlot = 0;
    imageCount = 0;

    Serial.printf("Image store initialised: %d slots, PSRAM free: %u bytes\n",
                  MAX_STORED_IMAGES, (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

int storeImage(const uint8_t *jpegData, size_t jpegLength,
               uint16_t width, uint16_t height) {
    if (!jpegData || jpegLength == 0 || jpegLength > MAX_IMAGE_SIZE) {
        Serial.println(F("Image store: invalid data or too large"));
        return -1;
    }

    CapturedImage &slot = images[nextSlot];

    // Free existing data if overwriting an occupied slot
    if (slot.occupied && slot.jpegData) {
        heap_caps_free(slot.jpegData);
        slot.jpegData = nullptr;
    }

    // Allocate PSRAM for the JPEG data
    slot.jpegData = (uint8_t *)heap_caps_malloc(jpegLength, MALLOC_CAP_SPIRAM);
    if (!slot.jpegData) {
        Serial.println(F("Image store: PSRAM allocation failed"));
        return -1;
    }

    memcpy(slot.jpegData, jpegData, jpegLength);
    slot.jpegLength = jpegLength;
    slot.captureTimestamp = millis();
    slot.width = width;
    slot.height = height;
    slot.numResults = 0;
    slot.numUserTags = 0;
    slot.tagsCorrected = false;
    slot.occupied = true;

    // Store ISO timestamp
    String iso = getIsoTimestamp();
    if (iso.length() > 0) {
        strncpy(slot.isoTime, iso.c_str(), sizeof(slot.isoTime) - 1);
        slot.isoTime[sizeof(slot.isoTime) - 1] = '\0';
    } else {
        snprintf(slot.isoTime, sizeof(slot.isoTime), "T+%lus", slot.captureTimestamp / 1000);
    }

    int stored = nextSlot;
    nextSlot = (nextSlot + 1) % MAX_STORED_IMAGES;
    if (imageCount < MAX_STORED_IMAGES) imageCount++;

    Serial.printf("Image stored: slot %d, %u bytes (%ux%u), PSRAM free: %u\n",
                  stored, (unsigned)jpegLength, width, height,
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    return stored;
}

const CapturedImage* getImage(int index) {
    if (index < 0 || index >= MAX_STORED_IMAGES) return nullptr;
    if (!images[index].occupied) return nullptr;
    return &images[index];
}

CapturedImage* getImageMutable(int index) {
    if (index < 0 || index >= MAX_STORED_IMAGES) return nullptr;
    if (!images[index].occupied) return nullptr;
    return &images[index];
}

int getImageCount() {
    return imageCount;
}

int getImageCapacity() {
    return MAX_STORED_IMAGES;
}

int getLatestImageIndex() {
    if (imageCount == 0) return -1;
    // nextSlot points to the NEXT write position, so latest is one before
    int latest = (nextSlot - 1 + MAX_STORED_IMAGES) % MAX_STORED_IMAGES;
    return images[latest].occupied ? latest : -1;
}

bool deleteImage(int index) {
    if (index < 0 || index >= MAX_STORED_IMAGES) return false;
    if (!images[index].occupied) return false;

    if (images[index].jpegData) {
        heap_caps_free(images[index].jpegData);
        images[index].jpegData = nullptr;
    }
    memset(&images[index], 0, sizeof(CapturedImage));
    images[index].jpegData = nullptr;

    // Recount occupied slots for accuracy
    imageCount = 0;
    for (int i = 0; i < MAX_STORED_IMAGES; i++) {
        if (images[i].occupied) imageCount++;
    }

    Serial.printf("Image deleted: slot %d, PSRAM free: %u\n",
                  index, (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    return true;
}

void clearImages() {
    for (int i = 0; i < MAX_STORED_IMAGES; i++) {
        if (images[i].jpegData) {
            heap_caps_free(images[i].jpegData);
        }
        memset(&images[i], 0, sizeof(CapturedImage));
        images[i].jpegData = nullptr;
    }
    nextSlot = 0;
    imageCount = 0;
    Serial.println(F("Image store cleared"));
}
