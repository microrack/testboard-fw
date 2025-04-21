#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_MCP23X17.h>
#include "board.h"

// Global objects
extern SPIClass SPI_DAC;
extern Adafruit_MCP23X17 mcp0;  // addr 0x20
extern Adafruit_MCP23X17 mcp1;  // addr 0x21

// Current measurement functions
int measureCurrent(int pin);

// DAC control functions
void dac_init();
void writeDAC(int cs_pin, uint8_t channel, uint16_t value);

// MCP initialization
void mcp_init(); 