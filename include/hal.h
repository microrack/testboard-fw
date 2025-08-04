#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP23X17.h>
#include "board.h"
#include <Wire.h>
#include <DAC8552.h>
#include <esp_log.h>

// Global objects
extern SPIClass SPI_DAC;
extern Adafruit_MCP23X17 mcp0;  // addr 0x20
extern Adafruit_MCP23X17 mcp1;  // addr 0x21

// Current measurement functions
int32_t measure_current(uint8_t pin);
int32_t measure_current_raw(uint8_t pin);
void hal_current_calibrate();
int32_t hal_adc_read(ADC_sink_t idx);
void hal_print_current(void);
void hal_clear_console(void);

typedef enum {
    SOURCE_A,
    SOURCE_B,
    SOURCE_C,
    SOURCE_D,
    SOURCE_COUNT
} source_net_t; 

// DAC control functions
void dac_init();
void write_dac(int cs_pin, uint8_t channel, uint16_t value);
void hal_set_source(source_net_t net, int32_t voltage_mv);
void hal_set_source_direct(source_net_t net, uint16_t dac_value);

// Signal generator functions
void hal_start_signal(source_net_t pin, float freq);
void hal_stop_signal(source_net_t pin);

// MCP initialization
void mcp_init();

// Function declarations
void hal_init();
void hal_adc_calibrate();
uint8_t hal_adapter_id();

// IO states
typedef enum {
    IO_LOW = 0,
    IO_HIGH = 1,
    IO_INPUT = 2
} io_state_t;

// IO control functions
void hal_set_io(mcp_io_t io_pin, io_state_t state);

// External objects
extern DAC8552 dac1;
extern DAC8552 dac2;
