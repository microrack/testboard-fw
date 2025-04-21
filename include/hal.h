#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP23X17.h>
#include "board.h"
#include <Wire.h>
#include <DAC8552.h>
#include <esp_log.h>
#include <driver/adc.h>

// Global objects
extern SPIClass SPI_DAC;
extern Adafruit_MCP23X17 mcp0;  // addr 0x20
extern Adafruit_MCP23X17 mcp1;  // addr 0x21

// Current measurement functions
int32_t measure_current(uint8_t pin);
int32_t measure_current_raw(uint8_t pin);
void hal_current_calibrate();
int32_t hal_adc_read(ADC_sink_t idx);

// DAC control functions
void dac_init();
void write_dac(int cs_pin, uint8_t channel, uint16_t value);

// MCP initialization
void mcp_init();

// Function declarations
void hal_init();
void hal_adc_calibrate();

// External objects
extern DAC8552 dac1;
extern DAC8552 dac2; 