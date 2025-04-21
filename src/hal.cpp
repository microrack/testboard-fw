#include "hal.h"
#include "esp_log.h"
#include <SPI.h>
#include <DAC8552.h>
#include <algorithm>

static const char* TAG = "hal";

// Global objects
SPIClass SPI_DAC(HSPI);
DAC8552 dac1(PIN_CS1, &SPI_DAC);
DAC8552 dac2(PIN_CS2, &SPI_DAC);
Adafruit_MCP23X17 mcp0;  // addr 0x20
Adafruit_MCP23X17 mcp1;  // addr 0x21

// Median filter buffer size
#define MEDIAN_FILTER_SIZE 15

void hal_init() {

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

// Helper function to get median value from an array
int getMedian(int arr[], int size) {
    // Create a copy of the array to avoid modifying the original
    int temp[size];
    for(int i = 0; i < size; i++) {
        temp[i] = arr[i];
    }
    
    // Sort the array
    std::sort(temp, temp + size);
    
    // Return the middle element
    return temp[size / 2];
}

int32_t measureCurrent(uint8_t pin) {
    // Apply median filter
    int samples[MEDIAN_FILTER_SIZE];
    
    // Take multiple samples
    for(int i = 0; i < MEDIAN_FILTER_SIZE; i++) {
        int raw = analogRead(pin);
        samples[i] = raw;
        delayMicroseconds(100); // Small delay between samples
    }
    
    // Get median value
    int raw = getMedian(samples, MEDIAN_FILTER_SIZE);

    int32_t voltage = raw * 3300 / 4095;

    int32_t current = (voltage * 1000) / (SHUNT_RESISTOR * INA196_GAIN);
    
    ESP_LOGI(TAG, "Raw ADC value: %d, voltage: %d mV, current: %d uA", raw, voltage, current);
    return current;
} 