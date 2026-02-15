#ifndef ML_INFERENCE_H
#define ML_INFERENCE_H

#include <Arduino.h>
#include "config.h"

// Initialise the TFLite Micro interpreter and allocate the tensor arena in PSRAM.
// Returns true on success.
bool initInference();

// Run classification on a JPEG image buffer.
// Decodes JPEG, resizes to model input dimensions, runs inference, and
// populates the results array with top-N predictions sorted by confidence.
// Returns the number of results written (0 on failure).
uint8_t classifyImage(const uint8_t *jpegData, size_t jpegLength,
                      ClassResult *results, uint8_t maxResults);

// Returns true if the ML model is loaded and ready for inference.
bool isInferenceReady();

// Get the last inference time in milliseconds.
uint32_t getLastInferenceMs();

#endif // ML_INFERENCE_H
