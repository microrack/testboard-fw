#include <Arduino.h>
#include <esp_log.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_MCP23X17.h>
#include <DAC8552.h>

#include "board.h"
#include "hal.h"

static const char* TAG = "main";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// External objects from hal.cpp
extern DAC8552 dac1;
extern DAC8552 dac2;

void setup() {
    // Initialize serial port
    Serial.begin(115200);
    delay(1000);  // Small delay for startup

    // Initialize ESP logging
    esp_log_level_set("*", ESP_LOG_DEBUG);
    esp_log_level_set("hal", ESP_LOG_INFO);  // Set log level for hal tag

    // Initialize hardware abstraction layer
    hal_init();

    // Calibrate current measurements
    hal_current_calibrate();

    // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
    if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        ESP_LOGE(TAG, "SSD1306 allocation failed");
        for(;;); // Don't proceed, loop forever
    }

    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE); // Draw white text
    display.cp437(true);         // Use full 256 char 'Code Page 437' font
    display.setRotation(0);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("Hello, Microrack!\n");
    display.display();

    // Initialize MCP IO expanders
    mcp_init();

    // Initialize DAC
    dac_init();

    // Configure INA196A pins as inputs
    pinMode(PIN_INA_12V, INPUT);
    pinMode(PIN_INA_5V, INPUT);
    pinMode(PIN_INA_M12V, INPUT);

    // Console output
    ESP_LOGI(TAG, "Hello, Microrack!");

    mcp1.digitalWrite(PIN_LED1, HIGH);
    mcp1.digitalWrite(PIN_LED2, HIGH);
}

void loop() {
    Serial.printf("\n\n\n\n===============\n");
    static bool state = false;

    // Keep LEDs on constantly
    mcp1.digitalWrite(PIN_LED1, state);
    mcp1.digitalWrite(PIN_LED2, state);

    // ESP_LOGI(TAG, "\n--- LED State ---");
    ESP_LOGI(TAG, "LEDS: %d", state);
    delay(1); // Small delay to let the current stabilize
    measureCurrent(PIN_INA_5V);

    // Measure and print currents
    ESP_LOGI(TAG, "Currents (µA) - +12V: %d, +5V: %d, -12V: %d",
        measureCurrent(PIN_INA_12V),
        measureCurrent(PIN_INA_5V),
        measureCurrent(PIN_INA_M12V)
    );

    ESP_LOGI(TAG, "Power rails - +12: %d +5: %d -12: %d",
        mcp1.digitalRead(PIN_P12V_PASS),
        mcp1.digitalRead(PIN_P5V_PASS),
        !mcp1.digitalRead(PIN_M12V_PASS)
    );

    static uint16_t value = 0;
    dac1.setValue(0, value);
    dac1.setValue(1, value);
    dac2.setValue(0, value);
    dac2.setValue(1, value);
    value += 5000;
    if(value > 65536 - 5000) {
        value = 0;
    }
    
    state = !state;

    delay(100);
}
