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

// Reference values for current calibration
static int32_t ref_current_12v = 0;
static int32_t ref_current_5v = 0;
static int32_t ref_current_m12v = 0;

// Reference values for ADC calibration
static int32_t ref_adc_values[ADC_sink_count] = {0};

// Median filter buffer size
#define MEDIAN_FILTER_SIZE 15

void hal_init() {
    // Initialize serial port
    Serial.begin(921600);
    delay(1000);  // Small delay for startup

    // Initialize ESP logging
    esp_log_level_set("*", ESP_LOG_DEBUG);
    esp_log_level_set("hal", ESP_LOG_INFO);  // Set log level for hal tag

    // Initialize ADC pins
    for(ADC_sink_t idx = (ADC_sink_t)0; idx < ADC_sink_count; idx = (ADC_sink_t)(idx + 1)) {
        pinMode(ADC_PINS[idx], INPUT);
    }

    // Configure INA196A pins as inputs
    pinMode(PIN_INA_12V, INPUT);
    pinMode(PIN_INA_5V, INPUT);
    pinMode(PIN_INA_M12V, INPUT);

    // Initialize DAC
    dac_init();

    // Initialize MCP
    mcp_init();

    // Turn on both LEDs to indicate startup
    mcp1.digitalWrite(PIN_LED_OK, HIGH);
    mcp1.digitalWrite(PIN_LED_FAIL, HIGH);

    ESP_LOGI(TAG, "Hardware initialization complete");
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

void write_dac(int cs_pin, uint8_t channel, uint16_t value) {
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

    // Initialize MCP0
    if (!mcp0.begin_I2C(MCP_ADDR_0)) {
        ESP_LOGE(TAG, "Error initializing MCP0");
        return;
    }

    // Configure all MCP0 pins as outputs
    for(int i = 0; i < 16; i++) {
        mcp0.pinMode(i, OUTPUT);
    }

    // Initialize MCP1
    if (!mcp1.begin_I2C(MCP_ADDR_1)) {
        ESP_LOGE(TAG, "Error initializing MCP1");
        return;
    }

    // Configure MCP1 pins
    mcp1.pinMode(PIN_LED_OK, OUTPUT);
    mcp1.pinMode(PIN_LED_FAIL, OUTPUT);
    mcp1.pinMode(PIN_P12V_PASS, INPUT);
    mcp1.pinMode(PIN_P5V_PASS, INPUT);
    mcp1.pinMode(PIN_M12V_PASS, INPUT);

    // Configure PD sink pins as outputs
    mcp1.pinMode(PIN_SINK_PD_A, OUTPUT);
    mcp1.pinMode(PIN_SINK_PD_B, OUTPUT);
    mcp1.pinMode(PIN_SINK_PD_C, OUTPUT);

    // Configure ID pins (GPA0-GPA4) with pullup
    for(int i = 0; i < 5; i++) {
        mcp1.pinMode(i, INPUT_PULLUP);
    }
}

// Helper function to get median value from an array
int get_median(int arr[], int size) {
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

void hal_adc_calibrate() {
    ESP_LOGD(TAG, "Starting ADC calibration...");
    
    // Take multiple measurements for each ADC pin and average them
    const int CALIB_SAMPLES = 10;
    
    // First calibrate 1k sinks
    for(ADC_sink_t idx = ADC_sink_1k_A; idx <= ADC_sink_1k_F; idx = (ADC_sink_t)(idx + 1)) {
        int32_t sum = 0;
        
        for(int i = 0; i < CALIB_SAMPLES; i++) {
            int samples[MEDIAN_FILTER_SIZE];
            
            // Take multiple samples with median filter
            for(int j = 0; j < MEDIAN_FILTER_SIZE; j++) {
                samples[j] = analogRead(ADC_PINS[idx]);
                delayMicroseconds(100);
            }
            
            sum += get_median(samples, MEDIAN_FILTER_SIZE);
            delay(1);
        }
        
        ref_adc_values[idx] = sum / CALIB_SAMPLES;
    }
    
    // Calculate average of 1k sink values
    int32_t avg_1k = 0;
    for(ADC_sink_t idx = ADC_sink_1k_A; idx <= ADC_sink_1k_F; idx = (ADC_sink_t)(idx + 1)) {
        avg_1k += ref_adc_values[idx];
    }
    avg_1k /= 6; // Number of 1k sinks
    
    ESP_LOGD(TAG, "Average 1k sink value: %d", avg_1k);
    
    // Use average 1k value for PD and Z sinks
    for(ADC_sink_t idx = ADC_sink_PD_A; idx <= ADC_sink_Z_F; idx = (ADC_sink_t)(idx + 1)) {
        ref_adc_values[idx] = avg_1k;
    }
    
    ESP_LOGD(TAG, "ADC calibration complete. Reference values:");
    for(ADC_sink_t idx = (ADC_sink_t)0; idx < ADC_sink_count; idx = (ADC_sink_t)(idx + 1)) {
        ESP_LOGD(TAG, "ADC %d: raw=%d", idx, ref_adc_values[idx]);
    }
}

int32_t hal_adc_read(ADC_sink_t idx) {
    if (idx >= ADC_sink_count) {
        ESP_LOGE(TAG, "Invalid ADC sink index");
        return 0;
    }
    
    uint8_t pin = ADC_PINS[idx];
    
    // Apply median filter
    int samples[MEDIAN_FILTER_SIZE];
    
    // Take multiple samples
    for(int i = 0; i < MEDIAN_FILTER_SIZE; i++) {
        samples[i] = analogRead(pin);
        delayMicroseconds(1); // Small delay between samples
    }
    
    // Get median value
    int raw = get_median(samples, MEDIAN_FILTER_SIZE);
    
    // Subtract reference value
    raw -= ref_adc_values[idx];
    
    // Convert to millivolts (3.3V reference, 12-bit ADC)
    int32_t millivolts = (raw * 3300) / 4095;

    millivolts *= ADC_atten;
    
    ESP_LOGD(TAG, "ADC sink %d: raw=%d, ref=%d, calib=%d, mV=%d", 
             idx, raw + ref_adc_values[idx], ref_adc_values[idx], raw, millivolts);
    
    return millivolts;
}

void hal_current_calibrate() {
    ESP_LOGD(TAG, "Starting current calibration...");
    
    // Take multiple measurements for each rail and average them
    const int CALIB_SAMPLES = 10;
    int32_t sum_12v = 0, sum_5v = 0, sum_m12v = 0;
    
    for(int i = 0; i < CALIB_SAMPLES; i++) {
        sum_12v += measure_current_raw(PIN_INA_12V);
        sum_5v += measure_current_raw(PIN_INA_5V);
        sum_m12v += measure_current_raw(PIN_INA_M12V);
        delay(100);
    }
    
    ref_current_12v = sum_12v / CALIB_SAMPLES;
    ref_current_5v = sum_5v / CALIB_SAMPLES;
    ref_current_m12v = sum_m12v / CALIB_SAMPLES;
    
    ESP_LOGD(TAG, "Calibration complete. Reference values:");
    ESP_LOGD(TAG, "+12V: %d, +5V: %d, -12V: %d", 
             ref_current_12v, ref_current_5v, ref_current_m12v);
}

// Helper function to measure raw current without calibration
int32_t measure_current_raw(uint8_t pin) {
    // Apply median filter
    int samples[MEDIAN_FILTER_SIZE];
    
    // Take multiple samples
    for(int i = 0; i < MEDIAN_FILTER_SIZE; i++) {
        int raw = analogRead(pin);
        samples[i] = raw;
        delayMicroseconds(100); // Small delay between samples
    }
    
    // Get median value
    int raw = get_median(samples, MEDIAN_FILTER_SIZE);
    
    int32_t voltage = raw * 3300 / 4095;
    int32_t current = (voltage * 1000) / (SHUNT_RESISTOR * INA196_GAIN);
    
    return current;
}

int32_t measure_current(uint8_t pin) {
    int32_t current = measure_current_raw(pin);
    
    // Subtract reference value based on the pin
    switch(pin) {
        case PIN_INA_12V:
            current -= ref_current_12v;
            break;
        case PIN_INA_5V:
            current -= ref_current_5v;
            break;
        case PIN_INA_M12V:
            current -= ref_current_m12v;
            break;
    }
    
    ESP_LOGD(TAG, "Current measurement - Raw: %d uA, Calibrated: %d uA", 
             current + (pin == PIN_INA_12V ? ref_current_12v : 
                       pin == PIN_INA_5V ? ref_current_5v : ref_current_m12v),
             current);
             
    current = max(0, current);
             
    return current;
}

uint8_t hal_adapter_id() {
    uint8_t id = 0;
    
    // Read ID pins in reverse order (ID0 on GPA4, ID4 on GPA0)
    for(int i = 0; i < 5; i++) {
        // Read the pin (GPA0-GPA4) and shift it to the correct position
        // Since pins are in reverse order, we use (4-i) to get the correct bit position
        id |= (mcp1.digitalRead(i) ? 1 : 0) << (4-i);
    }
    
    ESP_LOGD(TAG, "Adapter ID: 0x%02X", id);
    return id;
}

void hal_print_current(void) {
    int32_t current_12v_ua = measure_current(PIN_INA_12V);
    int32_t current_5v_ua = measure_current(PIN_INA_5V);
    int32_t current_m12v_ua = measure_current(PIN_INA_M12V);

    float current_12v_ma = current_12v_ua / 1000.0f;
    float current_5v_ma = current_5v_ua / 1000.0f;
    float current_m12v_ma = current_m12v_ua / 1000.0f;

    ESP_LOGI(TAG, "Current measurements:\n+12V: %.2f mA\n+5V: %.2f mA\n-12V: %.2f mA",
             current_12v_ma, current_5v_ma, current_m12v_ma);
}

void hal_set_source(source_net_t net, float voltage) {
    // Validate voltage range
    if (voltage < -5.0f || voltage > 5.0f) {
        ESP_LOGE(TAG, "Invalid voltage: %.2f V. Must be between -5 and +5 V", voltage);
        return;
    }

    // Validate net
    if (net >= SOURCE_COUNT) {
        ESP_LOGE(TAG, "Invalid source net: %d", net);
        return;
    }

    // Convert voltage to DAC value (0-65535)
    // -5V = 0, 0V = 32768, +5V = 65535
    uint16_t dac_value = (uint16_t)((voltage + 5.0f) * 65535.0f / 10.0f);

    // Set appropriate DAC based on net
    switch (net) {
        case SOURCE_A:
            dac2.setValue(0, dac_value);  // DAC1 channel A
            break;
        case SOURCE_B:
            dac2.setValue(1, dac_value);  // DAC2 channel B
            break;
        case SOURCE_C:
            dac1.setValue(0, dac_value);  // DAC2 channel A
            break;
        case SOURCE_D:
            dac1.setValue(1, dac_value);  // DAC1 channel B
            break;
        default:
            return;  // Should never reach here due to validation above
    }

    ESP_LOGD(TAG, "Set source %d to %.2f V (DAC value: %d)", net, voltage, dac_value);
}

void hal_clear_console(void) {
    // Send ANSI escape sequences to:
    // 1. Clear the screen (\033[2J)
    // 2. Move cursor to home position (\033[H)
    Serial.print("\033[2J\033[H");
    Serial.flush();  // Ensure all data is sent
} 