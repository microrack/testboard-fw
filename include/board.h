#pragma once

// Pin definitions
const int PIN_SCL = 22;
const int PIN_SDA = 21;

const int PIN_IO0_RST = 23;
const int PIN_IO1_RST = 19;

const int PIN_MOSI = 18;
const int PIN_SCK = 5;
const int PIN_CS1 = 17;  // TX2 → DAC8552 #1
const int PIN_CS2 = 16;  // RX2 → DAC8552 #2

// INA196A current monitor pins
const int PIN_INA_12V = 4;   // D4 → +12V rail current
const int PIN_INA_5V = 2;    // D2 → +5V rail current
const int PIN_INA_M12V = 15; // D15 → -12V rail current

// INA196A configuration
const float SHUNT_RESISTOR = 1.0f;  // 1 Ohm shunt resistor
const float INA196_GAIN = 20.0f;   // INA196A gain (V/V)

// Display configuration
const int SCREEN_WIDTH = 128; // OLED display width, in pixels
const int SCREEN_HEIGHT = 64; // OLED display height, in pixels
const int OLED_RESET = -1;    // Reset pin # (or -1 if sharing Arduino reset pin)
const int SCREEN_ADDRESS = 0x3C; ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// MCP23X17 configuration
const int MCP_ADDR_0 = 0x20;
const int MCP_ADDR_1 = 0x21;

// MCP23X17 pin definitions
const int GPB = 8;
const int PIN_LED_OK = GPB + 3;
const int PIN_LED_FAIL = GPB + 4;

const int PIN_P12V_PASS = GPB + 0;
const int PIN_P5V_PASS = GPB + 1;
const int PIN_M12V_PASS = GPB + 2;

// PD sink pins on MCP1
const int PIN_SINK_PD_A = GPB + 5;
const int PIN_SINK_PD_B = GPB + 6;
const int PIN_SINK_PD_C = GPB + 7;

typedef enum {
    IO0 = 0,
    IO1 = 1,
    IO2 = 2,
    IO3 = 3,
    IO4 = 4,
    IO5 = 5,
    IO6 = 6,
    IO7 = 7,
    IO8 = 8,
    IO9 = 9,
    IO10 = 10,
    IO11 = 11,
    IO12 = 12,
    IO13 = 13,
    IO14 = 14,
    IO15 = 15
} mcp_io_t;

typedef enum {
    ADC_sink_1k_A,
    ADC_sink_1k_B,
    ADC_sink_1k_C,
    ADC_sink_1k_D,
    ADC_sink_1k_E,
    ADC_sink_1k_F,
    ADC_sink_PD_A,
    ADC_sink_PD_B,
    ADC_sink_PD_C,
    ADC_sink_Z_D,
    ADC_sink_Z_E,
    ADC_sink_Z_F,
    ADC_sink_count
} ADC_sink_t;

const int ADC_PINS[ADC_sink_count] = {36, 39, 34, 35, 32, 33, 25, 26, 27, 14, 12, 13};

const int ADC_atten = 3;

/**
 * @brief Structure defining a voltage source test configuration
 */
typedef struct {
    ADC_sink_t adc_pin;    // ADC pin to read voltage from
    mcp_io_t pd_pin;       // Pull-down control pin
} voltage_source_t;

// Voltage source configurations
const voltage_source_t PD_A = {
    .adc_pin = ADC_sink_PD_A,
    .pd_pin = (mcp_io_t)PIN_SINK_PD_A
};

const voltage_source_t PD_B = {
    .adc_pin = ADC_sink_PD_B,
    .pd_pin = (mcp_io_t)PIN_SINK_PD_B
};

const voltage_source_t PD_C = {
    .adc_pin = ADC_sink_PD_C,
    .pd_pin = (mcp_io_t)PIN_SINK_PD_C
};
