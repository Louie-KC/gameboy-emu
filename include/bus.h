#pragma once

#include <stdint.h>

// Memory map
#define BUS_ROM_BANK_0_ADDR 0x0000
#define BUS_ROM_BANK_N_ADDR 0x4000
#define BUS_VRAM_ADDR       0x8000
#define BUS_EXT_RAM_ADDR    0xA000
#define BUS_WRAM_ADDR       0xC000
#define BUS_ECHO_ADDR       0xE000

// Memory sizes
#define BUS_ROM_BANK_SIZE BUS_ROM_BANK_N_ADDR
#define BUS_VRAM_SIZE     BUS_WRAM_ADDR - BUS_VRAM_ADDR
#define BUS_WRAM_SIZE     BUS_ECHO_ADDR - BUS_WRAM_ADDR

uint8_t bus_read(uint16_t addr);
void bus_write(uint16_t addr, uint8_t value);

uint16_t bus_read_16(uint16_t addr);
void bus_write_16(uint16_t addr, uint16_t value);