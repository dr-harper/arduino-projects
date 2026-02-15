#include "camera_manager.h"
#include "camera_pins.h"
#include "config.h"

bool initCamera() {
    camera_config_t camCfg = {};

    camCfg.ledc_channel = LEDC_CHANNEL_0;
    camCfg.ledc_timer   = LEDC_TIMER_0;
    camCfg.pin_d0       = Y2_GPIO_NUM;
    camCfg.pin_d1       = Y3_GPIO_NUM;
    camCfg.pin_d2       = Y4_GPIO_NUM;
    camCfg.pin_d3       = Y5_GPIO_NUM;
    camCfg.pin_d4       = Y6_GPIO_NUM;
    camCfg.pin_d5       = Y7_GPIO_NUM;
    camCfg.pin_d6       = Y8_GPIO_NUM;
    camCfg.pin_d7       = Y9_GPIO_NUM;
    camCfg.pin_xclk     = XCLK_GPIO_NUM;
    camCfg.pin_pclk     = PCLK_GPIO_NUM;
    camCfg.pin_vsync    = VSYNC_GPIO_NUM;
    camCfg.pin_href     = HREF_GPIO_NUM;
    camCfg.pin_sccb_sda = SIOD_GPIO_NUM;
    camCfg.pin_sccb_scl = SIOC_GPIO_NUM;
    camCfg.pin_pwdn     = PWDN_GPIO_NUM;
    camCfg.pin_reset    = RESET_GPIO_NUM;

    camCfg.xclk_freq_hz = 20000000;  // 20MHz XCLK
    camCfg.pixel_format = PIXFORMAT_JPEG;
    camCfg.grab_mode    = CAMERA_GRAB_LATEST;

    // Use PSRAM for frame buffers if available
    if (psramFound()) {
        camCfg.frame_size   = CAM_FRAME_SIZE;
        camCfg.jpeg_quality = CAM_JPEG_QUALITY;
        camCfg.fb_count     = CAM_FB_COUNT;
        camCfg.fb_location  = CAMERA_FB_IN_PSRAM;
        Serial.println(F("Camera: PSRAM detected, using PSRAM frame buffers"));
    } else {
        camCfg.frame_size   = FRAMESIZE_QVGA;
        camCfg.jpeg_quality = 20;
        camCfg.fb_count     = 1;
        camCfg.fb_location  = CAMERA_FB_IN_DRAM;
        Serial.println(F("Camera: No PSRAM, reduced settings"));
    }

    esp_err_t err = esp_camera_init(&camCfg);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", err);
        return false;
    }

    // Apply initial sensor settings
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 0);     // -2 to 2
        s->set_contrast(s, 0);       // -2 to 2
        s->set_saturation(s, 0);     // -2 to 2
        s->set_whitebal(s, 1);       // Auto white balance
        s->set_awb_gain(s, 1);       // Auto WB gain
        s->set_wb_mode(s, 0);        // Auto WB mode
        s->set_exposure_ctrl(s, 1);  // Auto exposure
        s->set_aec2(s, 0);           // DSP auto exposure
        s->set_gain_ctrl(s, 1);      // Auto gain
        s->set_hmirror(s, 0);
        s->set_vflip(s, 0);
    }

    Serial.println(F("Camera initialised"));
    return true;
}

camera_fb_t* captureFrame() {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println(F("Camera capture failed"));
        return nullptr;
    }
    return fb;
}

void releaseFrame(camera_fb_t *fb) {
    if (fb) {
        esp_camera_fb_return(fb);
    }
}

bool setCameraFrameSize(framesize_t size) {
    sensor_t *s = esp_camera_sensor_get();
    if (!s) return false;
    return s->set_framesize(s, size) == 0;
}

void setCameraJpegQuality(uint8_t quality) {
    sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_quality(s, quality);
    }
}
