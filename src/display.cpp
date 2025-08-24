#include "display.h"
#include "board.h"
#include "esp_log.h"

static const char* TAG = "display";

// Global display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool display_init() {
    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        ESP_LOGE(TAG, "SSD1306 allocation failed");
        return false;
    }

    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.setRotation(0);
    display_clear();

    ESP_LOGD(TAG, "Display initialized successfully");
    return true;
}

void display_printf(const char* format, ...) {
    // Clear display before printing
    display_clear();

    // Format message for console
    va_list args;
    va_start(args, format);
    char console_msg[256];
    vsnprintf(console_msg, sizeof(console_msg), format, args);
    va_end(args);
    // ESP_LOGI(TAG, "%s", console_msg);

    // Format message for display
    va_start(args, format);
    char display_msg[256];
    vsnprintf(display_msg, sizeof(display_msg), format, args);
    va_end(args);

    // Print to display
    display.printf("%s", display_msg);
    display.display();
}

void display_clear() {
    display.clearDisplay();
    display.setCursor(0, 0);
} 