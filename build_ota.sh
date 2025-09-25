#!/bin/bash

# Arduino Giga R1 WiFi OTA Build Script
# This script converts a compiled .bin file to a proper .ota file

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="$SCRIPT_DIR/ota-env"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Arduino Giga R1 WiFi OTA Build Script${NC}"
echo "=========================================="

# Check if virtual environment exists
if [ ! -d "$VENV_DIR" ]; then
    echo -e "${YELLOW}Setting up Python virtual environment...${NC}"
    python3 -m venv "$VENV_DIR"
    source "$VENV_DIR/bin/activate"
    pip install crccheck
else
    source "$VENV_DIR/bin/activate"
fi

# Function to show usage
show_usage() {
    echo "Usage: $0 <input.bin> [output.ota]"
    echo ""
    echo "Example:"
    echo "  $0 FullUI_with_WiFi_selector_Claude_IMU.ino.GIGA.bin"
    echo "  $0 FullUI_with_WiFi_selector_Claude_IMU.ino.GIGA.bin firmware.ota"
    echo ""
    echo "Steps to generate the .bin file:"
    echo "1. Open your Arduino sketch in Arduino IDE"
    echo "2. Select Arduino GIGA R1 WiFi board"
    echo "3. Go to Sketch > Export Compiled Binary"
    echo "4. Find the .bin file in your sketch directory"
    echo "5. Run this script with the .bin file as input"
    exit 1
}

# Check arguments
if [ $# -eq 0 ]; then
    show_usage
fi

INPUT_BIN="$1"
if [ $# -ge 2 ]; then
    OUTPUT_OTA="$2"
else
    # Generate output filename from input
    OUTPUT_OTA="${INPUT_BIN%.bin}.ota"
fi

# Check if input file exists
if [ ! -f "$INPUT_BIN" ]; then
    echo -e "${RED}Error: Input file '$INPUT_BIN' not found${NC}"
    echo ""
    echo "Make sure you've exported the compiled binary from Arduino IDE:"
    echo "Sketch > Export Compiled Binary"
    exit 1
fi

# Check if required tools exist
if [ ! -f "$SCRIPT_DIR/lzss.py" ] || [ ! -f "$SCRIPT_DIR/bin2ota.py" ] || [ ! -f "$SCRIPT_DIR/lzss.dylib" ]; then
    echo -e "${RED}Error: Required OTA tools not found${NC}"
    echo "Missing files: lzss.py, bin2ota.py, or lzss.dylib"
    exit 1
fi

# Generate intermediate filenames
COMPRESSED_FILE="${INPUT_BIN%.bin}.lzss"

echo -e "${YELLOW}Step 1: Compressing binary with LZSS...${NC}"
echo "Input:  $INPUT_BIN"
echo "Output: $COMPRESSED_FILE"

# Run LZSS compression
python3 "$SCRIPT_DIR/lzss.py" --encode "$INPUT_BIN" "$COMPRESSED_FILE"

if [ ! -f "$COMPRESSED_FILE" ]; then
    echo -e "${RED}Error: LZSS compression failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Compression successful${NC}"

echo -e "${YELLOW}Step 2: Creating OTA file...${NC}"
echo "Input:  $COMPRESSED_FILE"
echo "Output: $OUTPUT_OTA"

# Run bin2ota conversion for GIGA board
python3 "$SCRIPT_DIR/bin2ota.py" GIGA "$COMPRESSED_FILE" "$OUTPUT_OTA"

if [ ! -f "$OUTPUT_OTA" ]; then
    echo -e "${RED}Error: OTA file creation failed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ OTA file created successfully${NC}"

# Show file sizes
echo ""
echo "File sizes:"
echo "Original binary: $(wc -c < "$INPUT_BIN") bytes"
echo "Compressed:      $(wc -c < "$COMPRESSED_FILE") bytes"
echo "Final OTA:       $(wc -c < "$OUTPUT_OTA") bytes"

# Calculate compression ratio
ORIGINAL_SIZE=$(wc -c < "$INPUT_BIN")
COMPRESSED_SIZE=$(wc -c < "$COMPRESSED_FILE")
RATIO=$(echo "scale=1; $COMPRESSED_SIZE * 100 / $ORIGINAL_SIZE" | bc -l)

echo "Compression ratio: ${RATIO}%"

# Clean up intermediate file
rm "$COMPRESSED_FILE"

echo ""
echo -e "${GREEN}🎉 Success! OTA file ready: $OUTPUT_OTA${NC}"
echo ""
echo "Next steps:"
echo "1. Upload $OUTPUT_OTA to your GitHub repository"
echo "2. Create a version file with your version number"
echo "3. Create an MD5 checksum file"
echo "4. Test the OTA update on your Arduino Giga R1 WiFi"

# Generate version and checksum files
VERSION_FILE="${OUTPUT_OTA%.ota}.version"
CHECKSUM_FILE="${OUTPUT_OTA%.ota}.md5"

echo ""
echo -e "${YELLOW}Generating additional files...${NC}"

# Read current version from source code
CURRENT_VERSION=$(grep 'FIRMWARE_VERSION.*=' "$SCRIPT_DIR/FullUI_with_WiFi_selector_Claude_IMU.ino" | cut -d'"' -f2)
if [ -n "$CURRENT_VERSION" ]; then
    echo "$CURRENT_VERSION" > "$VERSION_FILE"
    echo -e "${GREEN}✓ Version file created: $VERSION_FILE (version: $CURRENT_VERSION)${NC}"
else
    echo "2.2.69" > "$VERSION_FILE"
    echo -e "${YELLOW}⚠ Using default version 2.2.69 in: $VERSION_FILE${NC}"
fi

# Generate MD5 checksum
MD5_HASH=$(md5 -q "$OUTPUT_OTA")
echo "$MD5_HASH" > "$CHECKSUM_FILE"
echo -e "${GREEN}✓ Checksum file created: $CHECKSUM_FILE ($MD5_HASH)${NC}"

echo ""
echo -e "${GREEN}All files ready for deployment:${NC}"
echo "📄 $OUTPUT_OTA"
echo "📄 $VERSION_FILE"
echo "📄 $CHECKSUM_FILE"