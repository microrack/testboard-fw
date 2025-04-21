#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "board.h"

// Global objects
extern SPIClass SPI_DAC;

// Current measurement functions
int measureCurrent(int pin);

// DAC control functions
void dac_init();
void writeDAC(int cs_pin, uint8_t channel, uint16_t value); 