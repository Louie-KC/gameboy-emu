#pragma once

#include "common.h"

#define MAX_NUM_BANKS 128

#define CARTRIDGE_TITLE_ADDR 0x134
#define CARTRIDGE_TITLE_SIZE 0x10
#define CARTRIDGE_TYPE_ADDR  0x147
#define CARTRIDGE_BANK_ADDR  0x148

typedef struct {
    uint8_t *data;
    uint8_t  bank_count;
    uint8_t  selected_bank;
    uint8_t  cartridge_type;
    uint32_t rom_size;
} cartridge_context;

uint8_t cartridge_load(const char *rom_path);
uint8_t cartridge_read(uint16_t addr);
void cartridge_print_info(void);
void cartridge_cleanup(void);
