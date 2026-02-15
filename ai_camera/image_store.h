#ifndef IMAGE_STORE_H
#define IMAGE_STORE_H

#include <Arduino.h>
#include "config.h"

// Initialise the PSRAM ring buffer for image storage.
void initImageStore();

// Store a JPEG image with metadata. Copies data into PSRAM.
// Returns the image index (0-based), or -1 on failure.
int storeImage(const uint8_t *jpegData, size_t jpegLength,
               uint16_t width, uint16_t height);

// Get a stored image by index (read-only). Returns nullptr if index invalid or slot empty.
const CapturedImage* getImage(int index);

// Get a mutable pointer to a stored image (for tag editing).
// Returns nullptr if index invalid or slot empty.
CapturedImage* getImageMutable(int index);

// Get the number of occupied image slots.
int getImageCount();

// Get the total capacity.
int getImageCapacity();

// Get the index of the most recently stored image, or -1 if empty.
int getLatestImageIndex();

// Delete a single image by index. Frees PSRAM.
// Returns true if the image was deleted, false if index invalid or slot empty.
bool deleteImage(int index);

// Clear all stored images and free PSRAM.
void clearImages();

#endif // IMAGE_STORE_H
