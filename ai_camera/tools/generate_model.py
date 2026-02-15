#!/usr/bin/env python3
"""
Download a pre-trained, INT8-quantised MobileNet V1 model and ImageNet labels,
then generate C header files for the AI Camera firmware.

Usage:
    python3 generate_model.py

Outputs (written to parent directory):
    model_data.h        — Quantised TFLite model as a PROGMEM C array
    classifier_labels.h — 1000 ImageNet class labels as a PROGMEM array

Requirements:
    pip install requests   (or just use urllib which is in stdlib)
"""

import os
import struct
import tarfile
import tempfile
import urllib.request

# ─── Configuration ───────────────────────────────────────────────────────────

# MobileNet V1, alpha=0.25, 128x128, INT8 quantised (~500KB)
# Smallest standard ImageNet classifier from TensorFlow model zoo
MODEL_URL = (
    "https://storage.googleapis.com/download.tensorflow.org/models/"
    "mobilenet_v1_2018_08_02/mobilenet_v1_0.25_128_quant.tgz"
)
MODEL_FILENAME = "mobilenet_v1_0.25_128_quant.tflite"

# ImageNet labels (1001 entries: background + 1000 classes)
LABELS_URL = (
    "https://storage.googleapis.com/download.tensorflow.org/data/ImageNetLabels.txt"
)

# Input dimensions for this model
INPUT_WIDTH = 128
INPUT_HEIGHT = 128
INPUT_CHANNELS = 3  # RGB

OUTPUT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ─── Download Helpers ────────────────────────────────────────────────────────


def download_model():
    """Download and extract the TFLite model from the TF model zoo."""
    print(f"Downloading model from {MODEL_URL}...")
    with tempfile.NamedTemporaryFile(suffix=".tgz", delete=False) as tmp:
        urllib.request.urlretrieve(MODEL_URL, tmp.name)
        tmp_path = tmp.name

    print("Extracting model...")
    with tarfile.open(tmp_path, "r:gz") as tar:
        # Find the .tflite file inside the archive
        for member in tar.getmembers():
            if member.name.endswith(".tflite"):
                f = tar.extractfile(member)
                if f:
                    model_bytes = f.read()
                    print(
                        f"Model extracted: {member.name} "
                        f"({len(model_bytes):,} bytes)"
                    )
                    os.unlink(tmp_path)
                    return model_bytes

    os.unlink(tmp_path)
    raise RuntimeError("No .tflite file found in archive")


def download_labels():
    """Download ImageNet class labels."""
    print(f"Downloading labels from {LABELS_URL}...")
    response = urllib.request.urlopen(LABELS_URL)
    text = response.read().decode("utf-8")
    labels = [line.strip() for line in text.strip().split("\n")]
    print(f"Downloaded {len(labels)} labels")

    # The file has 1001 entries: first is "background", rest are the 1000 classes.
    # Skip the "background" entry to get 1000 ImageNet classes.
    if labels[0].lower() == "background":
        labels = labels[1:]

    return labels[:1000]


# ─── Code Generation ─────────────────────────────────────────────────────────


def generate_model_header(model_bytes: bytes, output_path: str):
    """Generate model_data.h with the model as a PROGMEM C array."""
    print(f"Generating {output_path}...")

    with open(output_path, "w") as f:
        f.write("#ifndef MODEL_DATA_H\n")
        f.write("#define MODEL_DATA_H\n\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"// Auto-generated from {MODEL_FILENAME}\n")
        f.write(f"// Model size: {len(model_bytes):,} bytes\n")
        f.write(f"// Input: {INPUT_WIDTH}x{INPUT_HEIGHT}x{INPUT_CHANNELS} "
                f"(uint8, quantised)\n")
        f.write(f"// Output: 1001 classes (uint8, quantised)\n\n")
        f.write(f"#define MODEL_INPUT_WIDTH  {INPUT_WIDTH}\n")
        f.write(f"#define MODEL_INPUT_HEIGHT {INPUT_HEIGHT}\n")
        f.write(f"#define MODEL_INPUT_CHANNELS {INPUT_CHANNELS}\n\n")
        f.write(
            "alignas(8) const unsigned char model_data[] PROGMEM = {\n"
        )

        # Write bytes as hex, 16 per line
        for i in range(0, len(model_bytes), 16):
            chunk = model_bytes[i : i + 16]
            hex_vals = ", ".join(f"0x{b:02x}" for b in chunk)
            comma = "," if i + 16 < len(model_bytes) else ""
            f.write(f"    {hex_vals}{comma}\n")

        f.write("};\n\n")
        f.write(
            f"const unsigned int model_data_len = {len(model_bytes)};\n\n"
        )
        f.write("#endif // MODEL_DATA_H\n")

    print(f"  Written: {os.path.getsize(output_path):,} bytes")


def generate_labels_header(labels: list, output_path: str):
    """Generate classifier_labels.h with labels as a PROGMEM array."""
    print(f"Generating {output_path}...")

    with open(output_path, "w") as f:
        f.write("#ifndef CLASSIFIER_LABELS_H\n")
        f.write("#define CLASSIFIER_LABELS_H\n\n")
        f.write("#include <Arduino.h>\n\n")
        f.write(f"// Auto-generated ImageNet class labels ({len(labels)} classes)\n\n")
        f.write(f"#define NUM_CLASSES {len(labels)}\n\n")
        f.write("static const char* const CLASS_LABELS[NUM_CLASSES] PROGMEM = {\n")

        for i, label in enumerate(labels):
            # Escape any quotes in labels
            escaped = label.replace('"', '\\"')
            comma = "," if i < len(labels) - 1 else ""
            f.write(f'    "{escaped}"{comma}\n')

        f.write("};\n\n")
        f.write("#endif // CLASSIFIER_LABELS_H\n")

    print(f"  Written: {os.path.getsize(output_path):,} bytes")


# ─── Main ────────────────────────────────────────────────────────────────────


def main():
    print("=" * 60)
    print("AI Camera — Model & Labels Generator")
    print("=" * 60)
    print()

    model_bytes = download_model()
    labels = download_labels()

    model_path = os.path.join(OUTPUT_DIR, "model_data.h")
    labels_path = os.path.join(OUTPUT_DIR, "classifier_labels.h")

    generate_model_header(model_bytes, model_path)
    generate_labels_header(labels, labels_path)

    print()
    print("Done! Generated files:")
    print(f"  {model_path}")
    print(f"  {labels_path}")
    print()
    print("Next steps:")
    print("  1. Install the TFLite Micro library in Arduino IDE:")
    print("     Sketch > Include Library > Manage Libraries")
    print('     Search for "TensorFlowLite_ESP32" or "EloquentTinyML"')
    print("  2. Set board to XIAO_ESP32S3 with OPI PSRAM enabled")
    print("  3. Compile and upload the ai_camera sketch")


if __name__ == "__main__":
    main()
