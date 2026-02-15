#include "ml_inference.h"
#include "model_data.h"
#include "classifier_labels.h"
#include <esp_heap_caps.h>

// TensorFlow Lite Micro headers
// Install via Arduino Library Manager: search "TensorFlowLite_ESP32"
// or use esp-tflite-micro component
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// JPEG decoder — built into ESP32 ROM
#include "esp_camera.h"
#include "img_converters.h"

// ─── Module state ───────────────────────────────────────────────────────────

// Tensor arena — allocated in PSRAM for the larger models
#define TENSOR_ARENA_SIZE  (512 * 1024)  // 512KB — generous for MobileNet V1 0.25
static uint8_t *tensorArena = nullptr;

static const tflite::Model *model = nullptr;
static tflite::MicroInterpreter *interpreter = nullptr;
static TfLiteTensor *inputTensor = nullptr;
static TfLiteTensor *outputTensor = nullptr;

static bool inferenceReady = false;
static uint32_t lastInferenceMs = 0;

// Intermediate buffer for JPEG decode + resize (allocated in PSRAM)
static uint8_t *decodeBuffer = nullptr;
#define DECODE_BUF_SIZE  (640 * 480 * 3)  // RGB888 at VGA (matches CAM_FRAME_SIZE)

// ─── JPEG Decode Helper ─────────────────────────────────────────────────────

// Simple bilinear-ish downscale from arbitrary size to model input dimensions.
// Operates on RGB888 buffer in-place (writes to dest).
static void resizeRgb(const uint8_t *src, int srcW, int srcH,
                      uint8_t *dst, int dstW, int dstH) {
    for (int dy = 0; dy < dstH; dy++) {
        int sy = (dy * srcH) / dstH;
        if (sy >= srcH) sy = srcH - 1;
        for (int dx = 0; dx < dstW; dx++) {
            int sx = (dx * srcW) / dstW;
            if (sx >= srcW) sx = srcW - 1;
            int srcIdx = (sy * srcW + sx) * 3;
            int dstIdx = (dy * dstW + dx) * 3;
            dst[dstIdx]     = src[srcIdx];
            dst[dstIdx + 1] = src[srcIdx + 1];
            dst[dstIdx + 2] = src[srcIdx + 2];
        }
    }
}

// ─── Public API ─────────────────────────────────────────────────────────────

bool initInference() {
    Serial.println(F("ML: Initialising inference engine..."));

    // Allocate tensor arena in PSRAM
    tensorArena = (uint8_t *)heap_caps_malloc(TENSOR_ARENA_SIZE, MALLOC_CAP_SPIRAM);
    if (!tensorArena) {
        Serial.println(F("ML: Failed to allocate tensor arena in PSRAM"));
        return false;
    }
    Serial.printf("ML: Tensor arena allocated: %d KB in PSRAM\n", TENSOR_ARENA_SIZE / 1024);

    // Allocate decode buffer in PSRAM
    decodeBuffer = (uint8_t *)heap_caps_malloc(DECODE_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (!decodeBuffer) {
        Serial.println(F("ML: Failed to allocate decode buffer"));
        heap_caps_free(tensorArena);
        tensorArena = nullptr;
        return false;
    }

    // Load model
    model = tflite::GetModel(model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("ML: Model schema version mismatch: %lu vs %d\n",
                      model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }
    Serial.printf("ML: Model loaded (%u bytes)\n", model_data_len);

    // Create interpreter with all ops (simpler than selecting individual ops)
    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter staticInterpreter(
        model, resolver, tensorArena, TENSOR_ARENA_SIZE);
    interpreter = &staticInterpreter;

    // Allocate tensors
    TfLiteStatus status = interpreter->AllocateTensors();
    if (status != kTfLiteOk) {
        Serial.println(F("ML: AllocateTensors() failed"));
        return false;
    }

    inputTensor = interpreter->input(0);
    outputTensor = interpreter->output(0);

    Serial.printf("ML: Input tensor: [%d, %d, %d, %d] type=%d\n",
                  inputTensor->dims->data[0],
                  inputTensor->dims->data[1],
                  inputTensor->dims->data[2],
                  inputTensor->dims->data[3],
                  inputTensor->type);
    Serial.printf("ML: Output tensor: [%d, %d] type=%d\n",
                  outputTensor->dims->data[0],
                  outputTensor->dims->data[1],
                  outputTensor->type);

    size_t usedBytes = interpreter->arena_used_bytes();
    Serial.printf("ML: Arena used: %u / %u bytes (%.0f%%)\n",
                  (unsigned)usedBytes, TENSOR_ARENA_SIZE,
                  100.0f * usedBytes / TENSOR_ARENA_SIZE);

    inferenceReady = true;
    Serial.println(F("ML: Inference engine ready"));
    return true;
}

uint8_t classifyImage(const uint8_t *jpegData, size_t jpegLength,
                      ClassResult *results, uint8_t maxResults) {
    if (!inferenceReady || !jpegData || !results || maxResults == 0) {
        return 0;
    }

    unsigned long startMs = millis();

    // 1. Decode JPEG to RGB888
    // The esp_camera jpg2rgb888 function decodes JPEG to RGB888
    size_t outLen = 0;
    bool decoded = fmt2rgb888(jpegData, jpegLength, PIXFORMAT_JPEG, decodeBuffer);
    if (!decoded) {
        Serial.println(F("ML: JPEG decode failed"));
        return 0;
    }

    // Determine source dimensions from JPEG header
    // Simple JPEG SOF0 marker parsing for width/height
    int srcW = 0, srcH = 0;
    for (size_t i = 0; i + 8 < jpegLength; i++) {
        if (jpegData[i] == 0xFF && jpegData[i + 1] == 0xC0) {
            srcH = (jpegData[i + 5] << 8) | jpegData[i + 6];
            srcW = (jpegData[i + 7] << 8) | jpegData[i + 8];
            break;
        }
    }
    if (srcW == 0 || srcH == 0) {
        // Fallback — assume QVGA if we can't parse
        srcW = 320;
        srcH = 240;
    }

    // 2. Resize to model input dimensions
    int modelW = MODEL_INPUT_WIDTH;
    int modelH = MODEL_INPUT_HEIGHT;
    int modelC = MODEL_INPUT_CHANNELS;

    // Resize into the input tensor directly if dimensions differ
    uint8_t *inputData = inputTensor->data.uint8;

    if (modelC == 3) {
        // RGB input — resize directly into input tensor
        resizeRgb(decodeBuffer, srcW, srcH, inputData, modelW, modelH);
    } else if (modelC == 1) {
        // Grayscale input — resize RGB to end of decode buffer, then convert
        size_t resizedSize = (size_t)modelW * modelH * 3;
        uint8_t *tempResized = decodeBuffer + DECODE_BUF_SIZE - resizedSize;
        resizeRgb(decodeBuffer, srcW, srcH, tempResized, modelW, modelH);
        for (int i = 0; i < modelW * modelH; i++) {
            int r = tempResized[i * 3];
            int g = tempResized[i * 3 + 1];
            int b = tempResized[i * 3 + 2];
            inputData[i] = (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
        }
    }

    // 3. Run inference
    TfLiteStatus status = interpreter->Invoke();
    if (status != kTfLiteOk) {
        Serial.println(F("ML: Inference failed"));
        return 0;
    }

    lastInferenceMs = millis() - startMs;

    // 4. Parse output — find top N results
    int numClasses = outputTensor->dims->data[1];
    if (numClasses > NUM_CLASSES) numClasses = NUM_CLASSES;

    // For quantised models, output is uint8 with zero_point and scale
    float scale = outputTensor->params.scale;
    int32_t zeroPoint = outputTensor->params.zero_point;

    // Simple top-N selection (insertion sort into results array)
    uint8_t numResults = 0;
    for (int i = 0; i < maxResults && i < MAX_TOP_RESULTS; i++) {
        results[i].confidence = -1.0f;
        results[i].classIndex = 0;
    }

    for (int c = 0; c < numClasses; c++) {
        float conf;
        if (outputTensor->type == kTfLiteUInt8) {
            conf = (outputTensor->data.uint8[c] - zeroPoint) * scale;
        } else if (outputTensor->type == kTfLiteInt8) {
            conf = (outputTensor->data.int8[c] - zeroPoint) * scale;
        } else if (outputTensor->type == kTfLiteFloat32) {
            conf = outputTensor->data.f[c];
        } else {
            continue;
        }

        // Insert into sorted results if it belongs in top N
        for (int r = 0; r < maxResults && r < MAX_TOP_RESULTS; r++) {
            if (conf > results[r].confidence) {
                // Shift lower results down
                for (int s = maxResults - 1; s > r; s--) {
                    if (s < MAX_TOP_RESULTS) {
                        results[s] = results[s - 1];
                    }
                }
                results[r].classIndex = c;
                results[r].confidence = conf;
                // Copy label from PROGMEM
                const char *label = (const char *)pgm_read_ptr(&CLASS_LABELS[c]);
                strncpy(results[r].label, label, MAX_LABEL_LENGTH - 1);
                results[r].label[MAX_LABEL_LENGTH - 1] = '\0';
                if (numResults < maxResults) numResults++;
                break;
            }
        }
    }

    Serial.printf("ML: Classification done in %lu ms — top: %s (%.1f%%)\n",
                  lastInferenceMs,
                  numResults > 0 ? results[0].label : "none",
                  numResults > 0 ? results[0].confidence * 100.0f : 0.0f);

    return numResults;
}

bool isInferenceReady() {
    return inferenceReady;
}

uint32_t getLastInferenceMs() {
    return lastInferenceMs;
}
