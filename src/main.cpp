#include <Arduino.h>
#include <esp_log.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_MCP23X17.h>

#include <DAC8552.h>

#include "board.h"

static const char* TAG = "testboard";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_MCP23X17 mcp0;  // addr 0x20
Adafruit_MCP23X17 mcp1;  // addr 0x21

SPIClass SPI_DAC(HSPI);
DAC8552 dac1(PIN_CS1, &SPI_DAC);
DAC8552 dac2(PIN_CS2, &SPI_DAC);

// Function to measure current in microamps
int measureCurrent(int pin) {
    // Read analog value (0-4095 for ESP32)
    int rawValue = analogRead(pin);
    float voltage = (rawValue * 3.3) / 4095.0;
    
    ESP_LOGD(TAG, "Raw ADC: %d (%.3fV)", rawValue, voltage);
    
    // Calculate current through shunt
    // I = V / (R * Gain)
    // Convert to microamps and round to integer
    int current = (int)((voltage / (SHUNT_RESISTOR * INA196_GAIN)) * 1000000);
    
    return current;
}

void setup() {
    // Initialize serial port
    Serial.begin(115200);
    delay(1000);  // Small delay for startup

    // Initialize ESP logging
    esp_log_level_set("*", ESP_LOG_INFO);

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

    pinMode(PIN_IO0_RST, OUTPUT);
    digitalWrite(PIN_IO0_RST, LOW);   // удерживаем в сбросе
    delay(10);
    digitalWrite(PIN_IO0_RST, HIGH);  // выходим из сброса

    pinMode(PIN_IO1_RST, OUTPUT);
    digitalWrite(PIN_IO1_RST, LOW);   // удерживаем в сбросе
    delay(10);
    digitalWrite(PIN_IO1_RST, HIGH);  // выходим из сброса

    mcp1.begin_I2C(MCP_ADDR_1);
    mcp1.pinMode(PIN_LED1, OUTPUT);
    mcp1.pinMode(PIN_LED2, OUTPUT);

    mcp1.pinMode(PIN_P12V_PASS, INPUT);
    mcp1.pinMode(PIN_P5V_PASS, INPUT);
    mcp1.pinMode(PIN_M12V_PASS, INPUT);

    // Инициализация пинов CS
    pinMode(PIN_CS1, OUTPUT);
    pinMode(PIN_CS2, OUTPUT);
    digitalWrite(PIN_CS1, HIGH);
    digitalWrite(PIN_CS2, HIGH);

    // Инициализация SPI
    SPI_DAC.begin(PIN_SCK, -1, PIN_MOSI, -1); // SCK, MISO (нет), MOSI, SS (не используется)
    dac1.begin();
    dac2.begin();

    // Configure INA196A pins as inputs
    pinMode(PIN_INA_12V, INPUT);
    pinMode(PIN_INA_5V, INPUT);
    pinMode(PIN_INA_M12V, INPUT);

    // Console output
    ESP_LOGI(TAG, "Hello, Microrack!");
}

void writeDAC(int cs_pin, uint8_t channel, uint16_t value) {
    uint8_t command = 0x00;
    if (channel == 0) command = 0x00;         // DAC A
    else if (channel == 1) command = 0x10;    // DAC B
    else if (channel == 2) command = 0x20;    // both (A+B)

    uint8_t highByte = (value >> 8) & 0xFF;
    uint8_t lowByte  = value & 0xFF;

    digitalWrite(cs_pin, LOW);
    SPI_DAC.transfer(command);
    SPI_DAC.transfer(highByte);
    SPI_DAC.transfer(lowByte);
    digitalWrite(cs_pin, HIGH);
    delayMicroseconds(100);
}

void loop() {
    static bool state = false;

    mcp1.digitalWrite(PIN_LED1, state);
    mcp1.digitalWrite(PIN_LED2, state);

    ESP_LOGI(TAG, "\n--- LED State Change ---");
    ESP_LOGI(TAG, "%s", state ? "LED1 ON, LED2 ON" : "LED1 OFF, LED2 OFF");
    delay(100); // Small delay to let the current stabilize

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
    delay(500);
}
