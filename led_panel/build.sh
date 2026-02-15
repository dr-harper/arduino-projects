#!/usr/bin/env bash
# Build script for LED panel firmware.
# Usage:
#   ./build.sh              # Compile only
#   ./build.sh --upload     # Compile + OTA upload to device
#   ./build.sh --upload IP  # Compile + OTA upload to specific IP

set -euo pipefail
cd "$(dirname "$0")"

FQBN="esp32:esp32:nologo_esp32c3_super_mini"
SKETCH_DIR="."
BIN="build/esp32.esp32.nologo_esp32c3_super_mini/led_panel.ino.bin"
DEFAULT_IP="${LED_PANEL_IP:-ledpanel.local}"
DEVICE_PASS="${LED_PANEL_PASS:-}"

# Step 1: Compress HTML pages
echo "==> Compressing HTML pages..."
python3 compress_html.py

# Step 2: Compile
echo "==> Compiling..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR" --export-binaries

# Step 3: Optional OTA upload
if [[ "${1:-}" == "--upload" ]]; then
    if [[ -z "$DEVICE_PASS" ]]; then
        echo "Error: Set LED_PANEL_PASS environment variable (your device password)" >&2
        exit 1
    fi
    IP="${2:-$DEFAULT_IP}"
    echo "==> OTA uploading to $IP..."

    # Login to get session cookie
    curl -s -c /tmp/esp_cookies.txt -L \
        -d "password=$DEVICE_PASS" \
        "http://$IP/login" -o /dev/null

    # Upload firmware
    RESULT=$(curl -s -b /tmp/esp_cookies.txt \
        -F "firmware=@$BIN" \
        "http://$IP/update")

    if echo "$RESULT" | grep -q "Update OK"; then
        echo "==> Upload OK — device rebooting"
    else
        echo "==> Upload failed: $RESULT" >&2
        exit 1
    fi
fi

echo "==> Done"
