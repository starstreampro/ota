# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an Arduino project for the Arduino Giga R1 WiFi that creates a comprehensive touchscreen UI system called "STAR STREAM - STREAMBOX V2". The project integrates multiple hardware components and sensors to create a full-featured interface with WiFi connectivity, GPS tracking, IMU data, and video capabilities.

## Hardware Platform

- **Target Board**: Arduino Giga R1 WiFi
- **Display**: Arduino Giga Display (480x800 touchscreen)
- **Key Components**:
  - BMI270/BMM150 IMU sensor
  - GPS module via Serial1
  - WiFi connectivity
  - Touch interface
  - Video capabilities

## Build and Development Commands

Since this is an Arduino project, compilation and upload are typically handled through the Arduino IDE or Arduino CLI:

```bash
# Compile (from Arduino IDE or CLI)
arduino-cli compile --fqbn arduino:mbed_giga:giga FullUI_with_WiFi_selector_Claude_IMU.ino

# Upload to board
arduino-cli upload -p [PORT] --fqbn arduino:mbed_giga:giga FullUI_with_WiFi_selector_Claude_IMU.ino
```

## Code Architecture

### Core Structure

The project is organized around several key architectural components:

1. **UIConstants Namespace**: Centralized constants for layout, colors, fonts, and performance settings
2. **StyleCache Class**: Performance-optimized style management for UI components
3. **MemoryManager Class**: Handles memory optimization and cleanup
4. **WiFiManager Class**: Manages WiFi connectivity and network operations
5. **State Management**: Comprehensive system state tracking via structs

### Key Data Structures

- **HardwareStatus**: Tracks availability of GPS, IMU, WiFi, and other hardware
- **SystemState**: Manages UI state, power management, and system modes
- **SensorData**: Stores GPS, IMU, and environmental sensor readings
- **UIElements**: Manages LVGL UI object references
- **PerformanceMetrics**: Tracks system performance and memory usage

### UI Framework

- **Framework**: LVGL (Light and Versatile Graphics Library)
- **Design Pattern**: Screen-based navigation with centralized style management
- **Performance**: Optimized update intervals (50 FPS UI, 20 Hz sensors)
- **Memory Management**: Automatic cleanup every 30 seconds

### Hardware Integration

- **IMU**: Arduino_BMI270_BMM150 with Madgwick AHRS filter
- **GPS**: TinyGPSPlus library for NMEA parsing
- **WiFi**: Arduino WiFi library with credential management
- **Storage**: FlashIAP and KVStore for persistent data
- **Touch**: Arduino_GigaDisplayTouch for user interaction

### Main Application Flow

1. **setup()**: Initializes all hardware components, display, touch, and UI systems
2. **loop()**: Calls optimizedLoop() for performance-optimized main execution
3. **Screen Management**: Dynamic screen creation/destruction based on user navigation
4. **Sensor Processing**: Continuous reading and filtering of IMU and GPS data
5. **Power Management**: Watchdog timer and sleep/wake functionality

## Important Files

- `FullUI_with_WiFi_selector_Claude_IMU.ino`: Main application file containing all code
- `logo.c`: Logo bitmap data for the UI
- `lv_conf.h`: LVGL configuration (referenced but not present in directory)

## Development Notes

- The entire application is contained in a single `.ino` file (~71k tokens)
- Uses modern C++ features including namespaces, classes, and constexpr
- Implements comprehensive error handling and hardware status checking
- Memory-conscious design with style caching and periodic cleanup
- Real-time sensor fusion using Madgwick filter for orientation data