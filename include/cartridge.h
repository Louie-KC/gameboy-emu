#pragma once

#include "common.h"

#define MAX_NUM_BANKS 128

#define CARTRIDGE_TITLE_ADDR 0x134
#define CARTRIDGE_TITLE_SIZE 0x10
#define CARTRIDGE_TYPE_ADDR  0x147
#define CARTRIDGE_BANK_ADDR  0x148

#define CARTRIDGE_RAM_BANK_SIZE 0x2000

typedef enum {
    ROM_ONLY,
    MBC1,
} cart_type;

typedef struct {
    uint8_t  *rom;
    uint8_t   rom_bank_count;
    // uint8_t   selected_bank;
    uint8_t   rom_bank;
    cart_type cartridge_type;
    uint32_t  rom_size;
    uint8_t  *ram;
    uint8_t   ram_enable;
    uint8_t   ram_bank;
    uint8_t   mode;
} cartridge_context;

uint8_t cartridge_rom_load(const char *rom_path);
uint8_t cartridge_rom_read(uint16_t addr);
void cartridge_bank_operation(uint16_t addr, uint8_t value);
uint8_t cartridge_ram_read(uint16_t addr);
void cartridge_ram_write(uint16_t addr, uint8_t value);
void cartridge_print_info(void);
void cartridge_cleanup(void);
