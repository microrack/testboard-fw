#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdarg.h>

// External display object
extern Adafruit_SSD1306 display;

/**
 * @brief Initialize the display
 * @return true if display initialization was successful
 */
bool display_init();

/**
 * @brief Print formatted message to both display and console
 * @param format Format string (like printf)
 * @param ... Variable arguments for formatting
 */
void display_printf(const char* format, ...);

/**
 * @brief Clear the display and set cursor to top-left
 */
void display_clear(); 