#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

// ─── XIAO ESP32S3 Sense — OV2640 Camera Pin Mapping ────────────────────────
//
// Reference: Seeed Studio XIAO ESP32S3 Sense schematic
// Camera module directly connected to the ESP32-S3 via DVP interface.

#define PWDN_GPIO_NUM   -1    // No power-down pin
#define RESET_GPIO_NUM  -1    // No hardware reset pin
#define XCLK_GPIO_NUM   10    // Camera master clock

// SCCB (I2C-like) interface for camera register configuration
#define SIOD_GPIO_NUM   40    // SCCB data
#define SIOC_GPIO_NUM   39    // SCCB clock

// 8-bit parallel data bus (Y2 = D0, Y9 = D7)
#define Y9_GPIO_NUM     48
#define Y8_GPIO_NUM     11
#define Y7_GPIO_NUM     12
#define Y6_GPIO_NUM     14
#define Y5_GPIO_NUM     16
#define Y4_GPIO_NUM     18
#define Y3_GPIO_NUM     17
#define Y2_GPIO_NUM     15

// Sync signals
#define VSYNC_GPIO_NUM  38    // Vertical sync
#define HREF_GPIO_NUM   47    // Horizontal reference
#define PCLK_GPIO_NUM   13    // Pixel clock

#endif // CAMERA_PINS_H
