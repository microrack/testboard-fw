#include "hal.h"
#include "esp_log.h"
#include <SPI.h>
#include <DAC8552.h>

static const char* TAG = "hal";

// Global objects
SPIClass SPI_DAC(HSPI);
DAC8552 dac1(PIN_CS1, &SPI_DAC);
DAC8552 dac2(PIN_CS2, &SPI_DAC);
Adafruit_MCP23X17 mcp0;  // addr 0x20
Adafruit_MCP23X17 mcp1;  // addr 0x21

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

void dac_init() {
    // Initialize CS pins
    pinMode(PIN_CS1, OUTPUT);
    pinMode(PIN_CS2, OUTPUT);
    digitalWrite(PIN_CS1, HIGH);
    digitalWrite(PIN_CS2, HIGH);

    // Initialize SPI
    SPI_DAC.begin(PIN_SCK, -1, PIN_MOSI, -1); // SCK, MISO (none), MOSI, SS (not used)
    dac1.begin();
    dac2.begin();
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

void mcp_init() {
    // Initialize reset pins
    pinMode(PIN_IO0_RST, OUTPUT);
    pinMode(PIN_IO1_RST, OUTPUT);

    // Reset sequence for MCP0
    digitalWrite(PIN_IO0_RST, LOW);   // hold in reset
    delay(10);
    digitalWrite(PIN_IO0_RST, HIGH);  // release reset

    // Reset sequence for MCP1
    digitalWrite(PIN_IO1_RST, LOW);   // hold in reset
    delay(10);
    digitalWrite(PIN_IO1_RST, HIGH);  // release reset

    // Initialize MCP1
    if (!mcp1.begin_I2C(MCP_ADDR_1)) {
        ESP_LOGE(TAG, "Error initializing MCP1");
        return;
    }

    // Configure MCP1 pins
    mcp1.pinMode(PIN_LED1, OUTPUT);
    mcp1.pinMode(PIN_LED2, OUTPUT);
    mcp1.pinMode(PIN_P12V_PASS, INPUT);
    mcp1.pinMode(PIN_P5V_PASS, INPUT);
    mcp1.pinMode(PIN_M12V_PASS, INPUT);
} 