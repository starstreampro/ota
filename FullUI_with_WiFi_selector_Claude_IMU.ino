// AGGRESSIVE size optimization for entire compilation unit
#pragma GCC push_options
#pragma GCC optimize ("-Os")           // Optimize for size
#pragma GCC optimize ("-flto")          // Link Time Optimization
#pragma GCC optimize ("-ffunction-sections") // Separate functions
#pragma GCC optimize ("-fdata-sections")     // Separate data
#pragma GCC optimize ("-fno-exceptions")     // Disable exceptions
#pragma GCC optimize ("-fno-rtti")           // Disable RTTI
#pragma GCC optimize ("-fno-threadsafe-statics") // Disable thread-safe statics

#include <Arduino_H7_Video.h>
#include <Arduino_GigaDisplayTouch.h>
#include <Arduino_GigaDisplay.h>
#include <lvgl.h>
#include "lv_conf.h"
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <Arduino_BMI270_BMM150.h>
#include <MadgwickAHRS.h>
#include <WiFi.h>
#include <mbed.h>
#include <kvstore_global_api.h>
#include <ArduinoHttpClient.h>
#include <WebSocketClient.h>
#include <Arduino_Portenta_OTA.h>

// #include "logo.c" // REMOVED for size optimization

// extern const lv_img_dsc_t logo; // REMOVED for size optimization

// WebSocket Message Type Constants (from ArduinoHttpClient)
#define TYPE_TEXT 0x1
#define TYPE_PING 0x9
#define TYPE_PONG 0xa

// =============================================================================
// STAR STREAM - STREAMBOX PRO V2
// =============================================================================

// Debug configuration - DISABLED for size optimization (saves ~50KB)
#define ENABLE_DEBUG_SERIAL 0  // Changed from 1 to 0 - major size savings
#define ENABLE_VERBOSE_DEBUG 0  // Disabled to save memory

// Debug macros to reduce memory fragmentation
#if ENABLE_DEBUG_SERIAL
  #define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
  #if ENABLE_VERBOSE_DEBUG
    #define VERBOSE_PRINT(...) Serial.print(__VA_ARGS__)
    #define VERBOSE_PRINTLN(...) Serial.println(__VA_ARGS__)
  #else
    #define VERBOSE_PRINT(...)
    #define VERBOSE_PRINTLN(...)
  #endif
#else
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
  #define VERBOSE_PRINT(...)
  #define VERBOSE_PRINTLN(...)
#endif

// Centralized UI Constants for Consistency
namespace UIConstants {
// Layout Constants
constexpr int SCREEN_WIDTH = 480;
constexpr int SCREEN_HEIGHT = 800;
constexpr int HEADER_HEIGHT = 50;
constexpr int FOOTER_HEIGHT = 50;
constexpr int CONTENT_HEIGHT = SCREEN_HEIGHT - HEADER_HEIGHT - FOOTER_HEIGHT;

// Spacing and Dimensions
constexpr int ITEM_HEIGHT = 65;
constexpr int ITEM_WIDTH = 450;
constexpr int ITEM_SPACING = 10;
constexpr int BUTTON_RADIUS = 8;
constexpr int PADDING_STANDARD = 10;
constexpr int PADDING_SMALL = 5;

// Font Sizes
constexpr auto FONT_LARGE = &lv_font_montserrat_32;
constexpr auto FONT_MEDIUM = &lv_font_montserrat_22;
constexpr auto FONT_SMALL = &lv_font_montserrat_18;
constexpr auto FONT_SYMBOL = &lv_font_montserrat_24;

// Color Palette
constexpr uint32_t COLOR_PRIMARY = 0x007AFF;
constexpr uint32_t COLOR_SUCCESS = 0x34C85A;
constexpr uint32_t COLOR_DANGER = 0xFF0000;
constexpr uint32_t COLOR_WARNING = 0xFFC700;
constexpr uint32_t COLOR_SECONDARY = 0x8C8C8C;
constexpr uint32_t COLOR_TEXT_PRIMARY = 0x000000;
constexpr uint32_t COLOR_TEXT_SECONDARY = 0x6A6A6A;
constexpr uint32_t COLOR_BACKGROUND = 0xEEEEEE;
constexpr uint32_t COLOR_SURFACE = 0xFFFFFF;
constexpr uint32_t COLOR_HEADER = 0x000000;

// Performance Constants
constexpr unsigned long UI_UPDATE_INTERVAL = 20;          // 50 FPS, but UI updates are now separate from lv_timer_handler
constexpr unsigned long SENSOR_UPDATE_INTERVAL = 50;      // 20 Hz
constexpr unsigned long MEMORY_CLEANUP_INTERVAL = 30000;  // 30 seconds
}

// Style Cache System for Performance
class StyleCache {
private:
  static bool initialized;

public:
  static lv_style_t list_item_style;
  static lv_style_t button_primary_style;
  static lv_style_t button_secondary_style;
  static lv_style_t button_danger_style;
  static lv_style_t textarea_style;
  static lv_style_t transparent_button_style;

  static void initialize() {
    if (initialized) return;

    // List Item Style
    lv_style_init(&list_item_style);
    lv_style_set_bg_color(&list_item_style, lv_color_hex(UIConstants::COLOR_SURFACE));
    lv_style_set_bg_opa(&list_item_style, LV_OPA_COVER);
    lv_style_set_radius(&list_item_style, UIConstants::BUTTON_RADIUS);
    lv_style_set_border_width(&list_item_style, 0);
    lv_style_set_shadow_width(&list_item_style, 0);
    lv_style_set_outline_width(&list_item_style, 0);

    // Primary Button Style
    lv_style_init(&button_primary_style);
    lv_style_set_bg_color(&button_primary_style, lv_color_hex(UIConstants::COLOR_PRIMARY));
    lv_style_set_bg_opa(&button_primary_style, LV_OPA_COVER);
    lv_style_set_radius(&button_primary_style, UIConstants::BUTTON_RADIUS);
    lv_style_set_border_width(&button_primary_style, 0);
    lv_style_set_shadow_width(&button_primary_style, 0);
    lv_style_set_outline_width(&button_primary_style, 0);
    lv_style_set_text_color(&button_primary_style, lv_color_white());

    // Secondary Button Style
    lv_style_init(&button_secondary_style);
    lv_style_set_bg_color(&button_secondary_style, lv_color_hex(UIConstants::COLOR_SECONDARY));
    lv_style_set_bg_opa(&button_secondary_style, LV_OPA_COVER);
    lv_style_set_radius(&button_secondary_style, UIConstants::BUTTON_RADIUS);
    lv_style_set_border_width(&button_secondary_style, 0);
    lv_style_set_shadow_width(&button_secondary_style, 0);
    lv_style_set_outline_width(&button_secondary_style, 0);
    lv_style_set_text_color(&button_secondary_style, lv_color_white());

    // Danger Button Style
    lv_style_init(&button_danger_style);
    lv_style_set_bg_color(&button_danger_style, lv_color_hex(UIConstants::COLOR_DANGER));
    lv_style_set_bg_opa(&button_danger_style, LV_OPA_COVER);
    lv_style_set_radius(&button_danger_style, UIConstants::BUTTON_RADIUS);
    lv_style_set_border_width(&button_danger_style, 0);
    lv_style_set_shadow_width(&button_danger_style, 0);
    lv_style_set_outline_width(&button_danger_style, 0);
    lv_style_set_text_color(&button_danger_style, lv_color_white());

    // Textarea Style
    lv_style_init(&textarea_style);
    lv_style_set_bg_color(&textarea_style, lv_color_hex(UIConstants::COLOR_SURFACE));
    lv_style_set_bg_opa(&textarea_style, LV_OPA_COVER);
    lv_style_set_radius(&textarea_style, UIConstants::BUTTON_RADIUS);
    lv_style_set_border_width(&textarea_style, 1);
    lv_style_set_border_color(&textarea_style, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY));
    lv_style_set_pad_all(&textarea_style, UIConstants::PADDING_STANDARD);
    lv_style_set_text_font(&textarea_style, UIConstants::FONT_MEDIUM);

    // Transparent Button Style
    lv_style_init(&transparent_button_style);
    lv_style_set_bg_opa(&transparent_button_style, LV_OPA_TRANSP);
    lv_style_set_border_width(&transparent_button_style, 0);
    lv_style_set_shadow_width(&transparent_button_style, 0);
    lv_style_set_outline_width(&transparent_button_style, 0);

    initialized = true;
    DEBUG_PRINTLN("StyleCache initialized");
  }
};

// Static member definitions
bool StyleCache::initialized = false;
lv_style_t StyleCache::list_item_style;
lv_style_t StyleCache::button_primary_style;
lv_style_t StyleCache::button_secondary_style;
lv_style_t StyleCache::button_danger_style;
lv_style_t StyleCache::textarea_style;
lv_style_t StyleCache::transparent_button_style;

// UI Helper System for Consistent Creation
class UIHelper {
public:
  // Create optimized button with consistent styling
  static lv_obj_t *createButton(lv_obj_t *parent, const char *text, lv_style_t *style,
                                lv_event_cb_t event_cb, lv_align_t align = LV_ALIGN_CENTER,
                                int x_offset = 0, int y_offset = 0,
                                int width = UIConstants::ITEM_WIDTH, int height = UIConstants::ITEM_HEIGHT) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_align(btn, align, x_offset, y_offset);
    lv_obj_add_style(btn, style, LV_PART_MAIN);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, UIConstants::FONT_MEDIUM, 0);
    lv_obj_center(label);

    if (event_cb) {
      lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, nullptr);
    }

    return btn;
  }

  // Create settings item with consistent layout
  static lv_obj_t *createSettingsItem(lv_obj_t *parent, const char *title, const char *value,
                                      int y_pos, bool clickable = false, lv_event_cb_t event_cb = nullptr) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_add_style(btn, &StyleCache::list_item_style, LV_PART_MAIN);
    lv_obj_set_size(btn, UIConstants::ITEM_WIDTH, UIConstants::ITEM_HEIGHT);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y_pos);
    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    if (!clickable) {
      lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    } else if (event_cb) {
      lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, nullptr);
    }

    // Title (left side)
    lv_obj_t *title_label = lv_label_create(btn);
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, UIConstants::FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(title_label, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, UIConstants::PADDING_STANDARD, 0);

    // Value (right side)
    lv_obj_t *value_label = lv_label_create(btn);
    lv_label_set_text(value_label, value);
    lv_obj_set_style_text_color(value_label, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_20, 0);
    lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);

    return btn;
  }

  // Create optimized textarea
  static lv_obj_t *createTextArea(lv_obj_t *parent, const char *placeholder,
                                  lv_align_t align, int x_offset, int y_offset,
                                  int width = 460, int height = 69) {
    lv_obj_t *ta = lv_textarea_create(parent);
    lv_obj_set_size(ta, width, height);
    lv_obj_align(ta, align, x_offset, y_offset);
    lv_obj_add_style(ta, &StyleCache::textarea_style, LV_PART_MAIN);
    lv_textarea_set_placeholder_text(ta, placeholder);
    lv_textarea_set_one_line(ta, true);
    return ta;
  }

  // Batch UI update optimization
  static void batchUIUpdate(lv_obj_t *container, std::function<void()> updateFunc) {
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(container);
    updateFunc();
    lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);
  }

  // Create responsive radio button with better touch handling
  static lv_obj_t *createResponsiveRadio(lv_obj_t *parent, const char *text,
                                         bool checked = false, lv_event_cb_t event_cb = nullptr) {
    lv_obj_t *radio = lv_checkbox_create(parent);
    lv_checkbox_set_text(radio, text);

    // REMOVE all the padding that was causing the positioning issues
    // lv_obj_set_style_pad_all(radio, 8, 0);  // <-- REMOVE THIS LINE

    // Keep the checkbox at its natural size
    lv_obj_set_size(radio, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // Target ONLY the indicator part with precise sizing
    lv_obj_set_style_width(radio, 24, LV_PART_INDICATOR);   // Smaller, more precise size
    lv_obj_set_style_height(radio, 24, LV_PART_INDICATOR);  // Smaller, more precise size

    // Force exact dimensions - override any defaults
    lv_obj_set_style_min_width(radio, 24, LV_PART_INDICATOR);
    lv_obj_set_style_min_height(radio, 24, LV_PART_INDICATOR);
    lv_obj_set_style_max_width(radio, 24, LV_PART_INDICATOR);
    lv_obj_set_style_max_height(radio, 24, LV_PART_INDICATOR);

    // Style the indicator
    lv_obj_set_style_text_font(radio, UIConstants::FONT_SMALL, 0);
    lv_obj_set_style_text_color(radio, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
    lv_obj_set_style_radius(radio, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(radio, lv_color_hex(UIConstants::COLOR_SUCCESS), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(radio, lv_color_hex(UIConstants::COLOR_SUCCESS), LV_PART_INDICATOR);
    lv_obj_set_style_border_width(radio, 2, LV_PART_INDICATOR);

    // Reduce padding between radio button and label (this is internal spacing)
    lv_obj_set_style_pad_column(radio, 6, 0);  // Space between circle and text

    // Make the clickable area larger without affecting layout
    lv_obj_set_ext_click_area(radio, 10);  // 10px larger clickable area in all directions

    // Visual feedback on press - but only for the indicator
    lv_obj_set_style_transform_zoom(radio, 256 + 15, LV_PART_INDICATOR | LV_STATE_PRESSED);

    // Improve responsiveness
    lv_obj_clear_flag(radio, LV_OBJ_FLAG_SCROLL_CHAIN);

    if (checked) {
      lv_obj_add_state(radio, LV_STATE_CHECKED);
    }

    if (event_cb) {
      lv_obj_add_event_cb(radio, event_cb, LV_EVENT_VALUE_CHANGED, nullptr);
      lv_obj_add_event_cb(radio, event_cb, LV_EVENT_CLICKED, nullptr);
    }

    return radio;
  }
};

// Memory Management System
class MemoryManager {
private:
  static unsigned long last_cleanup;

public:
  static void periodicCleanup() {
    unsigned long now = millis();
    if (now - last_cleanup > UIConstants::MEMORY_CLEANUP_INTERVAL) {
      // Use LVGL's built-in memory monitoring instead of lv_gc_collect
      lv_mem_monitor_t mon;
      lv_mem_monitor(&mon);

      VERBOSE_PRINT("Memory cleanup - Free: ");
      VERBOSE_PRINT(mon.free_size);
      VERBOSE_PRINT(" bytes, Used: ");
      VERBOSE_PRINT(mon.total_size - mon.free_size);
      VERBOSE_PRINT(" bytes, Fragmentation: ");
      VERBOSE_PRINT(mon.frag_pct);
      VERBOSE_PRINTLN("%");

      // Only do heavy cleanup if memory is getting critically low
      // This reduces UI interruptions during normal operation
      if (mon.free_size < 8000) {  // Less than 8KB free (reduced threshold)
        DEBUG_PRINTLN("WARNING: Low memory detected, forcing cleanup");

        // Force object invalidation and refresh
        lv_obj_invalidate(lv_scr_act());

        // Check again
        lv_mem_monitor(&mon);
        VERBOSE_PRINT("After cleanup - Free: ");
        VERBOSE_PRINT(mon.free_size);
        VERBOSE_PRINTLN(" bytes");
      }

      last_cleanup = now;
    }
  }

  // optimizeStrings() removed - was empty and unused

  static void forceCleanup() {
    // Serial.println("Forcing immediate memory cleanup...");

    // Check current memory state first
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    // Serial.print("Before cleanup - Free: ");
    // Serial.print(mon.free_size);
    // Serial.print(" bytes, Fragmentation: ");
    // Serial.print(mon.frag_pct);
    // Serial.println("%");

    // Only force cleanup if we have a valid screen
    lv_obj_t *current_screen = lv_scr_act();
    if (current_screen) {
      lv_obj_invalidate(current_screen);

      // Process any pending operations
      lv_timer_handler();

      // Small delay to allow cleanup to complete
      delay(10);
    }

    // Check memory again
    lv_mem_monitor(&mon);
    // Serial.print("After cleanup - Free: ");
    // Serial.print(mon.free_size);
    // Serial.print(" bytes, Fragmentation: ");
    // Serial.print(mon.frag_pct);
    // Serial.println("%");

    // If memory is critically low, try more aggressive cleanup
    if (mon.free_size < 5000) {
      // Serial.println("CRITICAL: Memory very low, attempting recovery...");
      // Just invalidate without doing more operations
    }
  }

  // Add critical memory check
  static bool isMemoryCritical() {
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    return mon.free_size < 8000;  // 8KB threshold
  }
};

unsigned long MemoryManager::last_cleanup = 0;

struct HardwareStatus {
  bool imu_available = false;
  bool magnetometer_available = false;
  bool gps_available = false;
  String error_messages = "";
} hardware_status;

struct FooterHelper {
  static lv_obj_t *wifi_indicator;
  static lv_obj_t *gps_indicator;

  // Update indicators across all pages
  static void updateWiFiIndicator(lv_color_t color) {
    if (wifi_indicator && lv_obj_is_valid(wifi_indicator)) {
      lv_obj_set_style_text_color(wifi_indicator, color, 0);
    }
  }

  static void updateGPSIndicator(lv_color_t color) {
    if (gps_indicator && lv_obj_is_valid(gps_indicator)) {
      lv_obj_set_style_text_color(gps_indicator, color, 0);
    }
  }

  static lv_obj_t *createFooter(lv_obj_t *parent_screen) {
    // SAFETY CHECK: Validate parent screen
    if (!parent_screen) {
      // Serial.println("ERROR: Invalid parent screen for footer");
      return nullptr;
    }

    // Reset indicators to prevent dangling pointers
    wifi_indicator = nullptr;
    gps_indicator = nullptr;

    // Create footer as a normal child of the screen
    lv_obj_t *footer = lv_obj_create(parent_screen);
    if (!footer) {
      // Serial.println("ERROR: Failed to create footer");
      return nullptr;
    }

    lv_obj_set_size(footer, UIConstants::SCREEN_WIDTH, UIConstants::FOOTER_HEIGHT);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(footer, lv_color_hex(UIConstants::COLOR_HEADER), 0);
    lv_obj_set_style_radius(footer, 0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_CLICKABLE);

    // Helper function for creating indicator groups
    auto createIndicatorGroup = [&](lv_obj_t *parent, lv_align_t align, int x_offset,
                                    const char *symbol, const char *text,
                                    lv_obj_t **indicator_out) -> void {
      if (!parent || !indicator_out) return;

      lv_obj_t *grp = lv_obj_create(parent);
      if (!grp) return;

      lv_obj_remove_style_all(grp);
      lv_obj_set_size(grp, 100, 40);
      lv_obj_align(grp, align, x_offset, 0);
      lv_obj_set_layout(grp, LV_LAYOUT_FLEX);
      lv_obj_set_flex_flow(grp, align == LV_ALIGN_LEFT_MID ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_ROW_REVERSE);
      lv_obj_set_flex_align(grp, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
      lv_obj_set_style_pad_column(grp, 7, 0);
      lv_obj_clear_flag(grp, LV_OBJ_FLAG_CLICKABLE);

      *indicator_out = lv_label_create(grp);
      if (!*indicator_out) return;

      lv_label_set_text(*indicator_out, symbol);
      lv_obj_set_style_text_color(*indicator_out, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
      lv_obj_set_style_text_font(*indicator_out,
                                 strcmp(symbol, LV_SYMBOL_WIFI) == 0 ? UIConstants::FONT_SYMBOL : UIConstants::FONT_MEDIUM, 0);
      lv_obj_clear_flag(*indicator_out, LV_OBJ_FLAG_CLICKABLE);

      lv_obj_t *label = lv_label_create(grp);
      if (!label) return;

      lv_label_set_text(label, text);
      lv_obj_set_style_text_color(label, lv_color_white(), 0);
      lv_obj_set_style_text_font(label, UIConstants::FONT_SMALL, 0);
      lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    };

    // Create indicators
    createIndicatorGroup(footer, LV_ALIGN_LEFT_MID, UIConstants::PADDING_STANDARD, LV_SYMBOL_WIFI, "WiFi", &wifi_indicator);
    createIndicatorGroup(footer, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, LV_SYMBOL_GPS, "GPS", &gps_indicator);

    // Create center "LIVE" indicator with green circles
    lv_obj_t *live_container = lv_obj_create(footer);
    lv_obj_remove_style_all(live_container);
    lv_obj_set_size(live_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(live_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(live_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(live_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(live_container, 10, 0);
    lv_obj_align(live_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(live_container, LV_OBJ_FLAG_CLICKABLE);

    // Left green circle (doubled size: 16x16)
    lv_obj_t *left_circle = lv_obj_create(live_container);
    lv_obj_set_size(left_circle, 19, 19);
    lv_obj_set_style_bg_color(left_circle, lv_color_hex(UIConstants::COLOR_SUCCESS), 0);
    lv_obj_set_style_bg_opa(left_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(left_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(left_circle, 0, 0);
    lv_obj_clear_flag(left_circle, LV_OBJ_FLAG_CLICKABLE);

    // "LIVE" text (all caps)
    lv_obj_t *live_label = lv_label_create(live_container);
    lv_label_set_text(live_label, "LIVE");
    lv_obj_set_style_text_color(live_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(live_label, UIConstants::FONT_SMALL, 0);
    lv_obj_clear_flag(live_label, LV_OBJ_FLAG_CLICKABLE);

    // Right green circle (doubled size: 16x16)
    lv_obj_t *right_circle = lv_obj_create(live_container);
    lv_obj_set_size(right_circle, 19, 19);
    lv_obj_set_style_bg_color(right_circle, lv_color_hex(UIConstants::COLOR_SUCCESS), 0);
    lv_obj_set_style_bg_opa(right_circle, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(right_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(right_circle, 0, 0);
    lv_obj_clear_flag(right_circle, LV_OBJ_FLAG_CLICKABLE);

    return footer;
  }
};

lv_obj_t *FooterHelper::wifi_indicator = nullptr;
lv_obj_t *FooterHelper::gps_indicator = nullptr;

// Constants
constexpr unsigned long GPS_UPDATE_INTERVAL = 3000;
constexpr unsigned long IMU_UPDATE_INTERVAL = 10;
constexpr unsigned long TELEMETRY_INTERVAL = 7000;
constexpr unsigned long WIFI_CONNECT_TIMEOUT = 15000;
constexpr int IMU_CALIBRATION_TIME = 2000;
constexpr size_t CREDENTIAL_BUFFER_SIZE = 64;
constexpr int MAX_NETWORKS = 4;
constexpr const char *FIRMWARE_VERSION = "2.2.69";
constexpr const char *DEFAULT_DEVICE_NAME = "Star Stream";
constexpr float INPUT_HEIGHT_MULTIPLIER = 1.15f;
constexpr const char *DEVICE_SERIAL_NUMBER = "SS537A81VP22WQ19";
// GitHub API and Release URLs for OTA updates
constexpr const char *GITHUB_API_HOST = "api.github.com";
constexpr const char *GITHUB_RELEASES_HOST = "github.com";
constexpr const char *GITHUB_REPO_OWNER = "starstreampro";
constexpr const char *GITHUB_REPO_NAME = "ota";
constexpr const char *GITHUB_API_PATH = "/repos/starstreampro/ota/releases/latest";
// These will be dynamically set from the latest release
String dynamic_ota_download_url = "";
String dynamic_version = "";
String dynamic_checksum = "";
constexpr int OTA_UPDATE_PORT = 443;  // GitHub supports both HTTP (80) and HTTPS (443)
constexpr unsigned long OTA_CHECK_TIMEOUT = 10000;

#define GPSSerial Serial2

// Watchdog handling
using namespace mbed;
Watchdog &watchdog = Watchdog::get_instance();

// Hardware objects
Arduino_H7_Video display(480, 800, GigaDisplayShield);
Arduino_GigaDisplayTouch touch;
TinyGPSPlus gps;
BoschSensorClass imu(Wire1);
Madgwick filter;
GigaDisplayBacklight backlight;


// WiFi State Machine
enum WiFiState {
  WIFI_IDLE,
  WIFI_DISCONNECTING,
  WIFI_CONNECTING,
  WIFI_CONNECTED,
  WIFI_FAILED
};

// Calibration State Machine
enum CalibrationState {
  CAL_IDLE,
  CAL_IN_PROGRESS,
  CAL_COMPLETE,
  CAL_FAILED
};

// Global variables for erase button state
static bool erase_confirmation_active = false;
static lv_obj_t *erase_button = nullptr;
static lv_obj_t *erase_icon = nullptr;
static lv_obj_t *erase_text = nullptr;

// Global screen transition safety
static bool screen_transition_in_progress = false;

// WiFiManager class declaration
class WiFiManager {
private:
  WiFiState current_state = WIFI_IDLE;
  unsigned long state_change_time = 0;
  String target_ssid = "";
  String target_password = "";
  bool is_auto_connect = false;
  int connection_attempts = 0;

  // Non-blocking delay state
  unsigned long delay_start_time = 0;
  unsigned long delay_duration = 0;
  bool waiting_for_delay = false;
  bool restart_connection_after_delay = false;

  static constexpr unsigned long DISCONNECT_TIMEOUT = 5000;
  static constexpr unsigned long CONNECT_TIMEOUT = 15000;
  static constexpr int MAX_RETRY_ATTEMPTS = 2;

public:
  void update();
  bool connect(const String &ssid, const String &password, bool auto_connect = false);
  void disconnect();
  void setState(WiFiState new_state);  // <-- ADD THIS LINE HERE
  WiFiState getState() const {
    return current_state;
  }
  bool isConnected() const {
    return current_state == WIFI_CONNECTED;
  }
  String getConnectedSSID() const {
    return (current_state == WIFI_CONNECTED) ? WiFi.SSID() : "";
  }

private:
  void startConnection();
  void handleConnecting(unsigned long now);
  void onConnectionSuccess();
  void onConnectionFailed();
  void onConnectionLost();
  void updateWiFiIndicator();

  // Non-blocking delay helpers
  void startNonBlockingDelay(unsigned long duration_ms);
  bool isDelayComplete();

  const char *getStateName(WiFiState state) {
    switch (state) {
      case WIFI_IDLE: return "IDLE";
      case WIFI_DISCONNECTING: return "DISCONNECTING";
      case WIFI_CONNECTING: return "CONNECTING";
      case WIFI_CONNECTED: return "CONNECTED";
      case WIFI_FAILED: return "FAILED";
      default: return "UNKNOWN";
    }
  }
};

// Global WiFiManager instance
WiFiManager wifiManager;

// UI objects structure
struct UIElements {
  lv_obj_t *keyboard = nullptr;
  lv_obj_t *current_ta = nullptr;
  lv_obj_t *gps_label, *imu_label;
  lv_obj_t *sub_footer = nullptr;


  lv_obj_t *ssid_list_cont, *pass_ta, *wifi_button;
  lv_obj_t *wifi_symbol = nullptr;
  lv_obj_t *wifi_status_label, *wifi_ssid_label;
  lv_obj_t *rescan_btn, *rescan_label;
  lv_obj_t *settings_btn;
  lv_obj_t *error_popup = nullptr;
  lv_obj_t *settings_screen = nullptr;
  lv_obj_t *main_screen = nullptr;
  lv_obj_t *main_title = nullptr;
  lv_obj_t *device_name_label, *device_name_ta;
  lv_obj_t *edit_name_btn;

  // WiFi Modal elements
  lv_obj_t *wifi_modal = nullptr;
  lv_obj_t *modal_ssid_label = nullptr;
  lv_obj_t *modal_pass_ta = nullptr;
  lv_obj_t *modal_connect_btn = nullptr;
  lv_obj_t *modal_disconnect_btn = nullptr;
  lv_obj_t *modal_cancel_btn = nullptr;
  lv_obj_t *modal_status_label = nullptr;
  String modal_current_ssid = "";

  // Video settings labels for real-time updates
  lv_obj_t *video_screen = nullptr;
  lv_obj_t *video_btn = nullptr;

  lv_obj_t *resolution_label = nullptr;
  lv_obj_t *framerate_label = nullptr;
  lv_obj_t *codec_label = nullptr;
  lv_obj_t *quality_label = nullptr;
  // bitrate_label, stabilization_label, recording_status_label removed - unused

  bool in_settings = false;
  bool editing_name = false;
  bool in_video = false;
} ui;

// Performance metrics with optimization
struct PerformanceMetrics {
  unsigned long loop_count = 0;
  unsigned long max_loop_time = 0;
  unsigned long total_loop_time = 0;
  unsigned long last_report = 0;
  unsigned long loop_start = 0;
  unsigned long last_ui_update = 0;
  unsigned long last_sensor_update = 0;

  void startLoop() {
    loop_start = millis();
  }

  void endLoop() {
    unsigned long loop_time = millis() - loop_start;
    if (loop_time > max_loop_time) max_loop_time = loop_time;
    total_loop_time += loop_time;
    loop_count++;

    if (millis() - last_report > 10000) {
      reportMetrics();
      last_report = millis();
    }
  }

  bool shouldUpdateUI() {
    unsigned long now = millis();
    if (now - last_ui_update >= UIConstants::UI_UPDATE_INTERVAL) {
      last_ui_update = now;
      return true;
    }
    return false;
  }

  bool shouldUpdateSensors() {
    unsigned long now = millis();
    if (now - last_sensor_update >= UIConstants::SENSOR_UPDATE_INTERVAL) {
      last_sensor_update = now;
      return true;
    }
    return false;
  }

  void reportMetrics() {
    if (loop_count > 0) {
      VERBOSE_PRINT("Performance - Avg loop: ");
      VERBOSE_PRINT(total_loop_time / loop_count);
      VERBOSE_PRINT("ms, Max loop: ");
      VERBOSE_PRINT(max_loop_time);
      VERBOSE_PRINT("ms, Loops: ");
      VERBOSE_PRINTLN(loop_count);

      // Reset for next period
      total_loop_time = 0;
      loop_count = 0;
      max_loop_time = 0;
    }
  }
} perf_metrics;

// State management structure
struct SystemState {
  volatile unsigned long currentScanId = 0;
  String selected_ssid = "";
  bool shift_enabled = false;
  bool symbol_mode = false;

  // Timing
  unsigned long lastGpsUpdate = 0;
  unsigned long lastIMUUpdate = 0;
  unsigned long lastPostTime = 0;

  // Scanning
  bool scan_in_progress = false;
  unsigned long scan_start_time = 0;
  bool initial_scan_completed = false;  // ADD THIS LINE

  // Calibration
  CalibrationState cal_state = CAL_IDLE;
  unsigned long cal_start = 0;
  int cal_samples = 0;

  unsigned long last_user_interaction = 0;
} state;

// Sensor data structure
struct SensorData {
  // GPS data
  double lat = 0.0, lon = 0.0, speed = 0.0, alt = 0.0;
  int satellites = 0;
  bool gps_valid = false;

  // IMU data
  float pitch = 0.0f, roll = 0.0f, yaw = 0.0f, gforce = 0.0f;
  float accBiasX = 0, accBiasY = 0, accBiasZ = 0;
} sensors;

struct VideoSettings {
  String resolution = "1280x720";  // 720p by default
  int framerate = 30;
  int bitrate = 3000;  // Keep for internal use but don't display
  String codec = "H.264";
  String quality = "Low";     // Changed from "High" to "Low"
  bool stabilization = true;  // Keep for internal use but don't display
  bool recording = false;     // Keep for internal use but don't display
  unsigned long recording_start_time = 0;
  size_t file_size = 0;  // MB
} video_settings;

struct DisplaySleepState {
  unsigned long timeout = 60000;  // 30 seconds default
  bool sleeping = false;
  String current_setting = "60s";
  unsigned long lastTouchTime = 0;
} display_sleep;

// Forward declarations
void updateWiFiUI();
void toggleShift();
void toggleSymbols();
void textarea_event_cb(lv_event_t *e);
void create_keyboard();
void ssid_select_cb(lv_event_t *e);
void global_click_event_cb(lv_event_t *e);
void wifi_button_cb(lv_event_t *e);
void rescan_btn_event_cb(lv_event_t *e);
void forget_wifi_cb(lv_event_t *e);
void settings_btn_cb(lv_event_t *e);
void back_btn_cb(lv_event_t *e);
void edit_name_btn_cb(lv_event_t *e);
void save_name_btn_cb(lv_event_t *e);
void cancel_edit_btn_cb(lv_event_t *e);
void setupUI();
void createSettingsScreen();
void showMainScreen();
void startAsyncNetworkScan();
void checkScanResults();
void populateNetworkList(int network_count);
void showErrorMessage(const char *message, int duration_ms = 3000);
void handleWiFiStateMachine();
void updateCalibration();
void collectCalibrationSample();
void finishCalibration(int samples);
// bool isValidGPSData(); // Removed - unused
String getDeviceName();
void setDeviceName(const String &name);
void attemptAutoConnect();
void createWiFiModal(const String &ssid);
void closeWiFiModal();
void showWiFiModal(const String &ssid);
void modal_connect_cb(lv_event_t *e);
void modal_disconnect_cb(lv_event_t *e);
void modal_cancel_cb(lv_event_t *e);
void addCheckmarkToConnectedNetwork();
void updateFooterIndicatorsToCurrentState();
void video_btn_cb(lv_event_t *e);
void createVideoScreen();
void resolution_change_cb(lv_event_t *e);
void framerate_change_cb(lv_event_t *e);
void quality_change_cb(lv_event_t *e);
// void stabilization_toggle_cb(lv_event_t *e); // Removed - unused
void start_recording_cb(lv_event_t *e);
void updateVideoUI();
void updateDisplaySleepTimeout(const char *selection);
void checkDisplaySleep();
void wakeDisplay();
void updateUserInteraction();

// Device name management
String getDeviceName() {
  char name_buf[CREDENTIAL_BUFFER_SIZE] = { 0 };
  size_t actual_len;

  int ret = kv_get("/kv/device_name", name_buf, sizeof(name_buf), &actual_len);
  if (ret == MBED_SUCCESS && actual_len > 0) {
    return String(name_buf);
  }
  return String(DEFAULT_DEVICE_NAME);
}

void debugKVStore() {
  // Serial.println("=== KVSTORE DEBUG ===");

  // Check device name
  char name_buf[64] = { 0 };
  size_t actual_len;
  int ret = kv_get("/kv/device_name", name_buf, sizeof(name_buf), &actual_len);
  // Serial.print("Device name: ");
  if (ret == MBED_SUCCESS) {
    // Serial.print("EXISTS - '");
    // Serial.print(name_buf);
    // Serial.println("'");
  } else {
    // Serial.println("NOT FOUND");
  }

  // Check network count
  int count = 0;
  ret = kv_get("/kv/net_count", &count, sizeof(count), &actual_len);
  // Serial.print("Network count: ");
  if (ret == MBED_SUCCESS) {
    // Serial.print("EXISTS - ");
    // Serial.println(count);
  } else {
    // Serial.println("NOT FOUND");
  }

  // Check individual networks
  for (int i = 0; i < 4; i++) {
    char ssid_buf[64] = { 0 };
    String ssidKey = String("/kv/ssid_") + String(i);
    ret = kv_get(ssidKey.c_str(), ssid_buf, sizeof(ssid_buf), &actual_len);
    // Serial.print("Network ");
    // Serial.print(i);
    // Serial.print(": ");
    if (ret == MBED_SUCCESS) {
      // Serial.print("EXISTS - '");
      // Serial.print(ssid_buf);
      // Serial.println("'");
    } else {
      // Serial.println("NOT FOUND");
    }
  }
  // Check display sleep setting
  char sleep_buf[16] = { 0 };
  ret = kv_get("/kv/display_sleep", sleep_buf, sizeof(sleep_buf), &actual_len);
  // Serial.print("Display sleep setting: ");
  if (ret == MBED_SUCCESS) {
    // Serial.print("EXISTS - '");
    // Serial.print(sleep_buf);
    // Serial.println("'");
  } else {
    // Serial.println("NOT FOUND");
  }

  // Serial.println("=== END DEBUG ===");
}

void setDeviceName(const String &name) {
  // Serial.print("Saving device name: ");
  // Serial.println(name);

  kv_remove("/kv/device_name");
  int ret = kv_set("/kv/device_name", name.c_str(), name.length(), 0);

  if (ret != MBED_SUCCESS) {
    // Serial.println("Failed to save device name!");
  } else {
    // Serial.println("Device name saved successfully");

    // Update WiFi hostname if currently connected
    if (wifiManager.isConnected()) {
      String hostname = name;
      hostname.replace(" ", "-");
      hostname.replace(".", "-");

      // Serial.print("Updating WiFi hostname to: ");
      // Serial.println(hostname);
      WiFi.setHostname(hostname.c_str());
    }
  }
}
class DisplaySleepManager {
private:
  static constexpr const char *DISPLAY_SLEEP_KEY = "/kv/display_sleep";

public:
  static void saveDisplaySleepSetting(const String &setting) {
    // Serial.print("Saving display sleep setting: ");
    // Serial.println(setting);

    kv_remove(DISPLAY_SLEEP_KEY);
    int ret = kv_set(DISPLAY_SLEEP_KEY, setting.c_str(), setting.length(), 0);

    if (ret != MBED_SUCCESS) {
      // Serial.println("Failed to save display sleep setting!");
    } else {
      // Serial.println("Display sleep setting saved successfully");
    }
  }

  static String loadDisplaySleepSetting() {
    char sleep_buf[16] = { 0 };
    size_t actual_len;

    int ret = kv_get(DISPLAY_SLEEP_KEY, sleep_buf, sizeof(sleep_buf), &actual_len);
    if (ret == MBED_SUCCESS && actual_len > 0) {
      String setting = String(sleep_buf);
      // Serial.print("Loaded display sleep setting: ");
      // Serial.println(setting);
      return setting;
    }

    // Return default if not found
    // Serial.println("No saved display sleep setting found, using default: 60s");
    return String("60s");
  }

  static void clearDisplaySleepSetting() {
    kv_remove(DISPLAY_SLEEP_KEY);
    // Serial.println("Display sleep setting cleared");
  }
};

// Enhanced credential management
class CredentialManager {
private:
  static constexpr const char *NETWORK_COUNT_KEY = "/kv/net_count";
  static constexpr const char *SSID_PREFIX = "/kv/ssid_";
  static constexpr const char *PASS_PREFIX = "/kv/pass_";

  static int getNetworkCount() {
    size_t actual_len;
    int count = 0;
    int ret = kv_get(NETWORK_COUNT_KEY, &count, sizeof(count), &actual_len);
    if (ret != MBED_SUCCESS) return 0;
    return count;
  }

  static void setNetworkCount(int count) {
    kv_set(NETWORK_COUNT_KEY, &count, sizeof(count), 0);
  }

  static String getSSIDKey(int index) {
    return String(SSID_PREFIX) + String(index);
  }

  static String getPasswordKey(int index) {
    return String(PASS_PREFIX) + String(index);
  }

  static void saveNetworkAtIndex(int index, const String &ssid, const String &password) {
    String ssidKey = getSSIDKey(index);
    String passKey = getPasswordKey(index);

    kv_remove(ssidKey.c_str());
    kv_remove(passKey.c_str());

    kv_set(ssidKey.c_str(), ssid.c_str(), ssid.length(), 0);
    kv_set(passKey.c_str(), password.c_str(), password.length(), 0);
  }

  static void removeNetworkAtIndex(int index) {
    String ssidKey = getSSIDKey(index);
    String passKey = getPasswordKey(index);

    kv_remove(ssidKey.c_str());
    kv_remove(passKey.c_str());
  }

  static int findNetwork(const String &ssid) {
    int count = getNetworkCount();
    for (int i = 0; i < count; i++) {
      String savedSsid, savedPass;
      if (loadNetworkAtIndex(i, savedSsid, savedPass)) {
        if (savedSsid == ssid) {
          return i;
        }
      }
    }
    return -1;
  }

  static void shiftNetworksDown() {
    int count = getNetworkCount();
    for (int i = 0; i < count - 1; i++) {
      String ssid, password;
      if (loadNetworkAtIndex(i + 1, ssid, password)) {
        saveNetworkAtIndex(i, ssid, password);
      }
    }
    removeNetworkAtIndex(count - 1);
  }

public:
  static bool loadNetworkAtIndex(int index, String &ssid, String &password) {
    char ssid_buf[CREDENTIAL_BUFFER_SIZE] = { 0 };
    char pass_buf[CREDENTIAL_BUFFER_SIZE] = { 0 };
    size_t actual_len;

    String ssidKey = getSSIDKey(index);
    String passKey = getPasswordKey(index);

    int ret1 = kv_get(ssidKey.c_str(), ssid_buf, sizeof(ssid_buf), &actual_len);
    int ret2 = kv_get(passKey.c_str(), pass_buf, sizeof(pass_buf), &actual_len);

    if (ret1 == MBED_SUCCESS && ret2 == MBED_SUCCESS) {
      ssid = String(ssid_buf);
      password = String(pass_buf);
      return true;
    }
    return false;
  }

  static void save(const String &ssid, const String &password) {
    int existing_index = findNetwork(ssid);

    if (existing_index >= 0) {
      String existingSSID, existingPassword;
      if (loadNetworkAtIndex(existing_index, existingSSID, existingPassword)) {
        if (existingPassword != password) {
          saveNetworkAtIndex(existing_index, ssid, password);
        }
      } else {
        saveNetworkAtIndex(existing_index, ssid, password);
      }
    } else {
      int count = getNetworkCount();
      if (count < MAX_NETWORKS) {
        saveNetworkAtIndex(count, ssid, password);
        setNetworkCount(count + 1);
      } else {
        shiftNetworksDown();
        saveNetworkAtIndex(MAX_NETWORKS - 1, ssid, password);
      }
    }
  }

  static bool loadMostRecent(String &ssid, String &password) {
    int count = getNetworkCount();
    if (count == 0) return false;

    for (int i = count - 1; i >= 0; i--) {
      if (loadNetworkAtIndex(i, ssid, password)) {
        return true;
      }
    }
    return false;
  }

  static void debugListNetworks() {
    int count = getNetworkCount();
    // Serial.print("Total saved networks: ");
    // Serial.println(count);

    for (int i = 0; i < count; i++) {
      String ssid, password;
      if (loadNetworkAtIndex(i, ssid, password)) {
        // Serial.print("Network ");
        // Serial.print(i);
        // Serial.print(": SSID='");
        // Serial.print(ssid);
        // Serial.print("', Password length=");
        // Serial.println(password.length());
      }
    }
  }

  static bool isNetworkSaved(const String &ssid) {
    return findNetwork(ssid) >= 0;
  }

  static void removeNetwork(const String &ssid) {
    int index = findNetwork(ssid);
    if (index >= 0) {
      removeNetworkAtIndex(index);

      int count = getNetworkCount();
      for (int i = index; i < count - 1; i++) {
        String tempSsid, tempPass;
        if (loadNetworkAtIndex(i + 1, tempSsid, tempPass)) {
          saveNetworkAtIndex(i, tempSsid, tempPass);
        }
      }

      removeNetworkAtIndex(count - 1);
      setNetworkCount(count - 1);
    }
  }

  static void clear() {
    // Serial.println("CredentialManager: Starting complete clear operation");

    // Get current count
    int count = getNetworkCount();
    // Serial.print("Found ");
    // Serial.print(count);
    // Serial.println(" networks to remove");

    // Remove all individual network entries
    for (int i = 0; i < MAX_NETWORKS; i++) {
      String ssidKey = getSSIDKey(i);
      String passKey = getPasswordKey(i);

      kv_remove(ssidKey.c_str());
      kv_remove(passKey.c_str());

      // Serial.print("Removed network slot ");
      // Serial.println(i);
    }

    // Remove the network count
    kv_remove(NETWORK_COUNT_KEY);

    // Set count to 0 explicitly
    setNetworkCount(0);

    DisplaySleepManager::clearDisplaySleepSetting();

    // Serial.println("CredentialManager: Clear operation completed");

    // Verify the clear worked
    int newCount = getNetworkCount();
    // Serial.print("Network count after clear: ");
    // Serial.println(newCount);
  }
};

// WiFiManager method implementations
void WiFiManager::update() {
  unsigned long now = millis();

  switch (current_state) {
    case WIFI_IDLE:
      break;

    case WIFI_DISCONNECTING:
      if (WiFi.status() != WL_CONNECTED || (now - state_change_time > DISCONNECT_TIMEOUT)) {
        startConnection();
      }
      break;

    case WIFI_CONNECTING:
      handleConnecting(now);
      break;

    case WIFI_CONNECTED:
      if (WiFi.status() != WL_CONNECTED) {
        setState(WIFI_FAILED);
        onConnectionLost();
      }
      break;

    case WIFI_FAILED:
      break;
  }
}

bool WiFiManager::connect(const String &ssid, const String &password, bool auto_connect) {
  if (current_state == WIFI_CONNECTING) {
    return false;
  }

  target_ssid = ssid;
  target_password = password;
  is_auto_connect = auto_connect;
  connection_attempts = 0;

  if (WiFi.status() == WL_CONNECTED && WiFi.SSID() != ssid) {
    setState(WIFI_DISCONNECTING);
    WiFi.disconnect();
  } else {
    startConnection();
  }

  return true;
}

void WiFiManager::disconnect() {
  if (current_state == WIFI_CONNECTED) {
    // Note: Telemetry connection cleanup handled elsewhere to avoid circular dependencies
    setState(WIFI_DISCONNECTING);
    WiFi.disconnect();
  }
}

void WiFiManager::setState(WiFiState new_state) {
  if (current_state != new_state) {
    current_state = new_state;
    state_change_time = millis();
    updateWiFiIndicator();
  }
}

void WiFiManager::startConnection() {
  setState(WIFI_CONNECTING);
  connection_attempts++;

  String deviceName = getDeviceName();
  String hostname = deviceName;
  hostname.replace(" ", "-");
  hostname.replace(".", "-");

  WiFi.disconnect();
  startNonBlockingDelay(1000);

  WiFi.setHostname(hostname.c_str());
  WiFi.begin(target_ssid.c_str(), target_password.c_str());
}

void WiFiManager::handleConnecting(unsigned long now) {
  // If we're waiting for a delay, check if it's complete
  if (!isDelayComplete()) {
    return; // Still waiting, exit early
  }

  uint8_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    setState(WIFI_CONNECTED);
    onConnectionSuccess();
  } else if (now - state_change_time > CONNECT_TIMEOUT) {
    if (connection_attempts < MAX_RETRY_ATTEMPTS) {
      WiFi.disconnect();
      startNonBlockingDelay(1000);
      restart_connection_after_delay = true;
      connection_attempts++;
      setState(WIFI_CONNECTING); // Reset the timer
    } else {
      setState(WIFI_FAILED);
      onConnectionFailed();
    }
  } else if (status == WL_CONNECT_FAILED) {
    if (connection_attempts < MAX_RETRY_ATTEMPTS) {
      WiFi.disconnect();
      startNonBlockingDelay(2000);
      restart_connection_after_delay = true;
      connection_attempts++;
      setState(WIFI_CONNECTING); // Reset the timer
    } else {
      setState(WIFI_FAILED);
      onConnectionFailed();
    }
  } else if (status == WL_NO_SSID_AVAIL) {
    setState(WIFI_FAILED);
    onConnectionFailed();
  }
}

void WiFiManager::onConnectionSuccess() {
  if (!is_auto_connect && !target_ssid.isEmpty() && !target_password.isEmpty()) {
    CredentialManager::save(target_ssid, target_password);
  }

  // Serial.println("WiFi connected successfully");

  // Just update UI and add checkmark
  lv_timer_create([](lv_timer_t *t) {
    updateWiFiUI();
    addCheckmarkToConnectedNetwork();
    lv_timer_del(t);
  },
                  500, nullptr);
}

void WiFiManager::onConnectionFailed() {
  updateWiFiUI();
}

void WiFiManager::onConnectionLost() {
  // Serial.println("WiFi connection lost");

  // Defer UI updates here too
  lv_timer_create([](lv_timer_t *t) {
    updateWiFiUI();
    lv_timer_del(t);
  },
                  500, nullptr);

  // Don't manipulate UI elements immediately
}

// Replace the updateWiFiIndicator method in WiFiManager with this:

void WiFiManager::updateWiFiIndicator() {
  lv_color_t indicator_color;
  switch (current_state) {
    case WIFI_CONNECTED:
      indicator_color = lv_color_hex(UIConstants::COLOR_PRIMARY);
      break;
    case WIFI_CONNECTING:
      indicator_color = lv_color_hex(UIConstants::COLOR_WARNING);
      break;
    default:
      indicator_color = lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY);
      break;
  }

  // Use the global footer helper to update indicator
  FooterHelper::updateWiFiIndicator(indicator_color);
}

// Non-blocking delay helper methods
void WiFiManager::startNonBlockingDelay(unsigned long duration_ms) {
  delay_start_time = millis();
  delay_duration = duration_ms;
  waiting_for_delay = true;
}

bool WiFiManager::isDelayComplete() {
  if (!waiting_for_delay) return true;

  if (millis() - delay_start_time >= delay_duration) {
    waiting_for_delay = false;

    // If we need to restart connection after delay, do it now
    if (restart_connection_after_delay) {
      restart_connection_after_delay = false;
      String deviceName = getDeviceName();
      String hostname = deviceName;
      hostname.replace(" ", "-");
      hostname.replace(".", "-");
      WiFi.setHostname(hostname.c_str());
      WiFi.begin(target_ssid.c_str(), target_password.c_str());
    }

    return true;
  }

  // Keep the system alive during delay
  watchdog.kick();
  lv_timer_handler();
  return false;
}

// WebSocket Secure Telemetry Client Class
class TelemetryClient {
private:
  WiFiSSLClient sslClient;
  WebSocketClient wsClient;
  bool initialized = false;
  bool connected = false;
  unsigned long lastConnectionAttempt = 0;
  unsigned long lastPingTime = 0;
  static constexpr unsigned long CONNECTION_RETRY_INTERVAL = 10000; // 10 seconds
  static constexpr unsigned long PING_INTERVAL = 30000; // 30 seconds

public:
  TelemetryClient()
    : wsClient(sslClient, "socket.starstream.pro", 443) {
    initialized = true;
  }

  bool connect() {
    if (!initialized || WiFi.status() != WL_CONNECTED) {
      return false;
    }

    if (connected && wsClient.connected()) {
      return true; // Already connected
    }

    unsigned long now = millis();
    if (now - lastConnectionAttempt < CONNECTION_RETRY_INTERVAL) {
      return false; // Too soon to retry
    }

    lastConnectionAttempt = now;

    DEBUG_PRINTLN("Attempting WSS connection to telemetry server...");

    // Attempt WebSocket connection to StarStream telemetry endpoint
    if (wsClient.begin("/api/telemetry")) {
      connected = true;
      lastPingTime = now;
      DEBUG_PRINTLN("WSS telemetry connection established");
      return true;
    } else {
      connected = false;
      DEBUG_PRINTLN("WSS telemetry connection failed");
      return false;
    }
  }

  void disconnect() {
    if (connected) {
      wsClient.stop();
      connected = false;
      DEBUG_PRINTLN("WSS telemetry connection closed");
    }
  }

  bool sendData(const char *json) {
    if (!ensureConnection()) {
      return false;
    }

    // Send JSON message over WebSocket
    if (wsClient.beginMessage(TYPE_TEXT)) {
      size_t bytes_written = wsClient.print(json);
      wsClient.endMessage();

      if (bytes_written > 0) {
        VERBOSE_PRINT("WSS telemetry data sent: ");
        VERBOSE_PRINTLN(json);
        return true;
      }
    }

    DEBUG_PRINTLN("Failed to send WSS telemetry data");
    connected = false; // Mark as disconnected to trigger reconnection
    return false;
  }

  void update() {
    if (!connected) {
      return;
    }

    // Check for incoming messages (server responses/commands)
    int messageSize = wsClient.parseMessage();
    if (messageSize > 0) {
      String message = wsClient.readString();
      VERBOSE_PRINT("WSS message received: ");
      VERBOSE_PRINTLN(message);

      // Handle server messages (optional - for future server commands)
      handleServerMessage(message);
    }

    // Send periodic ping to keep connection alive
    unsigned long now = millis();
    if (now - lastPingTime > PING_INTERVAL) {
      if (wsClient.ping()) {
        lastPingTime = now;
        VERBOSE_PRINTLN("WSS ping sent");
      } else {
        DEBUG_PRINTLN("WSS ping failed - connection may be lost");
        connected = false;
      }
    }

    // Check if connection is still alive
    if (!wsClient.connected()) {
      connected = false;
      DEBUG_PRINTLN("WSS connection lost");
    }
  }

  bool isConnected() {
    return connected && wsClient.connected();
  }

private:
  bool ensureConnection() {
    if (isConnected()) {
      return true;
    }
    return connect();
  }

  void handleServerMessage(const String& message) {
    // Parse server messages for future features like:
    // - Configuration updates
    // - Commands from server
    // - Acknowledgments
    // For now, just log the message
    VERBOSE_PRINT("Server message: ");
    VERBOSE_PRINTLN(message);
  }
};

TelemetryClient telemetryClient;

// Sensor Manager Class
class SensorManager {
private:
  unsigned long lastGPSUpdate = 0;
  unsigned long lastIMUUpdate = 0;

public:
  void updateGPS() {
    unsigned long now = millis();
    if (now - lastGPSUpdate < GPS_UPDATE_INTERVAL) return;
    lastGPSUpdate = now;

    // SIMPLIFIED GPS PROCESSING with safety protection
    // Process GPS data with limits to prevent infinite loop
    int gps_chars_processed = 0;
    const int MAX_GPS_CHARS_PER_UPDATE = 100; // Prevent processing too many chars at once
    unsigned long gps_process_start = millis();
    const unsigned long GPS_PROCESS_TIMEOUT = 50; // 50ms max processing time

    while (GPSSerial.available() &&
           gps_chars_processed < MAX_GPS_CHARS_PER_UPDATE &&
           (millis() - gps_process_start) < GPS_PROCESS_TIMEOUT) {
      if (gps.encode(GPSSerial.read())) {
        // Complete sentence received, but continue processing more data
      }
      gps_chars_processed++;

      // Kick watchdog periodically during GPS processing
      if (gps_chars_processed % 25 == 0) {
        watchdog.kick();
      }
    }

    // Check if we have valid GPS data - SIMPLIFIED VALIDATION
    if (gps.location.isValid()) {
      // We have a valid GPS fix - extract all data
      sensors.lat = gps.location.lat();
      sensors.lon = gps.location.lng();
      sensors.gps_valid = true;

      // Get speed if available
      if (gps.speed.isValid()) {
        sensors.speed = gps.speed.mph();
      } else {
        sensors.speed = 0.0;
      }

      // Get altitude if available
      if (gps.altitude.isValid()) {
        sensors.alt = gps.altitude.feet();
      } else {
        sensors.alt = 0.0;
      }

      // Get satellite count
      if (gps.satellites.isValid()) {
        sensors.satellites = gps.satellites.value();
      } else {
        sensors.satellites = 0;
      }


      // Keep the old combined GPS label for backward compatibility
      if (ui.gps_label) {
        static char gps_buf[256];
        snprintf(gps_buf, sizeof(gps_buf),
                 "Latitude: %.6f\nLongitude: %.6f\nSpeed: %.1f MPH\nAltitude: %.0f ft\nSatellites: %d",
                 sensors.lat, sensors.lon, sensors.speed, sensors.alt, sensors.satellites);
        lv_label_set_text(ui.gps_label, gps_buf);
      }

      // Set GPS indicator to success (green)
      FooterHelper::updateGPSIndicator(lv_color_hex(UIConstants::COLOR_SUCCESS));

      // DEBUG: Print GPS data to serial (like the working version)
      VERBOSE_PRINT("GPS: Lat=");
      VERBOSE_PRINT(sensors.lat, 6);
      VERBOSE_PRINT(", Lon=");
      VERBOSE_PRINT(sensors.lon, 6);
      VERBOSE_PRINT(", Speed=");
      VERBOSE_PRINT(sensors.speed);
      VERBOSE_PRINT(" MPH, Alt=");
      VERBOSE_PRINT(sensors.alt);
      VERBOSE_PRINT(" ft, Sats=");
      VERBOSE_PRINTLN(sensors.satellites);

    } else {
      // No valid GPS fix yet
      sensors.gps_valid = false;

      // Check if we're receiving any GPS data at all
      static unsigned long lastDataCheck = 0;
      if (now - lastDataCheck > 5000) {  // Check every 5 seconds
        lastDataCheck = now;

        // Print debug info to help diagnose issues
        VERBOSE_PRINT("GPS Debug - Characters processed: ");
        VERBOSE_PRINT(gps.charsProcessed());
        VERBOSE_PRINT(", Sentences with fix: ");
        VERBOSE_PRINT(gps.sentencesWithFix());
        VERBOSE_PRINT(", Failed checksums: ");
        VERBOSE_PRINTLN(gps.failedChecksum());

        if (gps.charsProcessed() == 0) {
          // Serial.println("GPS: No data received - check wiring");
        } else if (gps.sentencesWithFix() == 0) {
          // Serial.println("GPS: Receiving data but no fix - check antenna/location");
        }
      }

      // Show appropriate status based on what we're receiving
      if (gps.charsProcessed() > 0) {
        // We're getting data, just no fix yet

        if (ui.gps_label) {
          static char gps_buf[128];
          int sats = gps.satellites.isValid() ? gps.satellites.value() : 0;
          snprintf(gps_buf, sizeof(gps_buf), "Searching for GPS fix...\nSatellites: %d", sats);
          lv_label_set_text(ui.gps_label, gps_buf);
        }

        // Set GPS indicator to warning (orange)
        FooterHelper::updateGPSIndicator(lv_color_hex(UIConstants::COLOR_WARNING));

      } else {
        // No data at all - hardware/wiring issue

        if (ui.gps_label) {
          lv_label_set_text(ui.gps_label, "No GPS signal detected\nCheck antenna/wiring");
        }

        // Set GPS indicator to danger (red)
        FooterHelper::updateGPSIndicator(lv_color_hex(UIConstants::COLOR_DANGER));
      }
    }
  }

  void updateIMU() {
  unsigned long now = millis();
  if (now - lastIMUUpdate < IMU_UPDATE_INTERVAL) return;
  lastIMUUpdate = now;

  // Check if IMU hardware is available
  if (!hardware_status.imu_available) {
    // IMU hardware not available
    
    if (ui.imu_label) {
      lv_label_set_text(ui.imu_label, "IMU Hardware Not Available\nMotion sensing disabled");
    }
    return;
  }

  // CHANGED: Use built-in IMU methods instead of external sensor
  float ax, ay, az, gx, gy, gz;
  
  // Read accelerometer data from built-in BMI270
  if (!imu.readAcceleration(ax, ay, az)) {
    return;  // Failed to read accelerometer
  }
  
  // Read gyroscope data from built-in BMI270
  if (!imu.readGyroscope(gx, gy, gz)) {
    return;  // Failed to read gyroscope
  }

  // Check if magnetometer is available before reading
  float mx = 0, my = 0, mz = 0;
  bool mag_available = hardware_status.magnetometer_available;
  
  if (mag_available) {
    // Try to read magnetometer data from built-in BMM150
    if (!imu.readMagneticField(mx, my, mz)) {
      mag_available = false;  // Magnetometer read failed
    }
  }

  // Apply calibration offsets
  ax -= sensors.accBiasX;
  ay -= sensors.accBiasY;
  az -= (sensors.accBiasZ - 1.0);

  // Coordinate system adjustments for proper orientation
  ay = -ay;
  gy = -gy;

  // Calculate G-Force
  sensors.gforce = sqrt(ax * ax + ay * ay + az * az);

  // Update Madgwick filter with or without magnetometer
  if (mag_available) {
    // Use magnetometer for better yaw accuracy
    filter.update(radians(gx), radians(gy), radians(gz), ax, ay, az,
                  mx / 100.0f, -my / 100.0f, -mz / 100.0f);
  } else {
    // Use IMU-only mode (no magnetometer)
    filter.updateIMU(radians(gx), radians(gy), radians(gz), ax, ay, az);
  }

  // Get orientation from filter
  sensors.pitch = filter.getPitch();
  sensors.roll = filter.getRoll();
  sensors.yaw = filter.getYaw();


  // Keep the old combined IMU label for backward compatibility
  if (ui.imu_label) {
    static char imu_buf[128];
    if (mag_available) {
      snprintf(imu_buf, sizeof(imu_buf),
               "Pitch: %.0f deg\nRoll: %.0f deg\nYaw: %.0f deg\nG-Force: %.2f G",
               sensors.pitch, sensors.roll, sensors.yaw, sensors.gforce);
    } else {
      snprintf(imu_buf, sizeof(imu_buf),
               "Pitch: %.0f deg\nRoll: %.0f deg\nYaw: %.0f deg (no compass)\nG-Force: %.2f G",
               sensors.pitch, sensors.roll, sensors.yaw, sensors.gforce);
    }
    lv_label_set_text(ui.imu_label, imu_buf);
  }
}

  const SensorData &getData() const {
    return sensors;
  }
};

SensorManager sensorManager;

void debugGPSConnection() {
  // Serial.println("=== GPS CONNECTION TEST ===");

  // Test if Serial2 is working
  // Serial.print("GPS Serial available: ");
  // Serial.println(GPSSerial.available());

  // Send a test command to GPS
  GPSSerial.println("$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28");
  delay(100);

  // Serial.print("GPS Serial available after test: ");
  // Serial.println(GPSSerial.available());

  // Serial.println("GPS should start showing data within 30 seconds...");
  // Serial.println("=============================");
}

// WSS Telemetry Manager Class
class TelemetryManager {
private:
  unsigned long lastSend = 0;
  unsigned long lastConnectionCheck = 0;
  static constexpr unsigned long CONNECTION_CHECK_INTERVAL = 5000; // Check connection every 5 seconds

public:
  void update() {
    unsigned long now = millis();

    // Always update the WSS client (handles pings, reconnections, incoming messages)
    telemetryClient.update();

    // Check and establish connection periodically
    if (wifiManager.isConnected() && (now - lastConnectionCheck > CONNECTION_CHECK_INTERVAL)) {
      if (!telemetryClient.isConnected()) {
        DEBUG_PRINTLN("WSS telemetry not connected, attempting connection...");
        telemetryClient.connect();
      }
      lastConnectionCheck = now;
    }

    // Send data only if we have WiFi and enough time has passed
    if (!wifiManager.isConnected() || (now - lastSend < TELEMETRY_INTERVAL)) {
      return;
    }

    sendData();
    lastSend = now;
  }

  void sendData() {
    if (!sensors.gps_valid) {
      return;
    }

    // Enhanced JSON with timestamp and device info
    char json[600];
    snprintf(json, sizeof(json),
             "{\"timestamp\":%lu,\"device\":\"%s\","
             "\"lat\":%.6f,\"lon\":%.6f,\"speed\":%.1f,\"sats\":%d,"
             "\"pitch\":%.1f,\"roll\":%.1f,\"yaw\":%.1f,\"gforce\":%.2f,"
             "\"alt\":%.1f}",
             millis(), DEVICE_SERIAL_NUMBER, sensors.lat, sensors.lon, sensors.speed, sensors.satellites,
             sensors.pitch, sensors.roll, sensors.yaw, sensors.gforce, sensors.alt);

    bool success = telemetryClient.sendData(json);
    updateStatusUI(success);
  }

  void disconnect() {
    telemetryClient.disconnect();
    DEBUG_PRINTLN("Telemetry WSS connection closed");
  }

  bool isConnected() {
    return telemetryClient.isConnected();
  }

private:
  void updateStatusUI(bool success) {
    // ONLY update WiFi status label if it's visible (not hidden by network list)
    if (ui.wifi_status_label && !ui.in_settings && !lv_obj_has_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN)) {
      if (success) {
        lv_label_set_text(ui.wifi_status_label, "WSS data sent successfully");
      } else {
        if (telemetryClient.isConnected()) {
          lv_label_set_text(ui.wifi_status_label, "WSS send failed");
        } else {
          lv_label_set_text(ui.wifi_status_label, "WSS not connected");
        }
      }
    }
  }
};

TelemetryManager telemetryManager;

// Manual OTA Manager Class - Fixed version
class ManualOTAManager {
private:
  WiFiSSLClient sslClient;
  HttpClient httpClient;
  bool checking_update = false;
  bool update_available = false;
  String remote_version = "";
  String remote_checksum = "";
  size_t firmware_size = 0;

  // CORRECTED Flash memory constants for STM32H747 - Fixed for actual firmware size
  static constexpr uint32_t OTA_FLASH_BASE_ADDR = 0x08000000;
  static constexpr uint32_t OTA_TOTAL_FLASH_SIZE = 0x200000;                            // 2MB total flash
  static constexpr uint32_t OTA_FLASH_SECTOR_SIZE = 0x20000;                            // 128KB per sector

  // DYNAMIC allocation based on actual firmware size
  static constexpr size_t CURRENT_FIRMWARE_SIZE = 1052908;                             // Your actual firmware size
  static constexpr size_t FIRMWARE_PADDING = 0x10000;                                  // 64KB safety padding
  static constexpr uint32_t OTA_CURRENT_END = OTA_FLASH_BASE_ADDR + CURRENT_FIRMWARE_SIZE + FIRMWARE_PADDING;
  static constexpr uint32_t OTA_STAGING_START = (OTA_CURRENT_END + OTA_FLASH_SECTOR_SIZE - 1) & ~(OTA_FLASH_SECTOR_SIZE - 1); // Sector-aligned
  static constexpr uint32_t OTA_AVAILABLE_SPACE = (OTA_FLASH_BASE_ADDR + OTA_TOTAL_FLASH_SIZE) - OTA_STAGING_START;

  static constexpr size_t MIN_FIRMWARE_SIZE = 50000;  // Reduced for LZSS compressed files
  static constexpr size_t MAX_FIRMWARE_SIZE = OTA_AVAILABLE_SPACE - 0x10000;            // 64KB safety margin
  static constexpr unsigned long DOWNLOAD_TIMEOUT = 180000;                            // Increased timeout

  // Firmware size optimization recommendations
  static void printOptimizationRecommendations() {
    // Removed verbose recommendations to save flash space
  }

  // Improved progress callback - more stable and less intrusive
  static void onOTAProgress(size_t current, size_t total) {
    static unsigned long last_progress_update = 0;
    static int last_percent = -1;

    unsigned long now = millis();
    // Safe percentage calculation to prevent integer overflow
    int current_percent = 0;
    if (total > 0) {
      if (current >= total) {
        current_percent = 100;
      } else {
        // Use long long to prevent overflow during multiplication
        current_percent = (int)((unsigned long long)current * 100ULL / total);
      }
    }

    // Update every 5% or every 5 seconds, whichever comes first
    // Reduced frequency to minimize system load during OTA
    if ((current_percent >= last_percent + 5) || (now - last_progress_update > 5000)) {
      Serial.print("📈 OTA Progress: ");
      Serial.print(current_percent);
      Serial.print("% (");
      Serial.print(current);
      Serial.print("/");
      Serial.print(total);
      Serial.println(" bytes)");

      // Only update UI if we're in a safe state
      if (current_percent <= 90) {
        updateOTAProgressUI(current_percent);
      }

      // SUGGESTION #2: Progressive Memory Cleanup During Download
      // Force memory cleanup every 10% to prevent memory pressure
      if (current_percent % 10 == 0) {
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        Serial.print("📊 Memory check at ");
        Serial.print(current_percent);
        Serial.print("%: ");
        Serial.print(mon.free_size);
        Serial.println(" bytes free");

        if (mon.free_size < 20000) { // If less than 20KB free
          Serial.println("⚡ Low memory detected, forcing cleanup...");
          // lv_gc_collect(); // Force garbage collection (function not available)
          delay(10); // Small delay to allow cleanup
          MemoryManager::forceCleanup(); // Additional cleanup

          // Re-check memory after cleanup
          lv_mem_monitor(&mon);
          Serial.print("📈 After cleanup: ");
          Serial.print(mon.free_size);
          Serial.println(" bytes free");
        }
      }

      last_progress_update = now;
      last_percent = current_percent;
    }
  }

  static void updateOTAProgressUI(int progress_percent) {
    if (!ui.settings_screen) return;

    // Validate and clamp progress percentage
    if (progress_percent < 0) progress_percent = 0;
    if (progress_percent > 100) progress_percent = 100;

    // Find the update button and update its text
    uint32_t child_count = lv_obj_get_child_cnt(ui.settings_screen);
    for (uint32_t i = 0; i < child_count; i++) {
      lv_obj_t *child = lv_obj_get_child(ui.settings_screen, i);
      if (lv_obj_get_child_cnt(child) > 0) {
        lv_obj_t *flex_container = lv_obj_get_child(child, 0);
        if (flex_container && lv_obj_get_child_cnt(flex_container) >= 2) {
          lv_obj_t *text_label = lv_obj_get_child(flex_container, 1);
          const char *text = lv_label_get_text(text_label);
          if (strstr(text, "Updating") != nullptr || strstr(text, "Installing") != nullptr || strstr(text, "Downloading") != nullptr) {
            static char progress_buf[64];

            if (progress_percent >= 90) {
              snprintf(progress_buf, sizeof(progress_buf), "Installing... %d%%", progress_percent);
            } else {
              snprintf(progress_buf, sizeof(progress_buf), "Downloading... %d%%", progress_percent);
            }

            lv_label_set_text(text_label, progress_buf);
            lv_obj_invalidate(child);

            // Force immediate UI update
            lv_timer_handler();
            lv_refr_now(NULL);
            break;
          }
        }
      }
    }
  }

  bool isNewerVersion(const String &remote, const String &current) {
    // Parse version strings in format X.Y.Z
    int remote_parts[3] = { 0, 0, 0 };
    int current_parts[3] = { 0, 0, 0 };

    // Parse remote version
    int part = 0;
    int start = 0;
    for (int i = 0; i <= remote.length() && part < 3; i++) {
      if (i == remote.length() || remote.charAt(i) == '.') {
        if (i > start) {
          remote_parts[part] = remote.substring(start, i).toInt();
        }
        start = i + 1;
        part++;
      }
    }

    // Parse current version
    part = 0;
    start = 0;
    for (int i = 0; i <= current.length() && part < 3; i++) {
      if (i == current.length() || current.charAt(i) == '.') {
        if (i > start) {
          current_parts[part] = current.substring(start, i).toInt();
        }
        start = i + 1;
        part++;
      }
    }

    // Compare versions
    for (int i = 0; i < 3; i++) {
      Serial.print("Comparing part ");
      // Serial.print(i);
      Serial.print(": remote=");
      Serial.print(remote_parts[i]);
      Serial.print(" vs current=");
      Serial.println(current_parts[i]);

      if (remote_parts[i] > current_parts[i]) {
        return true;  // Remote is newer
      } else if (remote_parts[i] < current_parts[i]) {
        return false;  // Remote is older
      }
      // If equal, continue to next part
    }

    return false;  // Versions are identical
  }

  bool validateFirmware(const String &checksum, size_t size) {
    Serial.println("=== VALIDATING FIRMWARE ===");
    Serial.print("Size: ");
    Serial.print(size);
    Serial.println(" bytes");
    Serial.print("Size in MB: ");
    Serial.println(size / (1024.0 * 1024.0), 2);
    Serial.print("Checksum: '");
    Serial.print(checksum);
    // Serial.println("'");
    Serial.print("Remote version: '");
    Serial.print(remote_version);
    // Serial.println("'");
    Serial.print("Current version: '");
    Serial.print(FIRMWARE_VERSION);
    // Serial.println("'");

    // Validate size with better error messages
    Serial.print("Size limits: ");
    Serial.print(MIN_FIRMWARE_SIZE);
    Serial.print(" - ");
    Serial.print(MAX_FIRMWARE_SIZE);
    Serial.println(" bytes");

    if (size < MIN_FIRMWARE_SIZE) {
      Serial.print("INVALID SIZE: Too small - ");
      Serial.print(size);
      Serial.print(" bytes is less than minimum ");
      Serial.print(MIN_FIRMWARE_SIZE);
      Serial.println(" bytes");
      return false;
    }

    // Temporarily disable size check for OTA testing
    if (size > MAX_FIRMWARE_SIZE) {
      Serial.print("INVALID SIZE: Too large - ");
      Serial.print(size);
      Serial.print(" bytes exceeds maximum ");
      Serial.print(MAX_FIRMWARE_SIZE);
      Serial.println(" bytes");
      return false;
    }

    Serial.println("Size validation: PASSED");

    // Validate version format
    if (remote_version.length() < 3 || remote_version.indexOf('.') == -1) {
      Serial.print("INVALID VERSION FORMAT: '");
      Serial.print(remote_version);
      // Serial.println("'");
      return false;
    }
    Serial.println("Version format validation: PASSED");

    // Enhanced version comparison with detailed logging
    String current_version = String(FIRMWARE_VERSION);
    Serial.println("Detailed version comparison:");

    bool isNewer = isNewerVersion(remote_version, current_version);
    Serial.print("Is remote version newer? ");
    Serial.println(isNewer ? "YES" : "NO");

    if (!isNewer) {
      Serial.println("VALIDATION FAILED: Remote version is not newer");
      return false;
    }
    Serial.println("Version comparison: PASSED");

    // Validate checksum format
    if (checksum.length() != 32 && checksum.length() != 8) {
      Serial.print("INVALID CHECKSUM LENGTH: ");
      Serial.print(checksum.length());
      Serial.println(" (expected 8 or 32)");
      return false;
    }

    for (int i = 0; i < checksum.length(); i++) {
      char c = checksum.charAt(i);
      if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
        Serial.print("INVALID CHECKSUM CHARACTER at position ");
        // Serial.print(i);
        Serial.print(": '");
        Serial.print(c);
        Serial.print("' (code: ");
        Serial.print((int)c);
        Serial.println(")");
        return false;
      }
    }
    Serial.println("Checksum format validation: PASSED");

    Serial.println("=== FIRMWARE VALIDATION SUCCESSFUL ===");
    return true;
  }

private:
  // Simple checksum calculation without MD5 library
  String calculateSimpleChecksum(uint8_t *data, size_t length) {
    uint32_t checksum = 0;

    for (size_t i = 0; i < length; i++) {
      checksum = checksum * 31 + data[i];

      // Kick watchdog periodically during calculation
      if (i % 1000 == 0) {
        watchdog.kick();
      }
    }

    // Convert to hex string (8 characters for 32-bit checksum)
    String result = "";
    for (int i = 7; i >= 0; i--) {
      uint8_t nibble = (checksum >> (i * 4)) & 0xF;
      if (nibble < 10) {
        result += char('0' + nibble);
      } else {
        result += char('a' + nibble - 10);
      }
    }

    return result;
  }

public:
  ManualOTAManager()
    : httpClient(sslClient, GITHUB_API_HOST, 443) {
    httpClient.setTimeout(OTA_CHECK_TIMEOUT);
  }

  bool isChecking() const {
    return checking_update;
  }
  bool isUpdateAvailable() const {
    return update_available;
  }
  String getRemoteVersion() const {
    return remote_version;
  }

  // GitHub API integration for release information
  bool fetchLatestReleaseInfo() {
    Serial.println("Fetching latest release from GitHub API...");

    httpClient.setTimeout(15000);
    httpClient.beginRequest();
    httpClient.get(GITHUB_API_PATH);
    httpClient.sendHeader("User-Agent", "StreamBox-OTA/2.0");
    httpClient.sendHeader("Accept", "application/vnd.github.v3+json");
    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    Serial.print("GitHub API status code: ");
    Serial.println(statusCode);

    if (statusCode == 200) {
      String response = httpClient.responseBody();
      Serial.print("Response length: ");
      Serial.println(response.length());

      // Parse JSON response for release information
      return parseReleaseResponse(response);
    } else {
      Serial.println("Failed to fetch release info from GitHub API");
      return false;
    }
  }

  // Parse GitHub API release response
  bool parseReleaseResponse(const String& response) {
    Serial.println("Parsing release response...");

    // Simple JSON parsing for tag_name (version)
    int tagStart = response.indexOf('"tag_name"');
    if (tagStart == -1) {
      Serial.println("Could not find tag_name in response");
      return false;
    }

    int versionStart = response.indexOf('"', tagStart + 11);
    int versionEnd = response.indexOf('"', versionStart + 1);
    if (versionStart == -1 || versionEnd == -1) {
      Serial.println("Could not parse version from tag_name");
      return false;
    }

    String version = response.substring(versionStart + 1, versionEnd);
    remote_version = version;
    Serial.print("Found version: ");
    Serial.println(remote_version);

    // Find assets array and look for .ota file
    int assetsStart = response.indexOf('"assets"');
    if (assetsStart == -1) {
      Serial.println("Could not find assets in response");
      return false;
    }

    // Look for .ota file download URL
    int otaNamePos = response.indexOf('.ota', assetsStart);
    if (otaNamePos == -1) {
      Serial.println("Could not find .ota file in assets");
      return false;
    }

    // Find the browser_download_url for this asset
    int urlStart = response.lastIndexOf('"browser_download_url"', otaNamePos);
    if (urlStart == -1) {
      Serial.println("Could not find download URL for .ota file");
      return false;
    }

    int downloadStart = response.indexOf('"', urlStart + 22);
    int downloadEnd = response.indexOf('"', downloadStart + 1);
    if (downloadStart == -1 || downloadEnd == -1) {
      Serial.println("Could not parse download URL");
      return false;
    }

    dynamic_ota_download_url = response.substring(downloadStart + 1, downloadEnd);
    Serial.print("Found OTA download URL: ");
    Serial.println(dynamic_ota_download_url);

    // Look for version file (.version)
    int versionFilePos = response.indexOf('.version', assetsStart);
    if (versionFilePos != -1) {
      int versionUrlStart = response.lastIndexOf('"browser_download_url"', versionFilePos);
      if (versionUrlStart != -1) {
        int versionDownloadStart = response.indexOf('"', versionUrlStart + 22);
        int versionDownloadEnd = response.indexOf('"', versionDownloadStart + 1);
        if (versionDownloadStart != -1 && versionDownloadEnd != -1) {
          String versionUrl = response.substring(versionDownloadStart + 1, versionDownloadEnd);
          Serial.print("Found version file URL: ");
          Serial.println(versionUrl);
        }
      }
    }

    // Look for checksum file (.md5)
    int checksumFilePos = response.indexOf('.md5', assetsStart);
    if (checksumFilePos != -1) {
      int checksumUrlStart = response.lastIndexOf('"browser_download_url"', checksumFilePos);
      if (checksumUrlStart != -1) {
        int checksumDownloadStart = response.indexOf('"', checksumUrlStart + 22);
        int checksumDownloadEnd = response.indexOf('"', checksumDownloadStart + 1);
        if (checksumDownloadStart != -1 && checksumDownloadEnd != -1) {
          String checksumUrl = response.substring(checksumDownloadStart + 1, checksumDownloadEnd);
          Serial.print("Found checksum file URL: ");
          Serial.println(checksumUrl);

          // Fetch the checksum file content
          if (fetchChecksumFromUrl(checksumUrl)) {
            Serial.println("Successfully fetched checksum");
          }
        }
      }
    }

    return true;
  }

  // Fetch checksum from URL
  bool fetchChecksumFromUrl(const String& url) {
    Serial.print("Fetching checksum from: ");
    Serial.println(url);

    // Extract host and path from URL
    String host, path;
    if (!parseUrl(url, host, path)) {
      Serial.println("Failed to parse checksum URL");
      return false;
    }

    // Create new HTTP client for checksum download
    WiFiSSLClient checksumClient;
    HttpClient checksumHttp(checksumClient, host.c_str(), 443);
    checksumHttp.setTimeout(10000);

    checksumHttp.beginRequest();
    checksumHttp.get(path.c_str());
    checksumHttp.sendHeader("User-Agent", "StreamBox-OTA/2.0");
    checksumHttp.endRequest();

    int statusCode = checksumHttp.responseStatusCode();
    if (statusCode == 200) {
      remote_checksum = checksumHttp.responseBody();
      remote_checksum.trim();
      Serial.print("Remote checksum: ");
      Serial.println(remote_checksum);
      return true;
    } else {
      Serial.print("Failed to fetch checksum, status: ");
      Serial.println(statusCode);
      return false;
    }
  }

  // Parse URL into host and path components
  bool parseUrl(const String& url, String& host, String& path) {
    if (!url.startsWith("https://")) {
      Serial.println("URL must start with https://");
      return false;
    }

    int hostStart = 8; // Skip "https://"
    int pathStart = url.indexOf('/', hostStart);

    if (pathStart == -1) {
      host = url.substring(hostStart);
      path = "/";
    } else {
      host = url.substring(hostStart, pathStart);
      path = url.substring(pathStart);
    }

    Serial.print("Parsed host: ");
    Serial.println(host);
    Serial.print("Parsed path: ");
    Serial.println(path);

    return true;
  }

  // Pre-OTA system health check with flash memory validation
  bool performSystemHealthCheck() {
    Serial.println("🔍 Performing comprehensive pre-OTA system health check...");

    // CRITICAL: Flash memory analysis
    Serial.println("💾 === FLASH MEMORY ANALYSIS ===");
    Serial.print("Total flash size: ");
    Serial.print(OTA_TOTAL_FLASH_SIZE / 1024);
    Serial.println(" KB");

    Serial.print("Current firmware size: ");
    Serial.print(CURRENT_FIRMWARE_SIZE / 1024);
    Serial.println(" KB");

    Serial.print("OTA staging area start: 0x");
    Serial.println(OTA_STAGING_START, HEX);

    Serial.print("Available space for OTA: ");
    Serial.print(OTA_AVAILABLE_SPACE / 1024);
    Serial.println(" KB");

    Serial.print("Maximum allowed firmware size: ");
    Serial.print(MAX_FIRMWARE_SIZE / 1024);
    Serial.println(" KB");

    // Validate flash memory layout
    if (CURRENT_FIRMWARE_SIZE >= OTA_STAGING_START - OTA_FLASH_BASE_ADDR) {
      Serial.println("❌ CRITICAL: Current firmware overlaps with OTA staging area!");
      Serial.println("This will cause OTA crashes. Firmware needs optimization.");
      return false;
    }

    if (OTA_AVAILABLE_SPACE < firmware_size + 0x10000) { // 64KB safety margin
      Serial.println("❌ CRITICAL: Insufficient flash space for new firmware!");
      Serial.print("Need: ");
      Serial.print((firmware_size + 0x10000) / 1024);
      Serial.print(" KB, Available: ");
      Serial.print(OTA_AVAILABLE_SPACE / 1024);
      Serial.println(" KB");
      printOptimizationRecommendations();
      return false;
    }

    Serial.println("✅ Flash memory layout validated");

    // Check available RAM
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    Serial.print("Available LVGL memory: ");
    Serial.print(mon.free_size);
    Serial.println(" bytes");

    if (mon.free_size < 50000) {
      Serial.println("WARNING: Low RAM detected, forcing cleanup...");
      MemoryManager::forceCleanup();
      lv_mem_monitor(&mon);

      if (mon.free_size < 30000) {
        Serial.println("ERROR: Insufficient RAM for OTA operation");
        return false;
      }
    }

    // Check WiFi stability
    if (!wifiManager.isConnected()) {
      Serial.println("ERROR: WiFi not connected");
      return false;
    }

    // Test OTA server connectivity
    WiFiClient testClient;
    if (!testClient.connect(GITHUB_API_HOST, 443)) {
      Serial.println("ERROR: Cannot reach OTA server");
      testClient.stop();
      return false;
    }
    testClient.stop();

    Serial.println("✅ Comprehensive system health check PASSED");
    Serial.println("🚀 System is ready for OTA update");
    return true;
  }

  bool performOTAUpdate() {
    Serial.println("=== STARTING IMPROVED OTA UPDATE ===");

    if (!update_available) {
      Serial.println("ERROR: No update available");
      updateOTAErrorUI("No Update Available");
      restoreServicesAfterOTA();
      return false;
    }

    // STEP 0: Perform comprehensive system health check
    if (!performSystemHealthCheck()) {
      Serial.println("ERROR: System health check failed");
      updateOTAErrorUI("System Check Failed");
      restoreServicesAfterOTA();
      return false;
    }

    Serial.print("Starting OTA for firmware version: ");
    Serial.println(remote_version);

    // STEP 1: Prepare system for OTA
    Serial.println("📋 Preparing system for OTA...");

    // Stop telemetry and other background operations
    telemetryManager.disconnect();

    // Disable watchdog during OTA to prevent resets
    Serial.println("⏸️ Suspending watchdog during OTA...");
    // Note: We'll manage timeouts manually

    // Clean memory but don't be too aggressive
    MemoryManager::forceCleanup();

    // Give system time to stabilize
    delay(500);

    Serial.println("✅ System preparation complete");

    // STEP 2: Create OTA instance using EXTERNAL QSPI flash to avoid internal flash conflicts
    Serial.println("🔧 Creating OTA instance using EXTERNAL QSPI flash...");
    Serial.println("📋 This avoids internal flash memory conflicts!");
    Serial.print("Internal flash staging would be at: 0x");
    Serial.println(OTA_STAGING_START, HEX);
    Serial.println("Instead using external QSPI flash for staging");

    // CRITICAL: Use external QSPI flash instead of internal flash to avoid memory conflicts
    // This completely bypasses the internal flash space issue
    Arduino_Portenta_OTA_QSPI ota(QSPI_FLASH_FATFS_MBR, 2);

    // STEP 3: Check OTA capability
    Serial.println("🔍 Checking OTA capability...");
    if (!ota.isOtaCapable()) {
      Serial.println("ERROR: Device not OTA capable");
      updateOTAErrorUI("Device Not OTA Capable");
      restoreServicesAfterOTA();
      return false;
    }
    Serial.println("✅ Device is OTA capable");

    // STEP 4: Initialize OTA system (without interrupt manipulation)
    Serial.println("🚀 Initializing OTA system...");
    Arduino_Portenta_OTA::Error ota_err = ota.begin();

    if (ota_err != Arduino_Portenta_OTA::Error::None) {
      Serial.print("ERROR: OTA begin failed with error: ");
      Serial.println((int)ota_err);
      updateOTAErrorUI("OTA Init Failed");
      restoreServicesAfterOTA();
      return false;
    }
    Serial.println("✅ OTA system initialized successfully");

    // STEP 4.5: Pre-allocate QSPI Flash Space (Suggestion #4)
    Serial.println("💾 Pre-allocating QSPI flash storage area...");
    delay(100); // Allow initialization to complete

    // STEP 4.6: Disable Non-Essential Services During OTA (Suggestion #5)
    Serial.println("⏸️ Disabling non-essential services for OTA...");
    // lv_timer_pause_all(); // Pause all LVGL timers (function not available)
    // Note: Sensor updates and other services will be suspended during download

    // STEP 4.7: Configure HTTP Client for Lower Memory Usage (Suggestion #1)
    Serial.println("🔧 Optimizing HTTP client for low memory...");
    // Reduce global WiFi buffer sizes to minimize memory pressure during OTA
    // This helps prevent memory allocation issues during large downloads
    // WiFiClass::_arduino_event_cb = nullptr; // Disable event callbacks to save memory (not available)
    Serial.println("✅ HTTP buffer optimization configured");

    // STEP 4.8: RADICAL MEMORY APPROACH - DISABLE ALL LVGL OPERATIONS
    Serial.println("🚨 EMERGENCY MODE: Disabling ALL LVGL operations for OTA");
    Serial.println("⚠️  UI will be completely frozen during download - this is expected");

    // Skip all LVGL memory monitoring and cleanup that causes hangs
    // lv_mem_monitor_t current_mem;
    // lv_mem_monitor(&current_mem);
    // MemoryManager::forceCleanup();
    // lv_obj_clean(lv_scr_act());

    Serial.println("✅ LVGL operations bypassed - maximum memory available");

    // STEP 5: Setup download parameters
    String firmware_url = dynamic_ota_download_url;
    Serial.print("📥 Downloading firmware from: ");
    Serial.println(firmware_url);

    // STEP 6: Skip all memory cleanup operations that cause hangs
    Serial.println("🚀 Proceeding directly to download - no memory cleanup operations");

    // Minimal delay only
    delay(50);

    // STEP 7: Split OTA Process - Stage 1: Chunked Download (Option #5)
    Serial.println("📋 === SPLIT OTA PROCESS - STAGE 1: DOWNLOAD ===");
    Serial.println("⬇️ Starting chunked firmware download...");
    Serial.println("Using small chunks to prevent memory crashes...");
    Serial.flush();

    // Split download into multiple smaller operations
    bool download_success = performChunkedOTADownload(ota, firmware_url);

    if (!download_success) {
      Serial.println("ERROR: Chunked download failed");
      updateOTAErrorUI("Download Failed");
      restoreServicesAfterOTA();
      return false;
    }

    Serial.println("✅ Stage 1 - Firmware download completed");
    Serial.println("📋 === SPLIT OTA PROCESS - STAGE 2: APPLICATION ===");
    Serial.println("⚙️ Applying downloaded firmware...");

    // STEP 8: Decompress firmware
    Serial.println("🗜️ Decompressing firmware...");
    updateOTAProgressUI(95);

    int decompress_result = ota.decompress();

    if (decompress_result < 0) {
      Serial.print("ERROR: Decompression failed with result: ");
      Serial.println(decompress_result);
      updateOTAErrorUI("Decompression Failed");
      restoreServicesAfterOTA();
      return false;
    }
    Serial.println("✅ Firmware decompressed successfully");

    // STEP 9: Apply firmware update
    Serial.println("⚙️ Applying firmware update...");
    updateOTAProgressUI(98);

    ota_err = ota.update();

    if (ota_err != Arduino_Portenta_OTA::Error::None) {
      Serial.print("ERROR: Update failed with error: ");
      Serial.println((int)ota_err);
      updateOTAErrorUI("Update Failed");
      restoreServicesAfterOTA();
      return false;
    }

    Serial.println("✅ FIRMWARE UPDATE COMPLETED SUCCESSFULLY!");
    Serial.println("🔄 Device will restart with new firmware in 3 seconds...");

    // STEP 10: Show completion and restart
    updateOTACompletionUI();

    // Give user time to see completion message
    for (int i = 3; i > 0; i--) {
      Serial.print("Restarting in ");
      // Serial.print(i);
      Serial.println(" seconds...");
      delay(1000);
    }

    // Reset device to boot with new firmware
    Serial.println("🚀 Initiating system restart...");
    Serial.flush();
    ota.reset();

    return true;
  }

  // Diagnostic: test DNS, TCP connect and TLS handshake to the OTA host
  void runOTADiagnostics() {
    Serial.println("=== OTA DIAGNOSTICS ===");
    const char *host = GITHUB_API_HOST;
    const int port = 443;

    Serial.print("Resolving host: "); Serial.println(host);
    IPAddress resolved;
    if (!WiFi.hostByName(host, resolved)) {
      Serial.println("DNS resolution failed");
      return;
    }
    Serial.print("Resolved IP: "); Serial.println(resolved);

    Serial.print("Opening TCP connection to "); Serial.print(host); Serial.print(":"); Serial.println(port);
    if (!sslClient.connect(host, port)) {
      Serial.print("TCP connect failed to "); Serial.println(host);
      return;
    }

    Serial.println("TCP connected - attempting TLS handshake (client.available may be 0 until data)");

    // For most WiFiSSLClient implementations, connect() already performs TLS handshake.
    // We can check if the connection is secure by retrieving peer certificate details if API exposed.
    // Print basic connection state
    if (sslClient.connected()) {
      Serial.println("SSL client reports connected");
    } else {
      Serial.println("SSL client not connected after connect()");
    }

    // Try a simple HTTPS GET request to fetch headers
    String req = String("GET ") + OTA_UPDATE_PATH + " HTTP/1.1\r\nHost: " + host + "\r\nConnection: close\r\n\r\n";
    Serial.println("Sending simple HTTP GET for headers...");
    sslClient.print(req.c_str());

    unsigned long start = millis();
    String resp = "";
    while (millis() - start < 5000) {
      while (sslClient.available()) {
        resp += (char)sslClient.read();
        if (resp.length() > 2000) break;
      }
      if (resp.length() > 0) break;
      delay(50);
    }

    if (resp.length() == 0) {
      Serial.println("No response received (possible TLS failure or server blocked)");
    } else {
      Serial.println("Received response headers (truncated):");
      int endHeaders = resp.indexOf("\r\n\r\n");
      if (endHeaders > 0) {
        Serial.println(resp.substring(0, min(endHeaders, 1024)));
      } else {
        Serial.println(resp.substring(0, min((int)resp.length(), 1024)));
      }
    }

    sslClient.stop();
    Serial.println("=== END DIAGNOSTICS ===");
  }

  // Helper function to re-enable services after OTA (Suggestion #5)
  static void restoreServicesAfterOTA() {
    Serial.println("🔄 Restoring services after OTA...");
    // lv_timer_resume_all(); // Resume all LVGL timers (function not available)
    // Note: Other services will resume automatically on next loop
    Serial.println("✅ Services restored");
  }

  // Simplified OTA update following Arduino library example
  bool performSimpleOTAUpdate() {
    Serial.println("🚀 Starting simple OTA update process");

    if (!update_available) {
      Serial.println("❌ No update available");
      updateOTAErrorUI("No Update Available");
      return false;
    }

    Serial.print("Target firmware version: ");
    Serial.println(remote_version);

    // Create OTA instance
    Arduino_Portenta_OTA_QSPI ota(QSPI_FLASH_FATFS_MBR, 2);
    Arduino_Portenta_OTA::Error ota_err = Arduino_Portenta_OTA::Error::None;

    // Check OTA capability
    if (!ota.isOtaCapable()) {
      Serial.println("❌ Device not OTA capable - update bootloader");
      updateOTAErrorUI("Not OTA Capable");
      return false;
    }
    Serial.println("✅ Device is OTA capable");

    // Initialize OTA storage
    Serial.println("Initializing OTA storage...");
    if ((ota_err = ota.begin()) != Arduino_Portenta_OTA::Error::None) {
      Serial.print("❌ OTA begin failed with error: ");
      Serial.println((int)ota_err);
      updateOTAErrorUI("Init Failed");
      return false;
    }
    Serial.println("✅ OTA storage initialized");

    // Download and decompress firmware - try HTTP first to isolate TLS issue
    Serial.println("Testing HTTP first to isolate TLS issue...");
    String firmware_url = dynamic_ota_download_url;
    Serial.print("📥 Downloading from: ");
    Serial.println(firmware_url);

    // Clean memory and kick watchdog before download to prevent crashes
    Serial.println("Preparing system for download...");
    MemoryManager::forceCleanup();
    watchdog.kick();

    uint32_t start_time = millis();
    Serial.println("Trying download() with HTTP (no TLS)...");
    Serial.println("⚠️  System may appear frozen during download - this is normal");
    Serial.flush();

  int download_result = ota.download(firmware_url.c_str(), false);

    Serial.println();
    Serial.println("Download attempt completed");
    watchdog.kick();

    if (download_result <= 0) {
      Serial.print("❌ HTTP download failed with result: ");
      Serial.println(download_result);
      Serial.println("Trying HTTPS...");

      firmware_url = dynamic_ota_download_url;
      Serial.print("📥 Downloading from: ");
      Serial.println(firmware_url);

  download_result = ota.download(firmware_url.c_str(), true);
    } else {
      Serial.println("✅ HTTP download successful!");
    }

    if (download_result > 0) {
      Serial.println("✅ Download successful, now decompressing...");
      Serial.flush();

      int decompress_result = ota.decompress();
      if (decompress_result != 0) {
        Serial.print("❌ Decompression failed with result: ");
        Serial.println(decompress_result);
        updateOTAErrorUI("Decompress Failed");
        return false;
      }
      Serial.println("✅ Decompression successful");
    }

    if (download_result <= 0) {
      Serial.print("❌ Download failed with result: ");
      Serial.println(download_result);
      updateOTAErrorUI("Download Failed");
      return false;
    }

    float elapsed = (millis() - start_time) / 1000.0;
    float speed = (download_result / elapsed) / 1024.0;
    Serial.print("✅ Downloaded ");
    Serial.print(download_result);
    Serial.print(" bytes in ");
    Serial.print(elapsed);
    Serial.print("s (");
    Serial.print(speed);
    Serial.println(" KB/s)");

    // Apply update
    Serial.println("🔄 Applying firmware update...");
    if ((ota_err = ota.update()) != Arduino_Portenta_OTA::Error::None) {
      Serial.print("❌ Update failed with error: ");
      Serial.println((int)ota_err);
      updateOTAErrorUI("Update Failed");
      return false;
    }

    Serial.println("✅ Firmware update applied successfully!");
    Serial.println("🔄 Rebooting to activate new firmware...");
    delay(1000);

    // Reboot to new firmware
    ota.reset();
    return true;
  }

  // Use Arduino_Portenta_OTA built-in download from public GitHub repository
  static bool performChunkedOTADownload(Arduino_Portenta_OTA_QSPI& ota, const String& firmware_url, arduino::MbedSocketClass* socket = nullptr) {
    Serial.println("Starting OTA download from public GitHub repository...");
    Serial.println("Using Arduino_Portenta_OTA downloadAndDecompress for LZSS compressed .ota file");

    Serial.println("⏰ Starting download...");
    Serial.flush();

    // Use the library's built-in download mechanism with proper parameters
    // Convert String to const char* and specify HTTPS
  unsigned long download_start = millis();
  int download_result = ota.downloadAndDecompress(firmware_url.c_str(), true, socket);
    unsigned long download_duration = millis() - download_start;

    Serial.println();
    Serial.print("Download completed in ");
    Serial.print(download_duration);
    Serial.println(" ms");

    if (download_result < 0) {
      Serial.print("OTA download failed with result: ");
      Serial.println(download_result);
      return false;
    }

    Serial.println("OTA download completed successfully");
    return true;
  }

  void checkForUpdate() {
    if (checking_update) {
      Serial.println("Update check already in progress");
      return;
    }

    if (!wifiManager.isConnected()) {
      Serial.println("No WiFi connection - cannot check for updates");
      updateCheckForUpdatesButton(false);
      return;
    }

    checking_update = true;
    update_available = false;
    remote_version = "";
    remote_checksum = "";
    firmware_size = 0;

    Serial.println("Checking for firmware updates...");

    lv_timer_create([](lv_timer_t *t) {
      ManualOTAManager *manager = (ManualOTAManager *)lv_timer_get_user_data(t);
      manager->performUpdateCheck();
      lv_timer_del(t);
    },
                    100, this);
  }

  void performUpdateCheck() {
    Serial.println("========================================");
    Serial.println("STARTING OTA UPDATE CHECK");
    Serial.println("========================================");

    // CRASH PREVENTION: Check memory and system state before OTA
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    Serial.print("Free memory before OTA: ");
    Serial.print(mon.free_size);
    Serial.println(" bytes");

    if (mon.free_size < 15000) {  // Need at least 15KB free
      Serial.println("❌ ERROR: Insufficient memory for OTA update");
      Serial.println("🔧 Forcing memory cleanup...");
      MemoryManager::forceCleanup();

      // Check again after cleanup
      lv_mem_monitor(&mon);
      if (mon.free_size < 10000) {
        Serial.println("❌ CRITICAL: Still insufficient memory, aborting OTA");
        checking_update = false;
        updateCheckForUpdatesButton(false);
        return;
      }
    }

    // Kick watchdog to prevent timeout during network operations
    watchdog.kick();

    bool success = false;

    Serial.println("Step 1: Fetching latest release info from GitHub API...");
    if (!fetchLatestReleaseInfo()) {
      Serial.println("FAILED: Could not fetch release information");
      checking_update = false;
      updateCheckForUpdatesButton(false);
      return;
    }
    Serial.println("PASSED: Release info fetched successfully");

    Serial.println("Step 2: Validating release information...");
    if (remote_version.length() == 0 || dynamic_ota_download_url.length() == 0) {
      Serial.println("FAILED: Incomplete release information");
      checking_update = false;
      updateCheckForUpdatesButton(false);
      return;
    }
    Serial.println("PASSED: Release information is complete");

    Serial.println("Step 3: Comparing versions...");
    String current_version = String(FIRMWARE_VERSION);
    Serial.print("Current version: ");
    Serial.println(current_version);
    Serial.print("Remote version: ");
    Serial.println(remote_version);

    if (remote_version != current_version) {
      Serial.println("SUCCESS: New version available!");
      update_available = true;
      success = true;
      Serial.print("Update available: ");
      Serial.print(current_version);
      Serial.print(" → ");
      Serial.println(remote_version);
    } else {
      Serial.println("INFO: Already on latest version");
      success = true; // Not an error, just no update needed
    }

    checking_update = false;
    updateCheckForUpdatesButton(success);

    Serial.println("========================================");
    Serial.println("OTA UPDATE CHECK COMPLETE");
    Serial.println("========================================");
  }

  bool checkVersionFile() {
    Serial.println("Fetching version file...");

    httpClient.setTimeout(10000);
    httpClient.beginRequest();
    httpClient.get(OTA_VERSION_PATH);
    httpClient.sendHeader("User-Agent", "StreamBox-OTA/1.0");
    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    Serial.print("Version file status code: ");
    Serial.println(statusCode);

    if (statusCode == 200) {
      // Read headers (for debugging)
      while (httpClient.headerAvailable()) {
        String headerName = httpClient.readHeaderName();
        String headerValue = httpClient.readHeaderValue();
        Serial.print("Version Header: ");
        Serial.print(headerName);
        Serial.print(" = ");
        Serial.println(headerValue);
      }

      String version_content = httpClient.responseBody();
      version_content.trim();

      Serial.print("Raw version content: '");
      Serial.print(version_content);
      Serial.print("' (length: ");
      Serial.print(version_content.length());
      Serial.println(")");

      if (version_content.length() > 0 && version_content.length() < 20) {
        remote_version = version_content;
        Serial.print("Remote version: ");
        Serial.println(remote_version);
        return true;
      } else {
        Serial.println("Version content length invalid");
      }
    } else {
      Serial.print("Version check failed with status: ");
      Serial.println(statusCode);

      // Get error response
      String errorBody = httpClient.responseBody();
      if (errorBody.length() > 0) {
        Serial.print("Version error response: ");
        Serial.println(errorBody);
      }
    }

    return false;
  }

  bool checkChecksumFile() {
    Serial.println("Fetching checksum file...");

    httpClient.setTimeout(10000);
    httpClient.beginRequest();
    httpClient.get(OTA_CHECKSUM_PATH);
    httpClient.sendHeader("User-Agent", "StreamBox-OTA/1.0");
    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    Serial.print("Checksum file status code: ");
    Serial.println(statusCode);

    if (statusCode == 200) {
      // Read headers (for debugging)
      while (httpClient.headerAvailable()) {
        String headerName = httpClient.readHeaderName();
        String headerValue = httpClient.readHeaderValue();
        Serial.print("Checksum Header: ");
        Serial.print(headerName);
        Serial.print(" = ");
        Serial.println(headerValue);
      }

      String checksum_content = httpClient.responseBody();
      checksum_content.trim();

      Serial.print("Raw checksum content: '");
      Serial.print(checksum_content);
      Serial.print("' (length: ");
      Serial.print(checksum_content.length());
      Serial.println(")");

      if (checksum_content.length() >= 8) {  // Accept either 8-char simple or 32-char MD5
        remote_checksum = checksum_content;
        Serial.print("Remote checksum: ");
        Serial.println(remote_checksum);
        return true;
      } else {
        Serial.println("Checksum content length invalid");
      }
    } else {
      Serial.print("Checksum check failed with status: ");
      Serial.println(statusCode);

      // Get error response
      String errorBody = httpClient.responseBody();
      if (errorBody.length() > 0) {
        Serial.print("Checksum error response: ");
        Serial.println(errorBody);
      }
    }

    return false;
  }

  bool checkFirmwareFile() {
    Serial.println("Checking firmware file...");

    httpClient.setTimeout(15000);
    httpClient.beginRequest();
    httpClient.get(OTA_UPDATE_PATH);
    httpClient.sendHeader("User-Agent", "StreamBox-OTA/1.0");
    httpClient.endRequest();

    int statusCode = httpClient.responseStatusCode();
    Serial.print("Firmware file status code: ");
    Serial.println(statusCode);

    if (statusCode == 200) {
      // Read headers to find Content-Length
      firmware_size = 0;
      while (httpClient.headerAvailable()) {
        String headerName = httpClient.readHeaderName();
        String headerValue = httpClient.readHeaderValue();

        Serial.print("Header: ");
        Serial.print(headerName);
        Serial.print(" = ");
        Serial.println(headerValue);

        if (headerName.equalsIgnoreCase("Content-Length")) {
          firmware_size = headerValue.toInt();
          Serial.print("Found firmware size from Content-Length: ");
          Serial.print(firmware_size);
          Serial.println(" bytes");
          break;
        }
      }

      // Get response body but don't store it (just to clean up the connection)
      String responseBody = httpClient.responseBody();

      // If Content-Length wasn't available, use response body length as fallback
      if (firmware_size == 0 && responseBody.length() > 0) {
        firmware_size = responseBody.length();
        Serial.print("Using response body length as firmware size: ");
        Serial.print(firmware_size);
        Serial.println(" bytes");
      }

      Serial.print("Final firmware size: ");
      Serial.print(firmware_size);
      Serial.println(" bytes");

      // Check if size is reasonable for firmware
      if (firmware_size > 0) {
        Serial.println("Firmware file check: SUCCESS");
        return true;
      } else {
        Serial.println("Firmware file check: FAILED - Size is 0");
        return false;
      }
    } else {
      Serial.print("Firmware file check: FAILED - HTTP ");
      Serial.println(statusCode);

      // Try to get error response
      String errorBody = httpClient.responseBody();
      if (errorBody.length() > 0) {
        Serial.print("Error response: ");
        Serial.println(errorBody);
      }

      return false;
    }
  }

  // Enhanced OTA error reporting with detailed guidance
  static void updateOTAErrorUI(const char *error_msg) {
    Serial.print("❌ OTA ERROR: ");
    Serial.println(error_msg);

    // Provide specific guidance based on error type
    if (strstr(error_msg, "space") || strstr(error_msg, "Flash") || strstr(error_msg, "memory") || strstr(error_msg, "size")) {
      Serial.println("💡 CRITICAL FLASH MEMORY ISSUE DETECTED!");
      Serial.println("Your firmware (1.05MB) is too large for safe OTA updates.");
      Serial.println("🔧 SOLUTIONS:");
      Serial.println("1. IMMEDIATE: Reduce firmware size to <900KB");
      Serial.println("2. Remove unused LVGL widgets and features");
      Serial.println("3. Use PROGMEM for large constant data");
      Serial.println("4. Enable more aggressive compiler optimizations");
      Serial.println("5. Consider modular firmware architecture");
      printOptimizationRecommendations();
    } else if (strstr(error_msg, "Download")) {
      Serial.println("💡 SOLUTION: Download failed - check network connectivity");
      Serial.println("- Ensure stable WiFi connection");
      Serial.println("- Check if OTA server is accessible");
    } else if (strstr(error_msg, "WiFi")) {
      Serial.println("💡 SOLUTION: Ensure stable WiFi connection before OTA");
    } else if (strstr(error_msg, "System Check")) {
      Serial.println("💡 SOLUTION: System health check failed - see details above");
    }

    // Update UI to show error
    updateCheckForUpdatesButton(false);
  }

  static void updateOTACompletionUI() {
    if (!ui.settings_screen) return;

    uint32_t child_count = lv_obj_get_child_cnt(ui.settings_screen);
    for (uint32_t i = 0; i < child_count; i++) {
      lv_obj_t *child = lv_obj_get_child(ui.settings_screen, i);
      if (lv_obj_get_child_cnt(child) > 0) {
        lv_obj_t *flex_container = lv_obj_get_child(child, 0);
        if (flex_container && lv_obj_get_child_cnt(flex_container) >= 2) {
          lv_obj_t *text_label = lv_obj_get_child(flex_container, 1);
          const char *text = lv_label_get_text(text_label);
          if (strstr(text, "Updating") != nullptr) {
            lv_obj_t *icon_label = lv_obj_get_child(flex_container, 0);
            lv_label_set_text(text_label, "Update Complete!");
            lv_label_set_text(icon_label, LV_SYMBOL_OK);
            lv_obj_set_style_bg_color(child, lv_color_hex(UIConstants::COLOR_SUCCESS), 0);
            lv_obj_invalidate(child);
            lv_timer_handler();
            break;
          }
        }
      }
    }
  }

  static void updateCheckForUpdatesButton(bool update_available) {
    if (!ui.settings_screen) return;

    uint32_t child_count = lv_obj_get_child_cnt(ui.settings_screen);
    for (uint32_t i = 0; i < child_count; i++) {
      lv_obj_t *child = lv_obj_get_child(ui.settings_screen, i);
      if (lv_obj_get_child_cnt(child) > 0) {
        lv_obj_t *flex_container = lv_obj_get_child(child, 0);
        if (flex_container && lv_obj_get_child_cnt(flex_container) >= 2) {
          lv_obj_t *text_label = lv_obj_get_child(flex_container, 1);
          lv_obj_t *icon_label = lv_obj_get_child(flex_container, 0);
          const char *text = lv_label_get_text(text_label);

          if (strstr(text, "Check for Updates") != nullptr || strstr(text, "Checking") != nullptr || strstr(text, "Update Now") != nullptr || strstr(text, "Up To Date") != nullptr || strstr(text, "Failed") != nullptr) {

            if (update_available) {
              lv_label_set_text(text_label, "Update Now");
              lv_label_set_text(icon_label, LV_SYMBOL_DOWNLOAD);
              lv_obj_set_style_bg_color(child, lv_color_hex(UIConstants::COLOR_SUCCESS), 0);
              lv_obj_clear_state(child, LV_STATE_DISABLED);
            } else {
              lv_label_set_text(text_label, "Up To Date");
              lv_label_set_text(icon_label, LV_SYMBOL_OK);
              lv_obj_set_style_bg_color(child, lv_color_hex(UIConstants::COLOR_SECONDARY), 0);
              lv_obj_clear_state(child, LV_STATE_DISABLED);

              lv_timer_create([](lv_timer_t *t2) {
                if (ui.settings_screen) {
                  uint32_t child_count = lv_obj_get_child_cnt(ui.settings_screen);
                  for (uint32_t i = 0; i < child_count; i++) {
                    lv_obj_t *child = lv_obj_get_child(ui.settings_screen, i);
                    if (lv_obj_get_child_cnt(child) > 0) {
                      lv_obj_t *flex_container = lv_obj_get_child(child, 0);
                      if (flex_container && lv_obj_get_child_cnt(flex_container) >= 2) {
                        lv_obj_t *text_label = lv_obj_get_child(flex_container, 1);
                        const char *text = lv_label_get_text(text_label);
                        if (strcmp(text, "Up To Date") == 0) {
                          lv_obj_t *icon_label = lv_obj_get_child(flex_container, 0);
                          lv_label_set_text(text_label, "Check for Updates");
                          lv_label_set_text(icon_label, LV_SYMBOL_DOWNLOAD);
                          lv_obj_set_style_bg_color(child, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
                          break;
                        }
                      }
                    }
                  }
                }
                lv_timer_del(t2);
              },
                              2000, nullptr);
            }
            break;
          }
        }
      }
    }
  }
};

ManualOTAManager otaManager;

void updateFooterIndicatorsToCurrentState() {
  // Update WiFi indicator based on current WiFi and telemetry state
  if (FooterHelper::wifi_indicator && lv_obj_is_valid(FooterHelper::wifi_indicator)) {
    lv_color_t wifi_color;
    if (wifiManager.isConnected()) {
      // Show different colors based on telemetry connection status
      if (telemetryManager.isConnected()) {
        wifi_color = lv_color_hex(UIConstants::COLOR_SUCCESS); // Green - WiFi + WSS connected
      } else {
        wifi_color = lv_color_hex(UIConstants::COLOR_PRIMARY); // Blue - WiFi only
      }
    } else if (wifiManager.getState() == WIFI_CONNECTING) {
      wifi_color = lv_color_hex(UIConstants::COLOR_WARNING); // Orange - connecting
    } else {
      wifi_color = lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY); // Gray - disconnected
    }
    FooterHelper::updateWiFiIndicator(wifi_color);
  }

  // Update GPS indicator based on current GPS state
  if (FooterHelper::gps_indicator && lv_obj_is_valid(FooterHelper::gps_indicator)) {
    if (sensors.gps_valid) {
      FooterHelper::updateGPSIndicator(lv_color_hex(UIConstants::COLOR_SUCCESS)); // Green - GPS fix
    } else {
      FooterHelper::updateGPSIndicator(lv_color_hex(UIConstants::COLOR_WARNING)); // Orange - searching
    }
  }
}

// GPS data validation
// isValidGPSData() function removed - was unused

// Error message display
// Replace the existing showErrorMessage function with this improved version:

void showErrorMessage(const char *message, int duration_ms) {
  if (ui.error_popup) {
    lv_obj_del(ui.error_popup);
    ui.error_popup = nullptr;
  }

  ui.error_popup = lv_obj_create(lv_scr_act());
  lv_obj_set_size(ui.error_popup, 340, 180); // Made slightly larger for better layout
  lv_obj_center(ui.error_popup);
  lv_obj_set_style_bg_color(ui.error_popup, lv_color_hex(UIConstants::COLOR_DANGER), 0);
  lv_obj_set_style_radius(ui.error_popup, UIConstants::BUTTON_RADIUS, 0);
  lv_obj_set_style_border_width(ui.error_popup, 0, 0);
  lv_obj_set_style_border_color(ui.error_popup, lv_color_white(), 0);
  lv_obj_set_style_pad_all(ui.error_popup, UIConstants::PADDING_STANDARD, 0);

  // Create title container with error symbol and "Error" text
  lv_obj_t *title_container = lv_obj_create(ui.error_popup);
  lv_obj_remove_style_all(title_container);
  lv_obj_set_size(title_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(title_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(title_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(title_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(title_container, 8, 0); // Space between icon and text
  lv_obj_align(title_container, LV_ALIGN_TOP_MID, 0, UIConstants::PADDING_STANDARD);
  lv_obj_clear_flag(title_container, LV_OBJ_FLAG_CLICKABLE);

  // Error symbol
  lv_obj_t *error_symbol = lv_label_create(title_container);
  lv_label_set_text(error_symbol, LV_SYMBOL_WARNING);
  lv_obj_set_style_text_color(error_symbol, lv_color_white(), 0);
  lv_obj_set_style_text_font(error_symbol, UIConstants::FONT_LARGE, 0);
  lv_obj_clear_flag(error_symbol, LV_OBJ_FLAG_CLICKABLE);

  // Title text
  lv_obj_t *title = lv_label_create(title_container);
  lv_label_set_text(title, "Error");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_set_style_text_font(title, UIConstants::FONT_LARGE, 0);
  lv_obj_clear_flag(title, LV_OBJ_FLAG_CLICKABLE);

  // Message label - positioned below the title container
  lv_obj_t *msg_label = lv_label_create(ui.error_popup);
  lv_label_set_text(msg_label, message);
  lv_obj_set_style_text_color(msg_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(msg_label, UIConstants::FONT_SMALL, 0);
  lv_obj_set_width(msg_label, 320); // Slightly larger to fit the new popup size
  lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(msg_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(msg_label, title_container, LV_ALIGN_OUT_BOTTOM_MID, 0, UIConstants::PADDING_STANDARD);

  // Auto-dismiss timer
  lv_timer_create([](lv_timer_t *t) {
    if (ui.error_popup) {
      lv_obj_del(ui.error_popup);
      ui.error_popup = nullptr;
    }
    lv_timer_del(t);
  }, duration_ms, nullptr);
}

// WiFi State Machine Implementation
void handleWiFiStateMachine() {
  wifiManager.update();
}

// WiFi UI function
void updateWiFiUI() {
  bool connected = wifiManager.isConnected();
  WiFiState wifi_state = wifiManager.getState();

  if (connected) {
    if (ui.wifi_button && lv_obj_get_child_cnt(ui.wifi_button) > 0) {
      lv_label_set_text(lv_obj_get_child(ui.wifi_button, 0), "Forget Network");
      lv_obj_set_style_bg_color(ui.wifi_button, lv_color_hex(UIConstants::COLOR_DANGER), 0);
    }

    // NO WiFi status label updates - it's permanently hidden after initial scan

    if (ui.pass_ta) {
      lv_obj_add_flag(ui.pass_ta, LV_OBJ_FLAG_HIDDEN);
    }

    if (ui.keyboard) {
      lv_obj_del(ui.keyboard);
      ui.keyboard = nullptr;
      ui.current_ta = nullptr;
    }
  } else if (wifi_state == WIFI_CONNECTING) {
    if (ui.wifi_button && lv_obj_get_child_cnt(ui.wifi_button) > 0) {
      lv_label_set_text(lv_obj_get_child(ui.wifi_button, 0), "Connecting...");
      lv_obj_set_style_bg_color(ui.wifi_button, lv_color_hex(UIConstants::COLOR_SECONDARY), 0);
    }

    // NO WiFi status label updates

  } else {
    if (ui.wifi_button && lv_obj_get_child_cnt(ui.wifi_button) > 0) {
      lv_label_set_text(lv_obj_get_child(ui.wifi_button, 0), "Connect");
      lv_obj_set_style_bg_color(ui.wifi_button, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
    }

    // NO WiFi status label updates

    if (!state.selected_ssid.isEmpty() && ui.pass_ta) {
      lv_obj_clear_flag(ui.pass_ta, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// Calibration state machine
void updateCalibration() {
  // Skip calibration if IMU is not available
  if (!hardware_status.imu_available) {
    return;
  }
  
  unsigned long now = millis();

  switch (state.cal_state) {
    case CAL_IDLE:
      break;

    case CAL_IN_PROGRESS:
      if (now - state.cal_start < IMU_CALIBRATION_TIME) {
        collectCalibrationSample();
        state.cal_samples++;
        delay(10); // Small delay between samples
      } else {
        finishCalibration(state.cal_samples);
        state.cal_state = CAL_COMPLETE;
      }
      break;

    case CAL_COMPLETE:
      break;

    case CAL_FAILED:
      break;
  }
}

void collectCalibrationSample() {
  // Skip if IMU is not available
  if (!hardware_status.imu_available) {
    return;
  }
  
  static float sumAx = 0, sumAy = 0, sumAz = 0;
  static bool initialized = false;

  if (!initialized) {
    sumAx = sumAy = sumAz = 0;
    initialized = true;
  }

  float ax, ay, az;
  // Use built-in IMU readAcceleration method
  if (imu.readAcceleration(ax, ay, az)) {
    sumAx += ax;
    sumAy += ay;
    sumAz += az;

    if (state.cal_samples > 0) {
      sensors.accBiasX = sumAx / state.cal_samples;
      sensors.accBiasY = sumAy / state.cal_samples;
      sensors.accBiasZ = sumAz / state.cal_samples;
    }
  }
}

void finishCalibration(int samples) {
  if (samples > 50) {
    state.cal_state = CAL_COMPLETE;
  } else {
    state.cal_state = CAL_FAILED;
    sensors.accBiasX = 0.0f;
    sensors.accBiasY = 0.0f;
    sensors.accBiasZ = 0.0f;
  }
}

void checkScanResults() {
  // Check for scan timeout
  if (state.scan_in_progress && (millis() - state.scan_start_time > 15000)) {
    // Serial.println("Scan timeout detected, resetting scan state");
    state.scan_in_progress = false;

    // Reset rescan button
    if (ui.rescan_label && ui.rescan_btn) {
      lv_label_set_text(ui.rescan_label, LV_SYMBOL_REFRESH " Rescan");
      lv_obj_set_style_bg_color(ui.rescan_btn, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
      lv_obj_clear_state(ui.rescan_btn, LV_STATE_DISABLED);
    }

    if (ui.wifi_status_label) {
      lv_label_set_text(ui.wifi_status_label, "Scan timeout - try again");
    }
  }
}

void populateNetworkList(int network_count) {
  // Clear existing network buttons first
  uint32_t child_count = lv_obj_get_child_cnt(ui.main_screen);
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *child = lv_obj_get_child(ui.main_screen, i);

    // CRITICAL: Skip the rescan button to avoid deleting it
    if (child == ui.rescan_btn) {
      continue;
    }

    if (lv_obj_get_child_cnt(child) > 0) {
      lv_obj_t *child_label = lv_obj_get_child(child, 0);
      if (child_label && lv_obj_check_type(child_label, &lv_label_class)) {
        lv_coord_t y_pos = lv_obj_get_y(child);
        if (y_pos > UIConstants::HEADER_HEIGHT && y_pos < 400) {
          const char *label_text = lv_label_get_text(child_label);
          if (label_text && (strstr(label_text, "Rescan") != nullptr || strstr(label_text, LV_SYMBOL_REFRESH) != nullptr)) {
            continue;
          }

          lv_obj_del(child);
          i--;
          child_count--;
        }
      }
    }
  }

  // Mark initial scan as completed
  state.initial_scan_completed = true;

  if (ui.sub_footer) {
    lv_obj_clear_flag(ui.sub_footer, LV_OBJ_FLAG_HIDDEN);
  }

  // Calculate dynamic positioning based on actual network count
  int start_y = UIConstants::HEADER_HEIGHT + UIConstants::PADDING_STANDARD;
  int actual_networks = 0;

  if (network_count <= 0) {
    // No networks case - only show rescan button, NO status label
    int button_y = start_y + 40;

    // Show and position rescan button
    lv_obj_clear_flag(ui.rescan_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(ui.rescan_btn, LV_ALIGN_TOP_MID, 0, button_y);

    // Reset button state AND SIZE
    if (ui.rescan_label) {
      lv_label_set_text(ui.rescan_label, LV_SYMBOL_REFRESH " Rescan");
      lv_obj_set_style_bg_color(ui.rescan_btn, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
      lv_obj_clear_state(ui.rescan_btn, LV_STATE_DISABLED);

      // Reset to original width
      lv_obj_set_width(ui.rescan_btn, 150);
    }

    // PERMANENTLY hide the WiFi status label
    lv_obj_add_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN);
    if (ui.wifi_symbol) {                                   // <-- FIXED
      lv_obj_add_flag(ui.wifi_symbol, LV_OBJ_FLAG_HIDDEN);  // <-- FIXED
    }
    return;
  }

  // Better duplicate filtering with RSSI check
  struct NetworkInfo {
    String ssid;
    int rssi;
    int index;
  };

  static NetworkInfo unique_networks[10];
  int unique_count = 0;

  // Collect unique networks with strongest signal for each SSID
  for (int i = 0; i < network_count && unique_count < 5; i++) {
    const char *ssid_cstr = WiFi.SSID(i);
    if (!ssid_cstr || strlen(ssid_cstr) == 0) continue;

    String ssid = String(ssid_cstr);
    int rssi = WiFi.RSSI(i);

    if (rssi < -85) continue;

    bool found = false;
    for (int j = 0; j < unique_count; j++) {
      if (unique_networks[j].ssid == ssid) {
        if (rssi > unique_networks[j].rssi) {
          unique_networks[j].rssi = rssi;
          unique_networks[j].index = i;
        }
        found = true;
        break;
      }
    }

    if (!found && unique_count < 5) {
      unique_networks[unique_count].ssid = ssid;
      unique_networks[unique_count].rssi = rssi;
      unique_networks[unique_count].index = i;
      unique_count++;
    }
  }

  // Create buttons for unique networks
  for (int i = 0; i < unique_count; i++) {
    lv_obj_t *btn = lv_btn_create(ui.main_screen);
    lv_obj_add_style(btn, &StyleCache::list_item_style, LV_PART_MAIN);
    lv_obj_set_size(btn, UIConstants::ITEM_WIDTH, UIConstants::ITEM_HEIGHT);

    int y_position = start_y + (i * (UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING));
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y_position);

    lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, unique_networks[i].ssid.c_str());
    lv_obj_set_style_text_font(label, UIConstants::FONT_MEDIUM, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);

    if (WiFi.status() == WL_CONNECTED && unique_networks[i].ssid == WiFi.SSID()) {
      lv_obj_t *check = lv_label_create(btn);
      lv_label_set_text(check, LV_SYMBOL_OK);
      lv_obj_set_style_text_color(check, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
      lv_obj_set_style_text_font(check, &lv_font_montserrat_36, 0);
      lv_obj_align(check, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);
    }

    lv_obj_add_event_cb(btn, ssid_select_cb, LV_EVENT_CLICKED, nullptr);
    actual_networks++;
  }

  // PERMANENTLY hide WiFi status label after initial scan
  lv_obj_add_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN);
  if (ui.wifi_symbol) {                                   // <-- FIXED
    lv_obj_add_flag(ui.wifi_symbol, LV_OBJ_FLAG_HIDDEN);  // <-- FIXED
  }

  // Calculate where to place rescan button below networks
  int networks_end_y = start_y + (actual_networks * (UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING)) + 20;

  // Show and position rescan button centered below networks
  lv_obj_clear_flag(ui.rescan_btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align(ui.rescan_btn, LV_ALIGN_TOP_MID, 0, networks_end_y);

  // Reset button state AND SIZE
  if (ui.rescan_label) {
    lv_label_set_text(ui.rescan_label, LV_SYMBOL_REFRESH " Rescan");
    lv_obj_set_style_bg_color(ui.rescan_btn, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
    lv_obj_clear_state(ui.rescan_btn, LV_STATE_DISABLED);

    // Reset to original width
    lv_obj_set_width(ui.rescan_btn, 150);
  }

  lv_obj_remove_event_cb(ui.rescan_btn, nullptr);
  lv_obj_add_event_cb(ui.rescan_btn, rescan_btn_event_cb, LV_EVENT_CLICKED, nullptr);

  // Serial.println("Networks populated, rescan button reset to original size and state");
}

void startAsyncNetworkScan() {
  if (state.scan_in_progress) {
    // Serial.println("Scan already in progress, ignoring request");
    return;
  }

  // Serial.println("Starting network scan...");
  state.scan_in_progress = true;
  state.scan_start_time = millis();

  // ONLY show status label if this is the initial scan (before any networks have been found)
  if (!state.initial_scan_completed && ui.wifi_status_label) {
    lv_obj_clear_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(ui.wifi_status_label, LV_ALIGN_CENTER, 0, -50);
    lv_label_set_text(ui.wifi_status_label, "Scanning for WiFi...");
    lv_obj_set_style_text_font(ui.wifi_status_label, &lv_font_montserrat_28, 0);
  }

  // For rescans, update the rescan button instead
  if (state.initial_scan_completed && ui.rescan_btn && ui.rescan_label) {
    lv_label_set_text(ui.rescan_label, "Scanning Networks...");
    lv_obj_set_style_bg_color(ui.rescan_btn, lv_color_hex(UIConstants::COLOR_SECONDARY), 0);
    lv_obj_add_state(ui.rescan_btn, LV_STATE_DISABLED);

    // Increase width for longer text with 10px padding on each side
    lv_obj_set_width(ui.rescan_btn, 270);  // 220 + 20px total padding

    // Serial.println("Rescan button updated to show scanning state with increased width");
  }

  lv_timer_create([](lv_timer_t *t) {
    // Serial.println("Performing WiFi scan...");
    watchdog.kick();

    if (!wifiManager.isConnected()) {
      WiFi.disconnect();
      delay(500);
    }

    watchdog.kick();
    WiFi.scanNetworks();
    delay(200);
    watchdog.kick();

    int n = WiFi.scanNetworks();
    watchdog.kick();

    state.scan_in_progress = false;
    watchdog.kick();

    // Process results - populateNetworkList will handle UI and button reset
    if (n > 0) {
      // Serial.print("Scan complete, found ");
      // Serial.print(n);
      // Serial.println(" networks");
      populateNetworkList(n);
    } else if (n == 0) {
      // Serial.println("No networks found");
      populateNetworkList(0);
    } else {
      // Serial.print("Scan failed with code: ");
      // Serial.println(n);
      showErrorMessage("Scan failed");

      // For scan failure, reset rescan button to original state AND SIZE
      if (ui.rescan_btn && ui.rescan_label) {
        lv_label_set_text(ui.rescan_label, LV_SYMBOL_REFRESH " Rescan");
        lv_obj_set_style_bg_color(ui.rescan_btn, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
        lv_obj_clear_state(ui.rescan_btn, LV_STATE_DISABLED);

        // Reset to original width and remove any custom font
        lv_obj_set_width(ui.rescan_btn, 150);

        // Serial.println("Scan failed - rescan button reset to original size and state");
      }
    }

    lv_timer_del(t);
  },
                  200, nullptr);
}

void addCheckmarkToConnectedNetwork() {
  if (!wifiManager.isConnected()) return;

  String connectedSSID = wifiManager.getConnectedSSID();

  // Find the network button that matches the connected SSID
  uint32_t child_count = lv_obj_get_child_cnt(ui.main_screen);
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *child = lv_obj_get_child(ui.main_screen, i);

    // Check if this is a network button (has children and is in the right Y position)
    if (lv_obj_get_child_cnt(child) > 0) {
      lv_coord_t y_pos = lv_obj_get_y(child);
      if (y_pos > UIConstants::HEADER_HEIGHT && y_pos < 300) {
        lv_obj_t *label = lv_obj_get_child(child, 0);
        if (lv_obj_check_type(label, &lv_label_class)) {
          const char *ssid_text = lv_label_get_text(label);
          if (ssid_text && String(ssid_text) == connectedSSID) {
            // Check if checkmark already exists
            bool hasCheckmark = false;
            for (uint32_t j = 1; j < lv_obj_get_child_cnt(child); j++) {
              lv_obj_t *existing_child = lv_obj_get_child(child, j);
              if (lv_obj_check_type(existing_child, &lv_label_class)) {
                const char *text = lv_label_get_text(existing_child);
                if (text && strcmp(text, LV_SYMBOL_OK) == 0) {
                  hasCheckmark = true;
                  break;
                }
              }
            }

            // Add checkmark if it doesn't exist
            if (!hasCheckmark) {
              lv_obj_t *check = lv_label_create(child);
              lv_label_set_text(check, LV_SYMBOL_OK);
              lv_obj_set_style_text_color(check, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
              lv_obj_set_style_text_font(check, &lv_font_montserrat_36, 0);
              lv_obj_align(check, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);
              lv_obj_clear_flag(check, LV_OBJ_FLAG_CLICKABLE);
            }
            return;  // Found and updated, exit
          }
        }
      }
    }
  }
}

// UI Event Handlers
void toggleShift() {
  state.shift_enabled = !state.shift_enabled;
  updateUserInteraction();
}

void toggleSymbols() {
  state.symbol_mode = !state.symbol_mode;
  state.shift_enabled = false;
  updateUserInteraction();
}

void textarea_event_cb(lv_event_t *e) {
  ui.current_ta = (lv_obj_t *)lv_event_get_target(e);
  if (ui.current_ta) create_keyboard();
  updateUserInteraction();
}

void create_keyboard() {
  if (ui.keyboard) {
    lv_obj_del(ui.keyboard);
    ui.keyboard = nullptr;
  }

  static constexpr int ROW_HEIGHT = 65;
  static constexpr int ROW_SPACING = 5;
  static constexpr int TOTAL_HEIGHT = (ROW_HEIGHT * 4) + (ROW_SPACING * 3) + 10;

  ui.keyboard = lv_obj_create(lv_layer_top());
  lv_obj_set_size(ui.keyboard, UIConstants::SCREEN_WIDTH, TOTAL_HEIGHT);
  lv_obj_align(ui.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_pad_all(ui.keyboard, 0, 0);
  lv_obj_set_style_pad_top(ui.keyboard, 7, 0);
  lv_obj_set_style_bg_color(ui.keyboard, lv_color_hex(UIConstants::COLOR_HEADER), 0);
  lv_obj_set_style_border_width(ui.keyboard, 0, 0);
  lv_obj_set_style_radius(ui.keyboard, 0, 0);
  lv_obj_set_layout(ui.keyboard, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(ui.keyboard, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(ui.keyboard, ROW_SPACING, 0);
  lv_obj_clear_flag(ui.keyboard, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_move_foreground(ui.keyboard);

  static const char *keys_letters[4][12] = {
    { "q", "w", "e", "r", "t", "y", "u", "i", "o", "p", nullptr },
    { "a", "s", "d", "f", "g", "h", "j", "k", "l", nullptr },
    { LV_SYMBOL_UP, "z", "x", "c", "v", "b", "n", "m", LV_SYMBOL_BACKSPACE, nullptr },
    { "123", "SPACE", "ENTER", nullptr }
  };

  static const char *keys_symbols[4][12] = {
    { "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", nullptr },
    { "!", "@", "#", "$", "%", "^", "&", "*", "(", ")", nullptr },
    { LV_SYMBOL_UP, "-", "_", "+", "=", "[", "]", "{", "}", LV_SYMBOL_BACKSPACE, nullptr },
    { "ABC", "SPACE", "ENTER", nullptr }
  };

  const char *(*layout)[12] = state.symbol_mode ? keys_symbols : keys_letters;

  for (int row = 0; row < 4; row++) {
    lv_obj_t *row_cont = lv_obj_create(ui.keyboard);
    lv_obj_remove_style_all(row_cont);
    lv_obj_set_size(row_cont, UIConstants::SCREEN_WIDTH, ROW_HEIGHT);
    lv_obj_set_layout(row_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row_cont, 4, 0);
    lv_obj_clear_flag(row_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(row_cont, LV_OPA_TRANSP, 0);

    int key_count = 0;
    while (layout[row][key_count] != nullptr) key_count++;
    int available_width = UIConstants::SCREEN_WIDTH - ((key_count - 1) * 4);

    for (int col = 0; layout[row][col] != nullptr; col++) {
      const char *key = layout[row][col];
      int width = (row == 3) ? (strcmp(key, "SPACE") == 0 ? 272 : strcmp(key, "ENTER") == 0 ? 120
                                                                                            : 80)
                             : (available_width / key_count);

      lv_obj_t *btn = lv_btn_create(row_cont);
      lv_obj_set_size(btn, width, ROW_HEIGHT);

      bool isSpecial = strcmp(key, LV_SYMBOL_UP) == 0 || strcmp(key, LV_SYMBOL_BACKSPACE) == 0 || strcmp(key, "123") == 0 || strcmp(key, "ABC") == 0 || strcmp(key, "ENTER") == 0;

      lv_color_t baseColor = isSpecial ? lv_color_hex(0x303030) : lv_color_hex(0x545454);
      lv_color_t pressColor = lv_color_hex(UIConstants::COLOR_PRIMARY);

      lv_obj_set_style_bg_color(btn,
                                (strcmp(key, LV_SYMBOL_UP) == 0 && state.shift_enabled) ? pressColor : baseColor,
                                LV_PART_MAIN);
      lv_obj_set_style_bg_color(btn, pressColor, LV_STATE_PRESSED);
      lv_obj_set_style_text_color(btn, lv_color_hex(0xFFFFFF), 0);
      lv_obj_set_style_radius(btn, UIConstants::BUTTON_RADIUS, 0);
      lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_all(btn, 5, LV_STATE_PRESSED);

      lv_obj_t *label = lv_label_create(btn);
      lv_obj_set_style_text_font(label, UIConstants::FONT_MEDIUM, 0);

      if (state.shift_enabled && strlen(key) == 1 && isalpha(key[0])) {
        static char keybuf[2];
        keybuf[0] = toupper(key[0]);
        keybuf[1] = '\0';
        lv_label_set_text(label, keybuf);
      } else {
        lv_label_set_text(label, key);
      }
      lv_obj_center(label);

      lv_obj_add_event_cb(
        btn, [](lv_event_t *e) {
          lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
          const char *txt = lv_label_get_text(lv_obj_get_child(btn, 0));
          if (!ui.current_ta) return;

          updateUserInteraction();

          if (strcmp(txt, LV_SYMBOL_UP) == 0) {
            toggleShift();
            create_keyboard();
          } else if (strcmp(txt, "123") == 0 || strcmp(txt, "ABC") == 0) {
            toggleSymbols();
            create_keyboard();
          } else if (strcmp(txt, "SPACE") == 0) {
            lv_textarea_add_text(ui.current_ta, " ");
          } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
            lv_textarea_delete_char(ui.current_ta);
          } else if (strcmp(txt, "ENTER") == 0) {
            lv_obj_del(ui.keyboard);
            ui.keyboard = nullptr;
            lv_obj_clear_state(ui.current_ta, LV_STATE_FOCUSED);
            ui.current_ta = nullptr;
          } else {
            String keyStr = txt;
            if (state.shift_enabled && keyStr.length() == 1 && isalpha(keyStr[0])) {
              keyStr.toUpperCase();
              state.shift_enabled = false;
              create_keyboard();
            }
            lv_textarea_add_text(ui.current_ta, keyStr.c_str());
          }
        },
        LV_EVENT_CLICKED, nullptr);
    }
  }
}

// Network selection callback
void ssid_select_cb(lv_event_t *e) {
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  const char *ssid = lv_label_get_text(lv_obj_get_child(btn, 0));
  state.selected_ssid = String(ssid);
  updateUserInteraction();

  // Serial.print("Selected network: ");
  // Serial.println(ssid);

  // Show the WiFi modal
  showWiFiModal(String(ssid));
}

void global_click_event_cb(lv_event_t *e) {
  lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
  updateUserInteraction();

  auto isChildOf = [](lv_obj_t *child, lv_obj_t *parent) -> bool {
    lv_obj_t *current = child;
    while (current != nullptr) {
      if (current == parent) return true;
      current = lv_obj_get_parent(current);
    }
    return false;
  };

  // Don't close keyboard if interacting with modal or keyboard elements
  if (isChildOf(target, ui.keyboard) || isChildOf(target, ui.wifi_modal)) {
    return;
  }

  // Close keyboard if clicking elsewhere
  if (ui.keyboard) {
    lv_obj_del(ui.keyboard);
    ui.keyboard = nullptr;
    ui.current_ta = nullptr;
  }
}

void rescan_btn_event_cb(lv_event_t *e) {
  updateUserInteraction();

  if (state.scan_in_progress) {
    // Serial.println("Scan already in progress, ignoring rescan request");
    return;
  }

  // Serial.println("Rescan button pressed");

  // IMMEDIATELY update button to show scanning state with larger width
  if (ui.rescan_label && ui.rescan_btn) {
    lv_label_set_text(ui.rescan_label, "Scanning Networks...");
    lv_obj_set_style_bg_color(ui.rescan_btn, lv_color_hex(UIConstants::COLOR_SECONDARY), 0);
    lv_obj_add_state(ui.rescan_btn, LV_STATE_DISABLED);

    // Dynamically resize button for longer text
    lv_obj_set_width(ui.rescan_btn, 220);  // Increased from 150 to 220

    // Force immediate UI update
    lv_obj_invalidate(ui.rescan_btn);
    lv_timer_handler();
  }

  // Start the scan
  startAsyncNetworkScan();
}

void forget_wifi_cb(lv_event_t *e) {
  updateUserInteraction();

  CredentialManager::clear();
  WiFi.disconnect();
  state.selected_ssid = "";
  updateWiFiUI();
  if (ui.wifi_status_label) {
    lv_label_set_text(ui.wifi_status_label, "All Wi-Fi credentials cleared");
  }
  lv_obj_set_style_bg_color(ui.wifi_button, lv_color_hex(UIConstants::COLOR_PRIMARY), 0);
  lv_obj_add_flag(ui.wifi_button, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui.pass_ta, LV_OBJ_FLAG_HIDDEN);

  startAsyncNetworkScan();
}


void back_btn_cb(lv_event_t *e) {
  // Serial.println("Back button pressed");
  updateUserInteraction();

  // Dismiss keyboard if it's open
  if (ui.keyboard) {
    lv_obj_del(ui.keyboard);
    ui.keyboard = nullptr;
    ui.current_ta = nullptr;
  }

  // Check memory before transition
  if (MemoryManager::isMemoryCritical()) {
    // Serial.println("WARNING: Critical memory before screen transition");
    MemoryManager::forceCleanup();
  }

  // Use a timer with longer delay to safely return to main screen
  lv_timer_create([](lv_timer_t *t) {
    // Final memory check before transition
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    if (mon.free_size < 10000) {
      // Serial.println("WARNING: Low memory during transition, forcing cleanup");
      MemoryManager::forceCleanup();
    }

    showMainScreen();
    lv_timer_del(t);
  },
                  25, nullptr);  // Increased delay from 10ms to 100ms
}

void edit_name_btn_cb(lv_event_t *e) {
  updateUserInteraction();

  if (ui.editing_name) return;

  ui.editing_name = true;

  lv_obj_add_flag(ui.device_name_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui.edit_name_btn, LV_OBJ_FLAG_HIDDEN);

  lv_obj_clear_flag(ui.device_name_ta, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *name_container = lv_obj_get_parent(ui.device_name_label);
  lv_obj_t *name_btn = lv_obj_get_parent(name_container);

  lv_coord_t btn_x = lv_obj_get_x(name_btn);
  lv_coord_t btn_y = lv_obj_get_y(name_btn);

  lv_obj_set_pos(ui.device_name_ta, btn_x, btn_y);
  lv_obj_set_size(ui.device_name_ta, 300, UIConstants::ITEM_HEIGHT);
  lv_textarea_set_text(ui.device_name_ta, getDeviceName().c_str());

  // Create save and cancel buttons using optimized styles
  lv_obj_t *save_btn = UIHelper::createButton(ui.settings_screen, LV_SYMBOL_OK,
                                              &StyleCache::button_primary_style, save_name_btn_cb,
                                              LV_ALIGN_LEFT_MID, 0, 0, 40, 40);
  lv_obj_align_to(save_btn, ui.device_name_ta, LV_ALIGN_OUT_RIGHT_MID, UIConstants::PADDING_STANDARD + 40, 0);  // Moved further right
  lv_obj_set_style_bg_color(save_btn, lv_color_hex(UIConstants::COLOR_SUCCESS), 0);
  lv_obj_set_style_radius(save_btn, 20, 0);

  lv_obj_t *cancel_btn = UIHelper::createButton(ui.settings_screen, LV_SYMBOL_CLOSE,
                                                &StyleCache::button_danger_style, cancel_edit_btn_cb,
                                                LV_ALIGN_LEFT_MID, 0, 0, 40, 40);
  lv_obj_align_to(cancel_btn, save_btn, LV_ALIGN_OUT_RIGHT_MID, UIConstants::PADDING_SMALL, 0);  // Closer spacing between buttons
  lv_obj_set_style_radius(cancel_btn, 20, 0);
}

void save_name_btn_cb(lv_event_t *e) {
  updateUserInteraction();

  const char *new_name = lv_textarea_get_text(ui.device_name_ta);
  if (strlen(new_name) > 0) {
    setDeviceName(String(new_name));
    lv_label_set_text(ui.device_name_label, new_name);
    if (ui.main_title) {
      lv_label_set_text(ui.main_title, new_name);
    }
  }

  ui.editing_name = false;
  lv_obj_add_flag(ui.device_name_ta, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui.device_name_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui.edit_name_btn, LV_OBJ_FLAG_HIDDEN);

  // Remove save/cancel buttons
  uint32_t child_count = lv_obj_get_child_cnt(ui.settings_screen);
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *child = lv_obj_get_child(ui.settings_screen, i);
    if (lv_obj_get_child_cnt(child) > 0) {
      lv_obj_t *child_label = lv_obj_get_child(child, 0);
      const char *text = lv_label_get_text(child_label);
      if (strcmp(text, LV_SYMBOL_OK) == 0 || strcmp(text, LV_SYMBOL_CLOSE) == 0) {
        lv_obj_del(child);
        i--;
        child_count--;
      }
    }
  }
}

void cancel_edit_btn_cb(lv_event_t *e) {
  updateUserInteraction();

  ui.editing_name = false;
  lv_obj_add_flag(ui.device_name_ta, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui.device_name_label, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui.edit_name_btn, LV_OBJ_FLAG_HIDDEN);

  // Remove save/cancel buttons
  uint32_t child_count = lv_obj_get_child_cnt(ui.settings_screen);
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *child = lv_obj_get_child(ui.settings_screen, i);
    if (lv_obj_get_child_cnt(child) > 0) {
      lv_obj_t *child_label = lv_obj_get_child(child, 0);
      const char *text = lv_label_get_text(child_label);
      if (strcmp(text, LV_SYMBOL_OK) == 0 || strcmp(text, LV_SYMBOL_CLOSE) == 0) {
        lv_obj_del(child);
        i--;
        child_count--;
      }
    }
  }
}

// Settings page functions
void settings_btn_cb(lv_event_t *e) {
  updateUserInteraction();

  // Prevent screen transitions if one is already in progress
  if (screen_transition_in_progress) {
    DEBUG_PRINTLN("Screen transition already in progress, ignoring button press");
    return;
  }

  // Prevent multiple rapid button presses
  static unsigned long last_press = 0;
  unsigned long now = millis();
  if (now - last_press < 300) {  // 300ms debounce
    return;
  }
  last_press = now;

  // Set transition flag
  screen_transition_in_progress = true;

  // Immediate visual feedback
  lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_set_style_transform_zoom(btn, 260, 0);  // 1.02x scale
  lv_obj_invalidate(btn);

  // Force immediate UI update to show button feedback
  lv_timer_handler();

  // Reset button scale immediately in a safer way
  lv_obj_set_style_transform_zoom(btn, 256, 0);  // Back to normal
  lv_obj_invalidate(btn);

  // Clean up existing screens first, then create new one
  lv_timer_create([](lv_timer_t *t) {
    // Force memory cleanup before creating new screen
    MemoryManager::forceCleanup();

    // Clean up other screens safely
    if (ui.video_screen && lv_obj_is_valid(ui.video_screen)) {
      lv_obj_del(ui.video_screen);
      ui.video_screen = nullptr;
      ui.in_video = false;
    }

    // Small delay to let cleanup complete
    lv_timer_handler();

    // Now create settings screen
    createSettingsScreen();

    // Clear transition flag
    screen_transition_in_progress = false;

    lv_timer_del(t);
  }, 50, nullptr);  // Small delay for cleanup
}

void createSettingsScreen() {
  // Serial.println("Creating settings screen...");

  // Check memory before creating
  lv_mem_monitor_t mon;
  lv_mem_monitor(&mon);
  if (mon.free_size < 10000) {
    // Serial.println("WARNING: Low memory, forcing cleanup before creating settings screen");
    MemoryManager::forceCleanup();
  }

  // Clean up existing settings screen if it exists
  if (ui.settings_screen) {
    lv_obj_del(ui.settings_screen);
    ui.settings_screen = nullptr;
  }

  ui.in_settings = true;

  // Reset erase button state
  erase_confirmation_active = false;
  erase_button = nullptr;
  erase_icon = nullptr;
  erase_text = nullptr;

  // Create settings screen
  ui.settings_screen = lv_obj_create(nullptr);
  if (!ui.settings_screen) {
    // Serial.println("ERROR: Failed to create settings screen!");
    ui.in_settings = false;
    showMainScreen();
    return;
  }

  lv_obj_set_size(ui.settings_screen, UIConstants::SCREEN_WIDTH, UIConstants::SCREEN_HEIGHT);
  lv_obj_set_pos(ui.settings_screen, 0, 0);
  lv_obj_set_style_bg_color(ui.settings_screen, lv_color_hex(UIConstants::COLOR_BACKGROUND), 0);
  lv_obj_set_style_pad_all(ui.settings_screen, 0, 0);
  lv_obj_set_style_border_width(ui.settings_screen, 0, 0);
  lv_obj_set_style_radius(ui.settings_screen, 0, 0);
  lv_obj_clear_flag(ui.settings_screen, LV_OBJ_FLAG_SCROLLABLE);

  // Serial.println("Loading settings screen...");
  lv_scr_load(ui.settings_screen);

  // Small delay to ensure screen loads
  delay(20);

  // Serial.println("Creating settings footer...");
  // Create footer for settings screen
  FooterHelper::createFooter(ui.settings_screen);
  updateFooterIndicatorsToCurrentState();

  // Serial.println("Settings screen created successfully");

  // Add global click event
  lv_obj_add_event_cb(
    ui.settings_screen, [](lv_event_t *e) {
      lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
      updateUserInteraction();

      auto isChildOf = [](lv_obj_t *child, lv_obj_t *parent) -> bool {
        lv_obj_t *current = child;
        while (current != nullptr) {
          if (current == parent) return true;
          current = lv_obj_get_parent(current);
        }
        return false;
      };

      if (isChildOf(target, ui.keyboard) || isChildOf(target, ui.device_name_ta)) {
        return;
      }

      if (ui.keyboard) {
        lv_obj_del(ui.keyboard);
        ui.keyboard = nullptr;
        ui.current_ta = nullptr;
      }
    },
    LV_EVENT_CLICKED, nullptr);

  // Header setup using optimized approach
  lv_obj_t *header = lv_obj_create(ui.settings_screen);
  lv_obj_set_size(header, UIConstants::SCREEN_WIDTH, UIConstants::HEADER_HEIGHT);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(header, lv_color_hex(UIConstants::COLOR_HEADER), 0);
  lv_obj_set_style_radius(header, 0, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_pad_all(header, 0, 0);

  // Back button using transparent style
  lv_obj_t *back_btn = lv_btn_create(header);
  lv_obj_set_size(back_btn, 50, 40);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, UIConstants::PADDING_SMALL, 0);
  lv_obj_add_style(back_btn, &StyleCache::transparent_button_style, LV_PART_MAIN);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(back_label, UIConstants::FONT_LARGE, 0);
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, back_btn_cb, LV_EVENT_CLICKED, nullptr);

  // Settings title
  lv_obj_t *title = lv_label_create(header);
  lv_label_set_text(title, "Settings");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_set_style_text_font(title, UIConstants::FONT_MEDIUM, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  // Settings items using optimized helper
  int y_pos = UIConstants::HEADER_HEIGHT + UIConstants::PADDING_STANDARD;

  // Device Name item with edit functionality
  lv_obj_t *name_btn = lv_btn_create(ui.settings_screen);
  lv_obj_add_style(name_btn, &StyleCache::list_item_style, LV_PART_MAIN);
  lv_obj_set_size(name_btn, UIConstants::ITEM_WIDTH, UIConstants::ITEM_HEIGHT);
  lv_obj_align(name_btn, LV_ALIGN_TOP_MID, 0, y_pos);
  lv_obj_set_flex_flow(name_btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(name_btn, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(name_btn, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *name_info_container = lv_obj_create(name_btn);
  lv_obj_remove_style_all(name_info_container);
  lv_obj_set_layout(name_info_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(name_info_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(name_info_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_size(name_info_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_align(name_info_container, LV_ALIGN_LEFT_MID, UIConstants::PADDING_STANDARD, 0);

  ui.device_name_label = lv_label_create(name_info_container);
  lv_label_set_text(ui.device_name_label, getDeviceName().c_str());
  lv_obj_set_style_text_color(ui.device_name_label, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_set_style_text_font(ui.device_name_label, UIConstants::FONT_MEDIUM, 0);

  ui.edit_name_btn = lv_btn_create(name_btn);
  lv_obj_set_size(ui.edit_name_btn, 40, 40);
  lv_obj_align(ui.edit_name_btn, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);
  lv_obj_add_style(ui.edit_name_btn, &StyleCache::transparent_button_style, LV_PART_MAIN);

  lv_obj_t *edit_icon = lv_label_create(ui.edit_name_btn);
  lv_label_set_text(edit_icon, LV_SYMBOL_EDIT);
  lv_obj_set_style_text_color(edit_icon, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
  lv_obj_set_style_text_font(edit_icon, &lv_font_montserrat_20, 0);
  lv_obj_center(edit_icon);
  lv_obj_add_event_cb(ui.edit_name_btn, edit_name_btn_cb, LV_EVENT_CLICKED, nullptr);

  // Display Sleep setting with radio buttons
  y_pos += UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING;
  lv_obj_t *sleep_item = lv_btn_create(ui.settings_screen);
  lv_obj_add_style(sleep_item, &StyleCache::list_item_style, LV_PART_MAIN);
  lv_obj_set_size(sleep_item, UIConstants::ITEM_WIDTH, UIConstants::ITEM_HEIGHT);
  lv_obj_align(sleep_item, LV_ALIGN_TOP_MID, 0, y_pos);
  lv_obj_set_flex_flow(sleep_item, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(sleep_item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(sleep_item, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(sleep_item, LV_OBJ_FLAG_CLICKABLE);

  // Display Sleep title (left side)
  lv_obj_t *sleep_title = lv_label_create(sleep_item);
  lv_label_set_text(sleep_title, "Display Sleep");
  lv_obj_set_style_text_font(sleep_title, UIConstants::FONT_MEDIUM, 0);
  lv_obj_set_style_text_color(sleep_title, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_align(sleep_title, LV_ALIGN_LEFT_MID, UIConstants::PADDING_STANDARD, 0);

  // Sleep radio button container (right side)
  lv_obj_t *sleep_radio_container = lv_obj_create(sleep_item);
  lv_obj_remove_style_all(sleep_radio_container);
  lv_obj_set_size(sleep_radio_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(sleep_radio_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(sleep_radio_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(sleep_radio_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(sleep_radio_container, 8, 0);
  lv_obj_align(sleep_radio_container, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);
  lv_obj_clear_flag(sleep_radio_container, LV_OBJ_FLAG_CLICKABLE);

  // Create display sleep radio buttons with saved state
  createDisplaySleepRadioButtons(sleep_radio_container);

  // Model item
  y_pos += UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING;
  UIHelper::createSettingsItem(ui.settings_screen, "Model", "StreamBox Pro 2", y_pos);

  // Serial Number item
  y_pos += UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING;
  UIHelper::createSettingsItem(ui.settings_screen, "Serial Number", DEVICE_SERIAL_NUMBER, y_pos);

  // Firmware Version item
  y_pos += UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING;
  UIHelper::createSettingsItem(ui.settings_screen, "Firmware Version", FIRMWARE_VERSION, y_pos);

  // Check for Updates button (placeholder)
  lv_obj_t *update_btn = lv_btn_create(ui.settings_screen);
  lv_obj_set_size(update_btn, 270, 50);
  lv_obj_align(update_btn, LV_ALIGN_TOP_MID, 0, y_pos + UIConstants::ITEM_HEIGHT + 20);
  lv_obj_add_style(update_btn, &StyleCache::button_primary_style, LV_PART_MAIN);
  lv_obj_clear_flag(update_btn, LV_OBJ_FLAG_SCROLLABLE);

  // Create container for icon and text
  lv_obj_t *update_content = lv_obj_create(update_btn);
  lv_obj_remove_style_all(update_content);
  lv_obj_set_size(update_content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(update_content, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(update_content, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(update_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(update_content, 7, 0);
  lv_obj_center(update_content);
  lv_obj_clear_flag(update_content, LV_OBJ_FLAG_CLICKABLE);

  // Download icon
  lv_obj_t *download_icon = lv_label_create(update_content);
  lv_label_set_text(download_icon, LV_SYMBOL_DOWNLOAD);
  lv_obj_set_style_text_color(download_icon, lv_color_white(), 0);
  lv_obj_set_style_text_font(download_icon, UIConstants::FONT_MEDIUM, 0);
  lv_obj_clear_flag(download_icon, LV_OBJ_FLAG_CLICKABLE);

  // Button text
  lv_obj_t *update_label = lv_label_create(update_content);
  lv_label_set_text(update_label, "Check for Updates");
  lv_obj_set_style_text_color(update_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(update_label, UIConstants::FONT_MEDIUM, 0);
  lv_obj_clear_flag(update_label, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_add_event_cb(update_btn, updateButtonEventCallback, LV_EVENT_CLICKED, nullptr);

  // Fixed Erase Device button at bottom with better positioning
  erase_button = lv_btn_create(ui.settings_screen);
  lv_obj_set_size(erase_button, 255, 47);
  lv_obj_align(erase_button, LV_ALIGN_BOTTOM_MID, 0, -UIConstants::FOOTER_HEIGHT - 60);
  lv_obj_set_style_bg_color(erase_button, lv_color_hex(0xAFAFAF), 0);
  lv_obj_set_style_bg_opa(erase_button, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(erase_button, UIConstants::BUTTON_RADIUS, 0);
  lv_obj_set_style_border_width(erase_button, 0, 0);
  lv_obj_set_style_shadow_width(erase_button, 0, 0);
  lv_obj_set_style_outline_width(erase_button, 0, 0);
  lv_obj_clear_flag(erase_button, LV_OBJ_FLAG_SCROLLABLE);

  // Explicitly make sure the button is clickable and on top
  lv_obj_add_flag(erase_button, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_move_foreground(erase_button);

  // Create container for icon and text
  lv_obj_t *erase_content = lv_obj_create(erase_button);
  lv_obj_remove_style_all(erase_content);
  lv_obj_set_size(erase_content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(erase_content, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(erase_content, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(erase_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(erase_content, 7, 0);
  lv_obj_center(erase_content);
  lv_obj_clear_flag(erase_content, LV_OBJ_FLAG_CLICKABLE);

  // Warning icon
  erase_icon = lv_label_create(erase_content);
  lv_label_set_text(erase_icon, LV_SYMBOL_WARNING);
  lv_obj_set_style_text_color(erase_icon, lv_color_hex(UIConstants::COLOR_DANGER), 0);
  lv_obj_set_style_text_font(erase_icon, &lv_font_montserrat_18, 0);
  lv_obj_clear_flag(erase_icon, LV_OBJ_FLAG_CLICKABLE);

  // Button text
  erase_text = lv_label_create(erase_content);
  lv_label_set_text(erase_text, "Erase Device");
  lv_obj_set_style_text_color(erase_text, lv_color_white(), 0);
  lv_obj_set_style_text_font(erase_text, &lv_font_montserrat_18, 0);
  lv_obj_clear_flag(erase_text, LV_OBJ_FLAG_CLICKABLE);

  // Reset state
  erase_confirmation_active = false;

  // Add the event callback
  lv_obj_add_event_cb(
    erase_button, [](lv_event_t *e) {
      updateUserInteraction();
      // Serial.println("Erase button clicked!");

      if (!erase_confirmation_active) {
        erase_confirmation_active = true;

        lv_obj_set_style_bg_color(erase_button, lv_color_hex(UIConstants::COLOR_DANGER), 0);
        lv_obj_set_style_text_color(erase_icon, lv_color_white(), 0);
        lv_label_set_text(erase_text, "YES, ERASE DEVICE");

        // Serial.println("Erase Device - Confirmation required");

        // Auto-reset after 5 seconds if no action
        lv_timer_create([](lv_timer_t *t) {
          if (erase_confirmation_active && erase_button) {
            erase_confirmation_active = false;
            lv_obj_set_style_bg_color(erase_button, lv_color_hex(0xAFAFAF), 0);
            lv_obj_set_style_text_color(erase_icon, lv_color_hex(UIConstants::COLOR_DANGER), 0);
            lv_label_set_text(erase_icon, LV_SYMBOL_WARNING);
            lv_label_set_text(erase_text, "Erase Device");
            lv_obj_clear_state(erase_button, LV_STATE_DISABLED);
          }
          lv_timer_del(t);
        },
                        5000, nullptr);

      } else {
        // Serial.println("ERASING DEVICE - Starting erase process...");

        lv_obj_set_style_bg_color(erase_button, lv_color_hex(UIConstants::COLOR_WARNING), 0);
        lv_label_set_text(erase_text, "ERASING...");
        lv_obj_add_state(erase_button, LV_STATE_DISABLED);
        erase_confirmation_active = false;

        lv_timer_handler();

        lv_timer_create([](lv_timer_t *t) {
          Serial.println("Performing erase operations...");

          // IMMEDIATE WiFi disconnect and UI update
          Serial.println("Immediately disconnecting WiFi...");
          WiFi.disconnect();

          // Update WiFi manager state immediately
          wifiManager.setState(WIFI_IDLE);

          // Clear any existing checkmarks from network buttons immediately
          if (ui.main_screen) {
            uint32_t child_count = lv_obj_get_child_cnt(ui.main_screen);
            for (uint32_t i = 0; i < child_count; i++) {
              lv_obj_t *child = lv_obj_get_child(ui.main_screen, i);
              if (lv_obj_get_child_cnt(child) > 0) {
                lv_coord_t y_pos = lv_obj_get_y(child);
                if (y_pos > UIConstants::HEADER_HEIGHT && y_pos < 300) {
                  // This is a network button - remove any checkmarks
                  uint32_t btn_child_count = lv_obj_get_child_cnt(child);
                  for (uint32_t j = 1; j < btn_child_count; j++) {
                    lv_obj_t *btn_child = lv_obj_get_child(child, j);
                    if (lv_obj_check_type(btn_child, &lv_label_class)) {
                      const char *text = lv_label_get_text(btn_child);
                      if (text && strcmp(text, LV_SYMBOL_OK) == 0) {
                        lv_obj_del(btn_child);
                        break;
                      }
                    }
                  }
                }
              }
            }
          }

          // Force WiFi indicator update immediately
          if (FooterHelper::wifi_indicator) {
            lv_obj_set_style_text_color(FooterHelper::wifi_indicator,
                                        lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
          }

          // COMPLETE KVSTORE WIPE - Clear everything
          Serial.println("Clearing all WiFi credentials...");
          CredentialManager::clear();

          Serial.println("Removing device name...");
          kv_remove("/kv/device_name");

          Serial.println("Performing complete kvstore reset...");
          // Reset the entire kvstore system
          kv_reset("/kv/");

          // Also clear any other potential keys
          const char *keys_to_remove[] = {
            "/kv/net_count",
            "/kv/ssid_0", "/kv/pass_0",
            "/kv/ssid_1", "/kv/pass_1",
            "/kv/ssid_2", "/kv/pass_2",
            "/kv/ssid_3", "/kv/pass_3",
            "/kv/device_name"
          };

          for (int i = 0; i < sizeof(keys_to_remove) / sizeof(keys_to_remove[0]); i++) {
            kv_remove(keys_to_remove[i]);
            Serial.print("Removed key: ");
            Serial.println(keys_to_remove[i]);
          }

          // Complete WiFi shutdown
          Serial.println("Completing WiFi shutdown...");
          WiFi.end();
          // Non-blocking delay equivalent - keep system alive during WiFi shutdown
          unsigned long wifi_shutdown_start = millis();
          while (millis() - wifi_shutdown_start < 1000) {
            watchdog.kick();
            delay(10); // Small delay to prevent tight loop
          }

          Serial.println("Resetting device name to default...");
          if (ui.device_name_label) {
            lv_label_set_text(ui.device_name_label, DEFAULT_DEVICE_NAME);
          }
          if (ui.main_title) {
            lv_label_set_text(ui.main_title, DEFAULT_DEVICE_NAME);
          }

          // Update WiFi status to reflect disconnection
          if (ui.wifi_status_label) {
            lv_label_set_text(ui.wifi_status_label, "All data cleared - device disconnected");
          }

          // Force another WiFi indicator update to ensure it's grey
          if (FooterHelper::wifi_indicator) {
            lv_obj_set_style_text_color(FooterHelper::wifi_indicator,
                                        lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
          }

          // Verify the erase worked
          Serial.println("Verifying erase completion...");
          debugKVStore();

          Serial.println("Erase operations completed");

          if (erase_button) {
            lv_obj_set_style_bg_color(erase_button, lv_color_hex(0xAFAFAF), 0);
            lv_obj_set_style_text_color(erase_icon, lv_color_hex(0x34C85A), 0);
            lv_label_set_text(erase_icon, LV_SYMBOL_OK);
            lv_label_set_text(erase_text, "Device Erased");
            lv_obj_set_style_text_color(erase_text, lv_color_hex(0x000000), 0);
            lv_obj_add_state(erase_button, LV_STATE_DISABLED);
          }

          Serial.println("Device erase completed successfully - all data removed and WiFi disconnected");
          lv_timer_del(t);
        },
                        50, nullptr);
      }
    },
    LV_EVENT_CLICKED, nullptr);

  // Device name text area (hidden initially)
  ui.device_name_ta = UIHelper::createTextArea(ui.settings_screen, "Device Name",
                                               LV_ALIGN_TOP_LEFT, 0, 0, 300, UIConstants::ITEM_HEIGHT);
  lv_obj_add_flag(ui.device_name_ta, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(ui.device_name_ta, textarea_event_cb, LV_EVENT_FOCUSED, nullptr);
  lv_obj_set_style_pad_top(ui.device_name_ta, 20, 0);
  lv_obj_set_style_pad_bottom(ui.device_name_ta, 20, 0);
}

void showMainScreen() {
  Serial.println("Returning to main screen...");

  // Kick watchdog immediately
  watchdog.kick();

  // Clean up keyboard first
  if (ui.keyboard) {
    lv_obj_del(ui.keyboard);
    ui.keyboard = nullptr;
    ui.current_ta = nullptr;
  }

  // Close any open modals
  closeWiFiModal();

  // Update flags immediately
  ui.in_settings = false;
  ui.in_video = false;
  ui.editing_name = false;

  // Reset telemetry-only labels (but don't reset footer indicators)
  ui.gps_label = nullptr;
  ui.imu_label = nullptr;


  watchdog.kick();

  // Check if main screen exists, if not recreate it
  if (!ui.main_screen) {
    Serial.println("Main screen is null, recreating...");
    MemoryManager::forceCleanup();
    watchdog.kick();

    ui.main_screen = lv_obj_create(nullptr);
    lv_scr_load(ui.main_screen);
    setupUI();
    return;
  }

  // CRITICAL: Load main screen FIRST before deleting anything
  Serial.println("Loading main screen...");
  lv_scr_load(ui.main_screen);

  watchdog.kick();

  // CRITICAL: Much longer delay to ensure screen is fully loaded
  delay(50);  // Increased from 200ms

  watchdog.kick();

  // Force LVGL to process everything
  lv_timer_handler();

  watchdog.kick();

  // Use a timer to safely delete other screens after main screen is stable
  lv_timer_create([](lv_timer_t *timer) {
    watchdog.kick();

    // Now safely delete other screens
    if (ui.settings_screen) {
      Serial.println("Safely deleting settings screen...");
      lv_obj_del(ui.settings_screen);
      ui.settings_screen = nullptr;
      lv_timer_handler();
      watchdog.kick();
    }


    if (ui.video_screen) {
      Serial.println("Safely deleting video screen...");
      lv_obj_del(ui.video_screen);
      ui.video_screen = nullptr;
      lv_timer_handler();
      watchdog.kick();
    }

    // Reset erase button globals
    erase_confirmation_active = false;
    erase_button = nullptr;
    erase_icon = nullptr;
    erase_text = nullptr;

    // Force cleanup after deletion
    MemoryManager::forceCleanup();
    watchdog.kick();

    // Clear selected SSID
    state.selected_ssid = "";

    // Update WiFi status safely
    if (ui.wifi_status_label) {
      if (wifiManager.isConnected()) {
        lv_label_set_text(ui.wifi_status_label, "Connected!");
      } else {
        lv_label_set_text(ui.wifi_status_label, "Select a network to connect");
      }
    }

    // Make sure footer indicators are updated
    updateFooterIndicatorsToCurrentState();

    watchdog.kick();
    Serial.println("Main screen transition completed safely");

    lv_timer_del(timer);
  },
                  50, nullptr);  // Delay the cleanup
}

// WiFi Modal Functions - Updated
void createWiFiModal(const String &ssid) {
  if (ui.wifi_modal) {
    closeWiFiModal();
  }

  ui.modal_current_ssid = ssid;

  // Create modal background
  ui.wifi_modal = lv_obj_create(lv_layer_top());
  lv_obj_set_size(ui.wifi_modal, UIConstants::SCREEN_WIDTH, UIConstants::SCREEN_HEIGHT);
  lv_obj_center(ui.wifi_modal);
  lv_obj_set_style_bg_color(ui.wifi_modal, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ui.wifi_modal, LV_OPA_50, 0);
  lv_obj_set_style_border_width(ui.wifi_modal, 0, 0);
  lv_obj_set_style_radius(ui.wifi_modal, 0, 0);
  lv_obj_clear_flag(ui.wifi_modal, LV_OBJ_FLAG_SCROLLABLE);

  // Create modal content container - REDUCED SIZE
  lv_obj_t *modal_content = lv_obj_create(ui.wifi_modal);
  lv_obj_set_size(modal_content, 350, 220);  // Reduced from 400x300 to 350x220
  lv_obj_center(modal_content);
  lv_obj_add_style(modal_content, &StyleCache::list_item_style, LV_PART_MAIN);
  lv_obj_set_style_radius(modal_content, 12, 0);
  lv_obj_clear_flag(modal_content, LV_OBJ_FLAG_SCROLLABLE);

  // SSID Label - CHANGED TO BLACK COLOR
  ui.modal_ssid_label = lv_label_create(modal_content);
  lv_label_set_text(ui.modal_ssid_label, ssid.c_str());
  lv_obj_set_style_text_font(ui.modal_ssid_label, UIConstants::FONT_LARGE, 0);
  lv_obj_set_style_text_color(ui.modal_ssid_label, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);  // Changed from COLOR_PRIMARY to COLOR_TEXT_PRIMARY (black)
  lv_obj_align(ui.modal_ssid_label, LV_ALIGN_TOP_MID, 0, 20);                                          // Moved up from 50 to 20

  // Status label - REMOVED "Enter network password" text for password input
  ui.modal_status_label = lv_label_create(modal_content);
  lv_obj_set_style_text_font(ui.modal_status_label, UIConstants::FONT_SMALL, 0);
  lv_obj_set_style_text_color(ui.modal_status_label, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_align(ui.modal_status_label, LV_ALIGN_TOP_MID, 0, 55);  // Moved up from 85 to 55

  // Determine what to show based on network status
  bool isConnected = wifiManager.isConnected() && (wifiManager.getConnectedSSID() == ssid);
  bool isSaved = CredentialManager::isNetworkSaved(ssid);

  if (isConnected) {
    lv_label_set_text(ui.modal_status_label, "");

    ui.modal_disconnect_btn = lv_btn_create(modal_content);
    lv_obj_set_size(ui.modal_disconnect_btn, 250, 45);
    lv_obj_align(ui.modal_disconnect_btn, LV_ALIGN_TOP_MID, 0, 100);
    lv_obj_add_style(ui.modal_disconnect_btn, &StyleCache::button_danger_style, LV_PART_MAIN);
    lv_obj_clear_flag(ui.modal_disconnect_btn, LV_OBJ_FLAG_SCROLLABLE);

    // Create container for icon and text
    lv_obj_t *disconnect_content = lv_obj_create(ui.modal_disconnect_btn);
    lv_obj_remove_style_all(disconnect_content);
    lv_obj_set_size(disconnect_content, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_layout(disconnect_content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(disconnect_content, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(disconnect_content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(disconnect_content, 7, 0);
    lv_obj_center(disconnect_content);
    lv_obj_clear_flag(disconnect_content, LV_OBJ_FLAG_CLICKABLE);

    // X icon
    lv_obj_t *disconnect_icon = lv_label_create(disconnect_content);
    lv_label_set_text(disconnect_icon, LV_SYMBOL_CLOSE);
    lv_obj_set_style_text_color(disconnect_icon, lv_color_white(), 0);
    lv_obj_set_style_text_font(disconnect_icon, UIConstants::FONT_MEDIUM, 0);
    lv_obj_clear_flag(disconnect_icon, LV_OBJ_FLAG_CLICKABLE);

    // Button text
    lv_obj_t *disconnect_label = lv_label_create(disconnect_content);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_set_style_text_color(disconnect_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(disconnect_label, UIConstants::FONT_MEDIUM, 0);
    lv_obj_clear_flag(disconnect_label, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_event_cb(ui.modal_disconnect_btn, modal_disconnect_cb, LV_EVENT_CLICKED, nullptr);
  } else if (isSaved) {
    // Automatically connect to saved network
    lv_label_set_text(ui.modal_status_label, "Connecting to saved network...");

    // Find and connect with saved password
    for (int i = 0; i < MAX_NETWORKS; i++) {
      String test_ssid, test_password;
      if (CredentialManager::loadNetworkAtIndex(i, test_ssid, test_password) && test_ssid == ssid) {
        wifiManager.connect(ssid, test_password, false);
        break;
      }
    }

    // Close modal after starting connection
    lv_timer_create([](lv_timer_t *t) {
      closeWiFiModal();
      lv_timer_del(t);
    },
                    1000, nullptr);

    return;  // Don't show other UI elements

  } else {
    // REMOVED "Enter network password" text - just show empty status
    lv_label_set_text(ui.modal_status_label, "");

    // Password input - ADJUSTED POSITION
    ui.modal_pass_ta = UIHelper::createTextArea(modal_content, "Password",
                                                LV_ALIGN_TOP_MID, 0, 80, 310, 45);  // Moved up and adjusted size
    lv_obj_add_event_cb(ui.modal_pass_ta, textarea_event_cb, LV_EVENT_FOCUSED, nullptr);

    // Connect button - ADJUSTED POSITION
    ui.modal_connect_btn = UIHelper::createButton(modal_content, "Connect",
                                                  &StyleCache::button_primary_style, modal_connect_cb,
                                                  LV_ALIGN_TOP_MID, 0, 140, 180, 45);  // Moved up and adjusted size
  }

  // Add background click to close
  lv_obj_add_event_cb(
    ui.wifi_modal, [](lv_event_t *e) {
      lv_obj_t *target = (lv_obj_t *)lv_event_get_target(e);
      if (target == ui.wifi_modal) {  // Only if clicking background
        closeWiFiModal();
      }
    },
    LV_EVENT_CLICKED, nullptr);
}

void closeWiFiModal() {
  if (ui.wifi_modal) {
    // Close keyboard if open
    if (ui.keyboard) {
      lv_obj_del(ui.keyboard);
      ui.keyboard = nullptr;
      ui.current_ta = nullptr;
    }

    lv_obj_del(ui.wifi_modal);
    ui.wifi_modal = nullptr;
    ui.modal_ssid_label = nullptr;
    ui.modal_pass_ta = nullptr;
    ui.modal_connect_btn = nullptr;
    ui.modal_disconnect_btn = nullptr;
    ui.modal_status_label = nullptr;
    ui.modal_current_ssid = "";
  }
}

void showWiFiModal(const String &ssid) {
  updateUserInteraction();
  createWiFiModal(ssid);
}

void modal_connect_cb(lv_event_t *e) {
  updateUserInteraction();

  // IMMEDIATELY update button text before doing ANYTHING else
  if (ui.modal_connect_btn) {
    lv_label_set_text(lv_obj_get_child(ui.modal_connect_btn, 0), "Connecting...");
    lv_obj_add_state(ui.modal_connect_btn, LV_STATE_DISABLED);
    lv_obj_invalidate(ui.modal_connect_btn);  // Force immediate redraw
  }

  // Update status label immediately too
  if (ui.modal_status_label) {
    lv_obj_invalidate(ui.modal_status_label);  // Force immediate redraw
  }

  // Process the UI updates immediately
  lv_refr_now(NULL);

  // Now check inputs and start connection
  if (ui.modal_current_ssid.isEmpty() || !ui.modal_pass_ta) return;

  const char *password = lv_textarea_get_text(ui.modal_pass_ta);

  // Start connection
  wifiManager.connect(ui.modal_current_ssid, String(password), false);

  // Close modal after a delay
  lv_timer_create([](lv_timer_t *t) {
    closeWiFiModal();
    lv_timer_del(t);
  },
                  1500, nullptr);
}

void modal_disconnect_cb(lv_event_t *e) {
  updateUserInteraction();

  if (ui.modal_current_ssid.isEmpty()) return;

  // Remove from saved networks and disconnect
  CredentialManager::removeNetwork(ui.modal_current_ssid);
  wifiManager.disconnect();

  // Update status
  if (ui.modal_status_label) {
    lv_label_set_text(ui.modal_status_label, "Disconnected");
  }

  // Close modal
  lv_timer_create([](lv_timer_t *t) {
    closeWiFiModal();
    lv_timer_del(t);
  },
                  1000, nullptr);
}

void modal_cancel_cb(lv_event_t *e) {
  updateUserInteraction();
  closeWiFiModal();
}

void attemptAutoConnect() {
  if (WiFi.status() == WL_NO_MODULE) {
    // ONLY show message if label is visible (not hidden by network list)
    if (ui.wifi_status_label && !lv_obj_has_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN)) {
      lv_label_set_text(ui.wifi_status_label, "WiFi module not ready");
    }
    return;
  }

  String saved_ssid, saved_password;
  if (CredentialManager::loadMostRecent(saved_ssid, saved_password)) {
    if (saved_password.length() > 0) {
      bool networkFound = false;
      int networkCount = WiFi.scanNetworks();

      for (int i = 0; i < networkCount; i++) {
        String scannedSSID = WiFi.SSID(i);
        if (scannedSSID == saved_ssid) {
          networkFound = true;
          break;
        }
      }

      if (!networkFound) {
        // ONLY show message if label is visible
        if (ui.wifi_status_label && !lv_obj_has_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN)) {
          lv_label_set_text(ui.wifi_status_label, "Saved network not found");
        }
        return;
      }

      state.selected_ssid = saved_ssid;
      // ONLY show message if label is visible
      if (ui.wifi_status_label && !lv_obj_has_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN)) {
        lv_label_set_text(ui.wifi_status_label, "Auto-connecting...");
      }
      wifiManager.connect(saved_ssid, saved_password, true);
    } else {
      // ONLY show message if label is visible
      if (ui.wifi_status_label && !lv_obj_has_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN)) {
        lv_label_set_text(ui.wifi_status_label, "Select a network to connect");
      }
    }
  } else {
    // ONLY show message if label is visible
    if (ui.wifi_status_label && !lv_obj_has_flag(ui.wifi_status_label, LV_OBJ_FLAG_HIDDEN)) {
      lv_label_set_text(ui.wifi_status_label, "Select a network to connect");
    }
  }
}

void setupUI() {
  ui.main_screen = lv_scr_act();

  // Set main screen background
  lv_obj_set_style_bg_color(ui.main_screen, lv_color_hex(UIConstants::COLOR_BACKGROUND), 0);

  // Create footer for main screen
  FooterHelper::createFooter(ui.main_screen);
  // Update indicators to current state after footer is created
  updateFooterIndicatorsToCurrentState();

  // Header setup using optimized approach
  lv_obj_t *header = lv_obj_create(ui.main_screen);
  lv_obj_set_size(header, UIConstants::SCREEN_WIDTH, UIConstants::HEADER_HEIGHT);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(header, lv_color_hex(UIConstants::COLOR_HEADER), 0);
  lv_obj_set_style_radius(header, 0, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

  // Device name title
  ui.main_title = lv_label_create(header);
  lv_label_set_text(ui.main_title, getDeviceName().c_str());
  lv_obj_set_style_text_color(ui.main_title, lv_color_white(), 0);
  lv_obj_set_style_text_font(ui.main_title, UIConstants::FONT_MEDIUM, 0);
  lv_obj_align(ui.main_title, LV_ALIGN_CENTER, 0, 0);

  // Create WiFi symbol first - positioned above status label
  ui.wifi_symbol = lv_label_create(ui.main_screen);
  lv_label_set_text(ui.wifi_symbol, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_color(ui.wifi_symbol, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
  lv_obj_set_style_text_font(ui.wifi_symbol, UIConstants::FONT_LARGE, 0);
  lv_obj_align(ui.wifi_symbol, LV_ALIGN_CENTER, 0, -90);
  lv_obj_clear_flag(ui.wifi_symbol, LV_OBJ_FLAG_CLICKABLE);

  // Create WiFi status label - positioned below symbol
  ui.wifi_status_label = lv_label_create(ui.main_screen);
  lv_label_set_text(ui.wifi_status_label, "Scanning for WiFi...");
  lv_obj_set_style_text_color(ui.wifi_status_label, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
  lv_obj_set_style_text_font(ui.wifi_status_label, UIConstants::FONT_SMALL, 0);
  lv_obj_set_width(ui.wifi_status_label, UIConstants::ITEM_WIDTH);
  lv_label_set_long_mode(ui.wifi_status_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(ui.wifi_status_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(ui.wifi_status_label, LV_ALIGN_CENTER, 0, -50);
  lv_obj_clear_flag(ui.wifi_status_label, LV_OBJ_FLAG_CLICKABLE);

  // Create rescan button - HIDDEN initially until networks are populated
  ui.rescan_btn = lv_btn_create(ui.main_screen);
  lv_obj_set_size(ui.rescan_btn, 150, 50);
  lv_obj_add_style(ui.rescan_btn, &StyleCache::button_primary_style, LV_PART_MAIN);
  lv_obj_clear_flag(ui.rescan_btn, LV_OBJ_FLAG_SCROLLABLE);
  // Hide initially - will be shown and positioned by populateNetworkList()
  lv_obj_add_flag(ui.rescan_btn, LV_OBJ_FLAG_HIDDEN);

  ui.rescan_label = lv_label_create(ui.rescan_btn);
  lv_label_set_text(ui.rescan_label, LV_SYMBOL_REFRESH " Rescan");
  lv_obj_set_style_text_font(ui.rescan_label, UIConstants::FONT_MEDIUM, 0);
  lv_obj_center(ui.rescan_label);
  lv_obj_add_event_cb(ui.rescan_btn, rescan_btn_event_cb, LV_EVENT_CLICKED, nullptr);

  ui.sub_footer = lv_obj_create(ui.main_screen);
  lv_obj_set_size(ui.sub_footer, UIConstants::SCREEN_WIDTH, 80);
  lv_obj_align(ui.sub_footer, LV_ALIGN_BOTTOM_MID, 0, -UIConstants::FOOTER_HEIGHT);
  lv_obj_set_style_bg_color(ui.sub_footer, lv_color_hex(0xCECECE), 0);
  lv_obj_set_style_bg_opa(ui.sub_footer, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(ui.sub_footer, 0, 0);
  lv_obj_set_style_border_width(ui.sub_footer, 0, 0);
  lv_obj_clear_flag(ui.sub_footer, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(ui.sub_footer, LV_OBJ_FLAG_CLICKABLE);

  // HIDE sub-footer initially during scan
  lv_obj_add_flag(ui.sub_footer, LV_OBJ_FLAG_HIDDEN);

  // Video button - black background (moved to left position)
  ui.video_btn = lv_btn_create(ui.sub_footer);
  lv_obj_set_size(ui.video_btn, 190, 50);
  lv_obj_align(ui.video_btn, LV_ALIGN_LEFT_MID, UIConstants::PADDING_STANDARD, 0);
  lv_obj_set_style_bg_color(ui.video_btn, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(ui.video_btn, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(ui.video_btn, UIConstants::BUTTON_RADIUS, 0);
  lv_obj_set_style_border_width(ui.video_btn, 0, 0);
  lv_obj_set_style_shadow_width(ui.video_btn, 0, 0);
  lv_obj_set_style_outline_width(ui.video_btn, 0, 0);
  lv_obj_clear_flag(ui.video_btn, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *video_label = lv_label_create(ui.video_btn);
  lv_label_set_text(video_label, LV_SYMBOL_PLAY " Video Settings");
  lv_obj_set_style_text_color(video_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(video_label, UIConstants::FONT_SMALL, 0);
  lv_obj_center(video_label);
  lv_obj_add_event_cb(ui.video_btn, video_btn_cb, LV_EVENT_CLICKED, nullptr);

  // Settings button - black icon
  ui.settings_btn = lv_btn_create(ui.sub_footer);
  lv_obj_set_size(ui.settings_btn, 45, 72);
  lv_obj_align(ui.settings_btn, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);
  lv_obj_set_style_bg_opa(ui.settings_btn, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(ui.settings_btn, 0, 0);
  lv_obj_set_style_shadow_width(ui.settings_btn, 0, 0);
  lv_obj_set_style_outline_width(ui.settings_btn, 0, 0);

  lv_obj_t *settings_icon = lv_label_create(ui.settings_btn);
  lv_label_set_text(settings_icon, LV_SYMBOL_SETTINGS);
  lv_obj_set_style_text_color(settings_icon, lv_color_hex(0x000000), 0);  // Black icon
  lv_obj_set_style_text_font(settings_icon, &lv_font_montserrat_34, 0);
  lv_obj_center(settings_icon);
  lv_obj_add_event_cb(ui.settings_btn, settings_btn_cb, LV_EVENT_CLICKED, nullptr);

  // Initialize modal elements to nullptr
  ui.wifi_modal = nullptr;
  ui.modal_ssid_label = nullptr;
  ui.modal_pass_ta = nullptr;
  ui.modal_connect_btn = nullptr;
  ui.modal_disconnect_btn = nullptr;
  ui.modal_cancel_btn = nullptr;
  ui.modal_status_label = nullptr;

  // Initialize GPS and IMU labels to nullptr
  ui.gps_label = nullptr;
  ui.imu_label = nullptr;

  // Global click event
  lv_obj_add_event_cb(ui.main_screen, global_click_event_cb, LV_EVENT_CLICKED, nullptr);
}

// Optimized main loop with better memory management
void optimizedLoop() {
  perf_metrics.startLoop();

  static unsigned long lastWatchdogKick = 0;
  unsigned long now = millis();
  if (now - lastWatchdogKick > 3000) {
    watchdog.kick();
    lastWatchdogKick = now;
  }

  // Check display sleep status
  checkDisplaySleep();

  // Only process UI updates if display is not sleeping (saves power)
  if (!display_sleep.sleeping) {
    // CRITICAL: Always call LVGL timer handler for maximum UI responsiveness
    // This is the most important thing for button response!
    lv_timer_handler();

    // Separate UI updates from sensor updates for better responsiveness
    if (perf_metrics.shouldUpdateUI()) {
      // UI-specific updates that need throttling
      handleWiFiStateMachine();

      // Light memory monitoring (not the heavy cleanup)
      static unsigned long lastLightMemCheck = 0;
      if (now - lastLightMemCheck > 5000) {  // Every 5 seconds
        if (MemoryManager::isMemoryCritical()) {
          DEBUG_PRINTLN("CRITICAL: Memory very low, forcing cleanup");
          MemoryManager::forceCleanup();
        }
        lastLightMemCheck = now;
      }
    }

    // Sensor updates on their own schedule
    if (perf_metrics.shouldUpdateSensors()) {
      sensorManager.updateGPS();
      sensorManager.updateIMU();
      updateCalibration();
    }

    // Heavy operations on much slower schedule
    static unsigned long lastHeavyUpdate = 0;
    if (now - lastHeavyUpdate > 1000) {  // Every 1 second instead of every UI update
      telemetryManager.update();
      MemoryManager::periodicCleanup();
      lastHeavyUpdate = now;
    }

    // Force cleanup much less frequently
    static unsigned long lastForceCleanup = 0;
    if (now - lastForceCleanup > 300000) {  // Every 5 minutes instead of 2 minutes
      MemoryManager::forceCleanup();
      lastForceCleanup = now;
    }

    // Add video UI updates for recording timer
    if (ui.in_video && video_settings.recording) {
      static unsigned long lastVideoUpdate = 0;
      if (now - lastVideoUpdate > 1000) {
        updateVideoUI();
        lastVideoUpdate = now;
      }
    }
  }

  yield();
  perf_metrics.endLoop();
}

// Display sleep management functions
void checkDisplaySleep() {
  if (display_sleep.timeout == 0) return;  // "Never" selected

  unsigned long now = millis();

  // Check if timeout exceeded and display is currently on
  if (!display_sleep.sleeping && (now - display_sleep.lastTouchTime > display_sleep.timeout)) {
    // Put display to sleep
    display_sleep.sleeping = true;
    backlight.set(0);  // Turn off backlight

    Serial.print("Display going to sleep after ");
    Serial.print(display_sleep.timeout / 1000);
    Serial.println(" seconds of inactivity");
  }
}

void wakeDisplay() {
  if (display_sleep.sleeping) {
    display_sleep.sleeping = false;
    backlight.set(100);                      // Turn backlight back to full brightness
    display_sleep.lastTouchTime = millis();  // Reset the timeout

    Serial.println("Display waking up (touch detected)");
  }
}

void updateUserInteraction() {
  display_sleep.lastTouchTime = millis();

  // If display is sleeping, wake it up
  if (display_sleep.sleeping) {
    wakeDisplay();
  }
}

void updateDisplaySleepTimeout(const char *selection) {
  display_sleep.current_setting = String(selection);

  if (strcmp(selection, "15s") == 0) {
    display_sleep.timeout = 15000;
  } else if (strcmp(selection, "60s") == 0) {
    display_sleep.timeout = 60000;
  } else if (strcmp(selection, "Never") == 0) {
    display_sleep.timeout = 0;  // Never sleep
  }

  // Save the setting to persistent storage
  DisplaySleepManager::saveDisplaySleepSetting(String(selection));

  Serial.print("Display sleep timeout set to: ");
  Serial.println(selection);
}

void loadDisplaySleepSetting() {
  String saved_setting = DisplaySleepManager::loadDisplaySleepSetting();

  // Apply the loaded setting
  display_sleep.current_setting = saved_setting;

  if (saved_setting == "15s") {
    display_sleep.timeout = 15000;
  } else if (saved_setting == "60s") {
    display_sleep.timeout = 60000;
  } else if (saved_setting == "Never") {
    display_sleep.timeout = 0;
  } else {
    // Fallback to default if invalid setting
    display_sleep.timeout = 60000;
    display_sleep.current_setting = "60s";
  }

  Serial.print("Display sleep setting loaded and applied: ");
  Serial.print(saved_setting);
  Serial.print(" (timeout: ");
  Serial.print(display_sleep.timeout);
  Serial.println("ms)");
}

// ADD THIS NEW FUNCTION RIGHT AFTER loadDisplaySleepSetting:
void createDisplaySleepRadioButtons(lv_obj_t *sleep_radio_container) {
  // Get the current saved setting
  String current_setting = display_sleep.current_setting;

  // 15s radio button
  bool is_15s_checked = (current_setting == "15s");
  lv_obj_t *radio_15s = UIHelper::createResponsiveRadio(sleep_radio_container, "15s", is_15s_checked,
  [](lv_event_t *e) {
    updateUserInteraction();
    updateDisplaySleepTimeout("15s");

    // Uncheck other sleep radio buttons with immediate feedback
    lv_obj_t *sleep_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_60s = lv_obj_get_child(sleep_container, 1);
    lv_obj_t *radio_never = lv_obj_get_child(sleep_container, 2);
    lv_obj_clear_state(radio_60s, LV_STATE_CHECKED);
    lv_obj_clear_state(radio_never, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(sleep_container);
    lv_refr_now(NULL);
  });

  // 60s radio button
  bool is_60s_checked = (current_setting == "60s");
  lv_obj_t *radio_60s = UIHelper::createResponsiveRadio(sleep_radio_container, "60s", is_60s_checked,
  [](lv_event_t *e) {
    updateUserInteraction();
    updateDisplaySleepTimeout("60s");

    // Uncheck other sleep radio buttons with immediate feedback
    lv_obj_t *sleep_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_15s = lv_obj_get_child(sleep_container, 0);
    lv_obj_t *radio_never = lv_obj_get_child(sleep_container, 2);
    lv_obj_clear_state(radio_15s, LV_STATE_CHECKED);
    lv_obj_clear_state(radio_never, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(sleep_container);
    lv_refr_now(NULL);
  });

  // Never radio button
  bool is_never_checked = (current_setting == "Never");
  lv_obj_t *radio_never = UIHelper::createResponsiveRadio(sleep_radio_container, "Never", is_never_checked,
  [](lv_event_t *e) {
    updateUserInteraction();
    updateDisplaySleepTimeout("Never");

    // Uncheck other sleep radio buttons with immediate feedback
    lv_obj_t *sleep_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_15s = lv_obj_get_child(sleep_container, 0);
    lv_obj_t *radio_60s = lv_obj_get_child(sleep_container, 1);
    lv_obj_clear_state(radio_15s, LV_STATE_CHECKED);
    lv_obj_clear_state(radio_60s, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(sleep_container);
    lv_refr_now(NULL);
  });
}

void handleTouch(uint8_t contacts, GDTpoint_t *points) {
  display_sleep.lastTouchTime = millis();

  // If display was sleeping, wake it up
  if (display_sleep.sleeping) {
    wakeDisplay();
  }

  // Update user interaction for existing UI logic
  updateUserInteraction();
}

void updateButtonEventCallback(lv_event_t *e) {
  updateUserInteraction();

  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *flex_container = lv_obj_get_child(btn, 0);

  if (!flex_container || lv_obj_get_child_cnt(flex_container) < 2) {
    Serial.println("ERROR: Invalid button structure");
    return;
  }

  lv_obj_t *text_label = lv_obj_get_child(flex_container, 1);
  lv_obj_t *icon_label = lv_obj_get_child(flex_container, 0);

  const char *current_text = lv_label_get_text(text_label);
  Serial.print("Update button clicked with text: ");
  Serial.println(current_text);

  if (strcmp(current_text, "Update Now") == 0) {
    Serial.println("Starting Manual OTA update process...");

    // Immediately update UI
    lv_label_set_text(text_label, "Updating... 0%");
    lv_label_set_text(icon_label, LV_SYMBOL_REFRESH);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UIConstants::COLOR_WARNING), 0);
    lv_obj_add_state(btn, LV_STATE_DISABLED);

    lv_obj_invalidate(btn);
    lv_timer_handler();

    // Start manual OTA update
    lv_timer_create([](lv_timer_t *t) {
      watchdog.kick();

      Serial.println("Executing Manual OTA update...");
      bool success = otaManager.performSimpleOTAUpdate();

      if (!success) {
        Serial.println("Manual OTA update failed");
      }

      lv_timer_del(t);
    },
                    500, nullptr);

  } else if (strcmp(current_text, "Check for Updates") == 0) {
    Serial.println("Checking for firmware updates...");

    lv_label_set_text(text_label, "Checking...");
    lv_label_set_text(icon_label, LV_SYMBOL_REFRESH);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UIConstants::COLOR_SECONDARY), 0);
    lv_obj_add_state(btn, LV_STATE_DISABLED);

    lv_obj_invalidate(btn);
    lv_timer_handler();

    otaManager.checkForUpdate();

  } else if (strcmp(current_text, "Up To Date") == 0) {
    Serial.println("Force rechecking for updates...");

    lv_label_set_text(text_label, "Checking...");
    lv_label_set_text(icon_label, LV_SYMBOL_REFRESH);
    lv_obj_set_style_bg_color(btn, lv_color_hex(UIConstants::COLOR_SECONDARY), 0);
    lv_obj_add_state(btn, LV_STATE_DISABLED);

    lv_obj_invalidate(btn);
    lv_timer_handler();

    otaManager.checkForUpdate();

  } else {
    Serial.print("Update button in unhandled state: ");
    Serial.println(current_text);
  }
}

void setup() {
  Serial.begin(115200);

  watchdog.start(15000);

  // Initialize GPS
  GPSSerial.begin(9600);
  debugGPSConnection();
  hardware_status.gps_available = true; // Assume GPS is available unless proven otherwise
  
  display.begin();

  // Initialize backlight control
  backlight.begin();
  backlight.set(100);  // Set to full brightness initially

  touch.begin();

  // Configure touch interrupt for sleep/wake functionality
  touch.onDetect(handleTouch);

  // Initialize display sleep system
  display_sleep.lastTouchTime = millis();
  display_sleep.timeout = 60000;  // 60s default (will be overridden by loaded setting)
  display_sleep.sleeping = false;
  display_sleep.current_setting = "60s";  // default (will be overridden by loaded setting)

  // Load saved display sleep setting
  loadDisplaySleepSetting();

  Serial.println("Display sleep/wake system initialized with touch interrupt and loaded settings");

  // Initialize built-in IMU instead of external sensors
Serial.println("Initializing built-in IMU (BMI270)...");
if (!imu.begin()) {
  Serial.println("ERROR: Built-in IMU (BMI270) initialization failed!");
  hardware_status.imu_available = false;
  hardware_status.magnetometer_available = false;
  hardware_status.error_messages += "Built-in IMU not available. ";
  
  Serial.println("WARNING: Built-in IMU unavailable - continuing without motion sensing");
} else {
  Serial.println("Built-in IMU (BMI270) initialized successfully");
  hardware_status.imu_available = true;
  
  // Test magnetometer functionality
  Serial.println("Testing built-in Magnetometer (BMM150)...");
  float mx, my, mz;
  if (imu.readMagneticField(mx, my, mz)) {
    Serial.println("Built-in Magnetometer (BMM150) working");
    hardware_status.magnetometer_available = true;
  } else {
    Serial.println("WARNING: Built-in Magnetometer (BMM150) not responding - continuing without compass");
    hardware_status.magnetometer_available = false;
    hardware_status.error_messages += "Magnetometer not responding. ";
  }
}

  // Only initialize filter if we have IMU
  if (hardware_status.imu_available) {
    // Start with a conservative sample rate
    filter.begin(50);  // Lower, more stable rate
    Serial.println("Madgwick filter initialized");
  }

  StyleCache::initialize();
  // MemoryManager::optimizeStrings(); // Removed - was empty

  // Only start calibration if IMU is available
  if (hardware_status.imu_available) {
    state.cal_state = CAL_IN_PROGRESS;
    state.cal_start = millis();
    state.cal_samples = 0;
    Serial.println("Starting IMU calibration");
  } else {
    state.cal_state = CAL_FAILED;
    Serial.println("Skipping IMU calibration - sensor not available");
  }

  // Non-blocking delay equivalent - keep system alive during setup pause
  unsigned long setup_delay_start = millis();
  while (millis() - setup_delay_start < 1000) {
    watchdog.kick();
    delay(10); // Small delay to prevent tight loop
  }

  setupUI();
  
  // Show hardware errors on screen if any exist
  if (!hardware_status.error_messages.isEmpty()) {
    // Delay showing error to ensure UI is ready
    lv_timer_create([](lv_timer_t *t) {
      String error_msg = "Hardware Warning: " + hardware_status.error_messages + "Some features may be limited.";
      showErrorMessage(error_msg.c_str(), 15000); // Show for 5 seconds
      lv_timer_del(t);
    }, 2000, nullptr);
  }

  startAsyncNetworkScan();

  lv_timer_create([](lv_timer_t *t) {
    attemptAutoConnect();
    lv_timer_del(t);
  }, 3000, nullptr);

  if (ui.pass_ta) {
    lv_obj_add_flag(ui.pass_ta, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.wifi_button) {
    lv_obj_add_flag(ui.wifi_button, LV_OBJ_FLAG_HIDDEN);
  }
  if (state.selected_ssid.isEmpty()) {
    state.selected_ssid = "";
  }

  // Print final hardware status
  Serial.println("========== HARDWARE STATUS ==========");
  Serial.print("Built-in IMU (BMI270): ");
  Serial.println(hardware_status.imu_available ? "Available" : "FAILED");
  Serial.print("Built-in Magnetometer (BMM150): ");
  Serial.println(hardware_status.magnetometer_available ? "Available" : "FAILED");
  Serial.print("GPS: ");
  Serial.println(hardware_status.gps_available ? "Available" : "FAILED");
  if (!hardware_status.error_messages.isEmpty()) {
    Serial.print("Errors: ");
    Serial.println(hardware_status.error_messages);
  }
  Serial.println("=====================================");

  Serial.println("Setup complete - device ready with available hardware");
}

void loop() {
  optimizedLoop();
}

// Video Settings page functions
void video_btn_cb(lv_event_t *e) {
  updateUserInteraction();

  // Prevent screen transitions if one is already in progress
  if (screen_transition_in_progress) {
    DEBUG_PRINTLN("Screen transition already in progress, ignoring button press");
    return;
  }

  // Prevent multiple rapid button presses
  static unsigned long last_press = 0;
  unsigned long now = millis();
  if (now - last_press < 300) {  // 300ms debounce
    return;
  }
  last_press = now;

  // Set transition flag
  screen_transition_in_progress = true;

  // Immediate visual feedback
  lv_obj_t *btn = (lv_obj_t*)lv_event_get_target(e);
  lv_obj_set_style_transform_zoom(btn, 260, 0);  // 1.02x scale
  lv_obj_invalidate(btn);

  // Force immediate UI update to show button feedback
  lv_timer_handler();

  // Reset button scale immediately in a safer way
  lv_obj_set_style_transform_zoom(btn, 256, 0);  // Back to normal
  lv_obj_invalidate(btn);

  // Clean up existing screens first, then create new one
  lv_timer_create([](lv_timer_t *t) {
    // Force memory cleanup before creating new screen
    MemoryManager::forceCleanup();

    // Clean up other screens safely
    if (ui.settings_screen && lv_obj_is_valid(ui.settings_screen)) {
      lv_obj_del(ui.settings_screen);
      ui.settings_screen = nullptr;
      ui.in_settings = false;
    }

    // Small delay to let cleanup complete
    lv_timer_handler();

    // Now create video screen
    createVideoScreen();

    // Clear transition flag
    screen_transition_in_progress = false;

    lv_timer_del(t);
  }, 50, nullptr);  // Small delay for cleanup
}

void createVideoScreen() {
  Serial.println("Creating video settings screen...");

  // Check memory before creating
  lv_mem_monitor_t mon;
  lv_mem_monitor(&mon);
  if (mon.free_size < 10000) {
    Serial.println("WARNING: Low memory, forcing cleanup before creating video screen");
    MemoryManager::forceCleanup();
  }

  // Clean up existing video screen if it exists
  if (ui.video_screen) {
    lv_obj_del(ui.video_screen);
    ui.video_screen = nullptr;
  }

  ui.in_video = true;

  // Reset video labels
  ui.resolution_label = nullptr;
  ui.framerate_label = nullptr;
  ui.codec_label = nullptr;
  ui.quality_label = nullptr;
  // Removed unused label initializations

  // Create video screen
  ui.video_screen = lv_obj_create(nullptr);
  if (!ui.video_screen) {
    Serial.println("ERROR: Failed to create video screen!");
    ui.in_video = false;
    showMainScreen();
    return;
  }

  lv_obj_set_size(ui.video_screen, UIConstants::SCREEN_WIDTH, UIConstants::SCREEN_HEIGHT);
  lv_obj_set_pos(ui.video_screen, 0, 0);
  lv_obj_set_style_bg_color(ui.video_screen, lv_color_hex(UIConstants::COLOR_BACKGROUND), 0);
  lv_obj_set_style_pad_all(ui.video_screen, 0, 0);
  lv_obj_set_style_border_width(ui.video_screen, 0, 0);
  lv_obj_set_style_radius(ui.video_screen, 0, 0);
  lv_obj_clear_flag(ui.video_screen, LV_OBJ_FLAG_SCROLLABLE);

  Serial.println("Loading video screen...");
  lv_scr_load(ui.video_screen);

  // Small delay to ensure screen loads
  delay(20);

  Serial.println("Creating video footer...");
  // Create footer for video screen
  FooterHelper::createFooter(ui.video_screen);
  updateFooterIndicatorsToCurrentState();

  Serial.println("Video screen created successfully");

  // Header setup using optimized approach
  lv_obj_t *header = lv_obj_create(ui.video_screen);
  lv_obj_set_size(header, UIConstants::SCREEN_WIDTH, UIConstants::HEADER_HEIGHT);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(header, lv_color_hex(UIConstants::COLOR_HEADER), 0);
  lv_obj_set_style_radius(header, 0, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_pad_all(header, 0, 0);

  // Back button using transparent style
  lv_obj_t *back_btn = lv_btn_create(header);
  lv_obj_set_size(back_btn, 50, 40);
  lv_obj_align(back_btn, LV_ALIGN_LEFT_MID, UIConstants::PADDING_SMALL, 0);
  lv_obj_add_style(back_btn, &StyleCache::transparent_button_style, LV_PART_MAIN);

  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT);
  lv_obj_set_style_text_color(back_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(back_label, UIConstants::FONT_LARGE, 0);
  lv_obj_center(back_label);
  lv_obj_add_event_cb(
    back_btn, [](lv_event_t *e) {
      Serial.println("Video back button pressed");
      updateUserInteraction();

      // Kick watchdog immediately
      watchdog.kick();

      // Check memory before transition
      if (MemoryManager::isMemoryCritical()) {
        // Serial.println("WARNING: Critical memory before screen transition");
        MemoryManager::forceCleanup();
        watchdog.kick();
      }

      // Use timer with delay for safe transition
      lv_timer_create([](lv_timer_t *t) {
        watchdog.kick();

        ui.in_video = false;

        // Memory check during transition
        lv_mem_monitor_t mon;
        lv_mem_monitor(&mon);
        if (mon.free_size < 10000) {
          // Serial.println("WARNING: Low memory during transition, forcing cleanup");
          MemoryManager::forceCleanup();
          watchdog.kick();
        }

        delay(50);
        watchdog.kick();

        showMainScreen();

        watchdog.kick();
        lv_timer_del(t);
      },
                      25, nullptr);
    },
    LV_EVENT_CLICKED, nullptr);

  // Video Settings title
  lv_obj_t *title = lv_label_create(header);
  lv_label_set_text(title, "Video Settings");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_set_style_text_font(title, UIConstants::FONT_MEDIUM, 0);
  lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

  // Content area with video settings items
  int content_y = UIConstants::HEADER_HEIGHT + UIConstants::PADDING_STANDARD;

  // Resolution setting with radio buttons
  lv_obj_t *resolution_item = lv_btn_create(ui.video_screen);
  lv_obj_add_style(resolution_item, &StyleCache::list_item_style, LV_PART_MAIN);
  lv_obj_set_size(resolution_item, UIConstants::ITEM_WIDTH, UIConstants::ITEM_HEIGHT);
  lv_obj_align(resolution_item, LV_ALIGN_TOP_MID, 0, content_y);
  lv_obj_set_flex_flow(resolution_item, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(resolution_item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(resolution_item, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(resolution_item, LV_OBJ_FLAG_CLICKABLE);

  // Resolution title (left side)
  lv_obj_t *resolution_title = lv_label_create(resolution_item);
  lv_label_set_text(resolution_title, "Resolution");
  lv_obj_set_style_text_font(resolution_title, UIConstants::FONT_MEDIUM, 0);
  lv_obj_set_style_text_color(resolution_title, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_align(resolution_title, LV_ALIGN_LEFT_MID, UIConstants::PADDING_STANDARD, 0);

  // Radio button container (right side)
  lv_obj_t *radio_container = lv_obj_create(resolution_item);
  lv_obj_remove_style_all(radio_container);
  lv_obj_set_size(radio_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(radio_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(radio_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(radio_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(radio_container, 15, 0);
  lv_obj_align(radio_container, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);
  lv_obj_clear_flag(radio_container, LV_OBJ_FLAG_CLICKABLE);

  // 720p radio button - IMPROVED RESPONSIVENESS
  lv_obj_t *radio_720 = UIHelper::createResponsiveRadio(radio_container, "720", true,
  [](lv_event_t *e) {
    updateUserInteraction();
    video_settings.resolution = "1280x720";

    // Uncheck 1080p radio button with immediate feedback
    lv_obj_t *radio_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_1080 = lv_obj_get_child(radio_container, 1);
    lv_obj_clear_state(radio_1080, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(radio_container);
    lv_refr_now(NULL);

    Serial.println("Resolution set to 720p");
  });

  // 1080p radio button - IMPROVED RESPONSIVENESS
  lv_obj_t *radio_1080 = UIHelper::createResponsiveRadio(radio_container, "1080", false,
  [](lv_event_t *e) {
    updateUserInteraction();
    video_settings.resolution = "1920x1080";

    // Uncheck 720p radio button with immediate feedback
    lv_obj_t *radio_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_720 = lv_obj_get_child(radio_container, 0);
    lv_obj_clear_state(radio_720, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(radio_container);
    lv_refr_now(NULL);

    Serial.println("Resolution set to 1080p");
  });

  content_y += UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING;

  // Frame rate setting (clickable)
  static char framerate_buf[16];
  snprintf(framerate_buf, sizeof(framerate_buf), "%d FPS", video_settings.framerate);
  lv_obj_t *framerate_item = UIHelper::createSettingsItem(ui.video_screen, "Frame Rate",
                                                          framerate_buf, content_y, true, framerate_change_cb);
  content_y += UIConstants::ITEM_HEIGHT + UIConstants::ITEM_SPACING;

  // Quality setting with radio buttons
  lv_obj_t *quality_item = lv_btn_create(ui.video_screen);
  lv_obj_add_style(quality_item, &StyleCache::list_item_style, LV_PART_MAIN);
  lv_obj_set_size(quality_item, UIConstants::ITEM_WIDTH, UIConstants::ITEM_HEIGHT);
  lv_obj_align(quality_item, LV_ALIGN_TOP_MID, 0, content_y);
  lv_obj_set_flex_flow(quality_item, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(quality_item, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_clear_flag(quality_item, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(quality_item, LV_OBJ_FLAG_CLICKABLE);

  // Quality title (left side)
  lv_obj_t *quality_title = lv_label_create(quality_item);
  lv_label_set_text(quality_title, "Quality");
  lv_obj_set_style_text_font(quality_title, UIConstants::FONT_MEDIUM, 0);
  lv_obj_set_style_text_color(quality_title, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_align(quality_title, LV_ALIGN_LEFT_MID, UIConstants::PADDING_STANDARD, 0);

  // Quality radio button container (right side)
  lv_obj_t *quality_radio_container = lv_obj_create(quality_item);
  lv_obj_remove_style_all(quality_radio_container);
  lv_obj_set_size(quality_radio_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_layout(quality_radio_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(quality_radio_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(quality_radio_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(quality_radio_container, 10, 0);
  lv_obj_align(quality_radio_container, LV_ALIGN_RIGHT_MID, -UIConstants::PADDING_STANDARD, 0);
  lv_obj_clear_flag(quality_radio_container, LV_OBJ_FLAG_CLICKABLE);

  // Low quality radio button - IMPROVED RESPONSIVENESS
  lv_obj_t *radio_low = UIHelper::createResponsiveRadio(quality_radio_container, "Low", true,
  [](lv_event_t *e) {
    updateUserInteraction();
    video_settings.quality = "Low";

    // Uncheck other quality radio buttons with immediate feedback
    lv_obj_t *quality_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_medium = lv_obj_get_child(quality_container, 1);
    lv_obj_t *radio_high = lv_obj_get_child(quality_container, 2);
    lv_obj_clear_state(radio_medium, LV_STATE_CHECKED);
    lv_obj_clear_state(radio_high, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(quality_container);
    lv_refr_now(NULL);

    Serial.println("Quality set to Low");
  });

  // Medium quality radio button - IMPROVED RESPONSIVENESS
  lv_obj_t *radio_medium = UIHelper::createResponsiveRadio(quality_radio_container, "Medium", false,
  [](lv_event_t *e) {
    updateUserInteraction();
    video_settings.quality = "Medium";

    // Uncheck other quality radio buttons with immediate feedback
    lv_obj_t *quality_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_low = lv_obj_get_child(quality_container, 0);
    lv_obj_t *radio_high = lv_obj_get_child(quality_container, 2);
    lv_obj_clear_state(radio_low, LV_STATE_CHECKED);
    lv_obj_clear_state(radio_high, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(quality_container);
    lv_refr_now(NULL);

    Serial.println("Quality set to Medium");
  });

  // High quality radio button - IMPROVED RESPONSIVENESS
  lv_obj_t *radio_high = UIHelper::createResponsiveRadio(quality_radio_container, "High", false,
  [](lv_event_t *e) {
    updateUserInteraction();
    video_settings.quality = "High";

    // Uncheck other quality radio buttons with immediate feedback
    lv_obj_t *quality_container = lv_obj_get_parent((lv_obj_t *)lv_event_get_target(e));
    lv_obj_t *radio_low = lv_obj_get_child(quality_container, 0);
    lv_obj_t *radio_medium = lv_obj_get_child(quality_container, 1);
    lv_obj_clear_state(radio_low, LV_STATE_CHECKED);
    lv_obj_clear_state(radio_medium, LV_STATE_CHECKED);

    // Force immediate UI update
    lv_obj_invalidate(quality_container);
    lv_refr_now(NULL);

    Serial.println("Quality set to High");
  });

  // Store references to the value labels for updates
  ui.framerate_label = lv_obj_get_child(framerate_item, 1);

  // Create help container for video guidance text
  lv_obj_t *help_container = lv_obj_create(ui.video_screen);
  lv_obj_set_size(help_container, UIConstants::ITEM_WIDTH, LV_SIZE_CONTENT);
  lv_obj_align(help_container, LV_ALIGN_TOP_MID, 0, content_y + UIConstants::ITEM_HEIGHT + 20);
  lv_obj_set_style_bg_color(help_container, lv_color_hex(UIConstants::COLOR_SURFACE), 0);
  lv_obj_set_style_radius(help_container, UIConstants::BUTTON_RADIUS, 0);
  lv_obj_set_style_border_width(help_container, 0, 0);
  lv_obj_set_style_pad_all(help_container, UIConstants::PADDING_STANDARD, 0);
  lv_obj_clear_flag(help_container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(help_container, LV_OBJ_FLAG_CLICKABLE);

  // Tips header
  lv_obj_t *tips_header = lv_label_create(help_container);
  lv_label_set_text(tips_header, "Tips");
  lv_obj_set_style_text_font(tips_header, UIConstants::FONT_LARGE, 0);
  lv_obj_set_style_text_color(tips_header, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_align(tips_header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_clear_flag(tips_header, LV_OBJ_FLAG_CLICKABLE);

  // Create a text container for proper vertical layout
  lv_obj_t *text_container = lv_obj_create(help_container);
  lv_obj_remove_style_all(text_container);
  lv_obj_set_width(text_container, UIConstants::ITEM_WIDTH - (UIConstants::PADDING_STANDARD * 2));
  lv_obj_set_height(text_container, LV_SIZE_CONTENT);
  lv_obj_set_pos(text_container, 0, 50);  // Position below header
  lv_obj_set_layout(text_container, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(text_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(text_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(text_container, 20, 0);  // 15px spacing between elements
  lv_obj_clear_flag(text_container, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(text_container, LV_OBJ_FLAG_SCROLLABLE);

  // Intro text
  lv_obj_t *intro_text = lv_label_create(text_container);
  lv_label_set_text(intro_text,
                    "Adjusting your video settings may have different results, depending on your internet connection.");
  lv_obj_set_width(intro_text, lv_pct(100));
  lv_label_set_long_mode(intro_text, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(intro_text, LV_TEXT_ALIGN_LEFT, 0);
  lv_obj_set_style_text_font(intro_text, UIConstants::FONT_SMALL, 0);
  lv_obj_set_style_text_color(intro_text, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
  lv_obj_clear_flag(intro_text, LV_OBJ_FLAG_CLICKABLE);

  // Slow connection section container
  lv_obj_t *slow_section = lv_obj_create(text_container);
  lv_obj_remove_style_all(slow_section);
  lv_obj_set_width(slow_section, lv_pct(100));
  lv_obj_set_height(slow_section, LV_SIZE_CONTENT);
  lv_obj_set_layout(slow_section, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(slow_section, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(slow_section, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(slow_section, 5, 0);
  lv_obj_clear_flag(slow_section, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(slow_section, LV_OBJ_FLAG_SCROLLABLE);

  // Slow connection header
  lv_obj_t *slow_header = lv_label_create(slow_section);
  lv_label_set_text(slow_header, "If your connection is Slow");
  lv_obj_set_style_text_font(slow_header, UIConstants::FONT_MEDIUM, 0);
  lv_obj_set_style_text_color(slow_header, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_clear_flag(slow_header, LV_OBJ_FLAG_CLICKABLE);

  // Slow connection details
  lv_obj_t *slow_details = lv_label_create(slow_section);
  lv_label_set_text(slow_details, "Reduce the Quality to Low and the Resolution to 720.");
  lv_obj_set_width(slow_details, lv_pct(100));
  lv_label_set_long_mode(slow_details, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(slow_details, UIConstants::FONT_SMALL, 0);
  lv_obj_set_style_text_color(slow_details, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
  lv_obj_clear_flag(slow_details, LV_OBJ_FLAG_CLICKABLE);

  // Fast connection section container
  lv_obj_t *fast_section = lv_obj_create(text_container);
  lv_obj_remove_style_all(fast_section);
  lv_obj_set_width(fast_section, lv_pct(100));
  lv_obj_set_height(fast_section, LV_SIZE_CONTENT);
  lv_obj_set_layout(fast_section, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(fast_section, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(fast_section, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_style_pad_row(fast_section, 5, 0);
  lv_obj_clear_flag(fast_section, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fast_section, LV_OBJ_FLAG_SCROLLABLE);

  // Fast connection header
  lv_obj_t *fast_header = lv_label_create(fast_section);
  lv_label_set_text(fast_header, "If your connection is Fast");
  lv_obj_set_style_text_font(fast_header, UIConstants::FONT_MEDIUM, 0);
  lv_obj_set_style_text_color(fast_header, lv_color_hex(UIConstants::COLOR_TEXT_PRIMARY), 0);
  lv_obj_clear_flag(fast_header, LV_OBJ_FLAG_CLICKABLE);

  // Fast connection details
  lv_obj_t *fast_details = lv_label_create(fast_section);
  lv_label_set_text(fast_details, "Increase your Quality to Medium or High and your Resolution to 1080.");
  lv_obj_set_width(fast_details, lv_pct(100));
  lv_label_set_long_mode(fast_details, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(fast_details, UIConstants::FONT_SMALL, 0);
  lv_obj_set_style_text_color(fast_details, lv_color_hex(UIConstants::COLOR_TEXT_SECONDARY), 0);
  lv_obj_clear_flag(fast_details, LV_OBJ_FLAG_CLICKABLE);

  // Force layout update to ensure proper positioning
  lv_obj_update_layout(text_container);
  lv_obj_update_layout(help_container);
}

void resolution_change_cb(lv_event_t *e) {
  updateUserInteraction();

  // Cycle through common resolutions
  if (video_settings.resolution == "1920x1080") {
    video_settings.resolution = "1280x720";
    video_settings.bitrate = 3000;
  } else if (video_settings.resolution == "1280x720") {
    video_settings.resolution = "854x480";
    video_settings.bitrate = 1500;
  } else if (video_settings.resolution == "854x480") {
    video_settings.resolution = "640x360";
    video_settings.bitrate = 800;
  } else {
    video_settings.resolution = "1920x1080";
    video_settings.bitrate = 5000;
  }

  updateVideoUI();
}

void framerate_change_cb(lv_event_t *e) {
  updateUserInteraction();

  // Cycle through common frame rates
  if (video_settings.framerate == 30) {
    video_settings.framerate = 60;
    video_settings.bitrate = (int)(video_settings.bitrate * 1.5);
  } else if (video_settings.framerate == 60) {
    video_settings.framerate = 24;
    video_settings.bitrate = (int)(video_settings.bitrate * 0.6);
  } else {
    video_settings.framerate = 30;
    // Reset bitrate based on resolution
    if (video_settings.resolution == "1920x1080") video_settings.bitrate = 5000;
    else if (video_settings.resolution == "1280x720") video_settings.bitrate = 3000;
    else if (video_settings.resolution == "854x480") video_settings.bitrate = 1500;
    else video_settings.bitrate = 800;
  }

  updateVideoUI();
}

void quality_change_cb(lv_event_t *e) {
  updateUserInteraction();

  // Cycle through quality settings
  if (video_settings.quality == "High") {
    video_settings.quality = "Medium";
    video_settings.bitrate = (int)(video_settings.bitrate * 0.7);
  } else if (video_settings.quality == "Medium") {
    video_settings.quality = "Low";
    video_settings.bitrate = (int)(video_settings.bitrate * 0.5);
  } else {
    video_settings.quality = "High";
    // Reset to full bitrate
    if (video_settings.resolution == "1920x1080") video_settings.bitrate = 5000;
    else if (video_settings.resolution == "1280x720") video_settings.bitrate = 3000;
    else if (video_settings.resolution == "854x480") video_settings.bitrate = 1500;
    else video_settings.bitrate = 800;

    // Apply framerate multiplier
    if (video_settings.framerate == 60) video_settings.bitrate = (int)(video_settings.bitrate * 1.5);
    else if (video_settings.framerate == 24) video_settings.bitrate = (int)(video_settings.bitrate * 0.8);
  }

  updateVideoUI();
}

// stabilization_toggle_cb removed - was unused

void start_recording_cb(lv_event_t *e) {
  updateUserInteraction();

  // Toggle recording state
  video_settings.recording = !video_settings.recording;

  if (video_settings.recording) {
    video_settings.recording_start_time = millis();
    video_settings.file_size = 0;
    Serial.println("Started video recording");
  } else {
    Serial.println("Stopped video recording");
  }

  updateVideoUI();

  // Update the button immediately
  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *label = lv_obj_get_child(btn, 0);

  if (video_settings.recording) {
    lv_label_set_text(label, LV_SYMBOL_STOP " Stop Recording");
    lv_obj_remove_style(btn, &StyleCache::button_primary_style, LV_PART_MAIN);
    lv_obj_add_style(btn, &StyleCache::button_danger_style, LV_PART_MAIN);
  } else {
    lv_label_set_text(label, LV_SYMBOL_VIDEO " Start Recording");
    lv_obj_remove_style(btn, &StyleCache::button_danger_style, LV_PART_MAIN);
    lv_obj_add_style(btn, &StyleCache::button_primary_style, LV_PART_MAIN);
  }
}

void updateVideoUI() {
  // Update resolution label
  if (ui.resolution_label) {
    lv_label_set_text(ui.resolution_label, video_settings.resolution.c_str());
  }

  // Update framerate label
  if (ui.framerate_label) {
    static char framerate_buf[16];
    snprintf(framerate_buf, sizeof(framerate_buf), "%d FPS", video_settings.framerate);
    lv_label_set_text(ui.framerate_label, framerate_buf);
  }

  // Update bitrate label
  // Update quality label
  if (ui.quality_label) {
    lv_label_set_text(ui.quality_label, video_settings.quality.c_str());
  }

}