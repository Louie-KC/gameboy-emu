#pragma once

#include <stdint.h>

#define MEMORY_SIZE 8192

uint8_t memory_work_ram[MEMORY_SIZE];

uint8_t memory_read(uint16_t addr);
uint8_t memory_write(uint16_t addr, uint8_t value);