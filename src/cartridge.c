#include "cartridge.h"

static cartridge_context ctx;

uint8_t cartridge_rom_load(const char *rom_path) {
    FILE *f = fopen(rom_path, "rb");
    if (!f) {
        printf("[ERROR] cartridge_load: Could not open '%s'\n", rom_path);
        return 0;
    }
    
    // Determine file size
    fseek(f, 0, SEEK_END);
    ctx.rom_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate necessary memory
    ctx.rom = (uint8_t *) malloc(ctx.rom_size);
    if (!ctx.rom) {
        printf("[ERROR] cartridge_load: malloc fail\n");
        return 0;
    }

    // Read file content to cartridge
    if (!fread(ctx.rom, ctx.rom_size, 1, f)) {
        printf("[ERROR] cartridge_load: fread fail\n");
        return 0;
    }

    // Finished with the file
    fclose(f);

    // Some header data
    switch (ctx.rom[CARTRIDGE_TYPE_ADDR]) {
        case 0x00:
            ctx.cartridge_type = ROM_ONLY;
            ctx.ram = malloc(CARTRIDGE_RAM_BANK_SIZE);
            break;
        case 0x03:  // MBC1+RAM+BATTERY

            // fall through
        case 0x02:  // MBC1+RAM
            ctx.ram = malloc(CARTRIDGE_RAM_BANK_SIZE * 4);
            // fall through
        case 0x01:  // MBC1
            ctx.cartridge_type = MBC1;
            break;
        default:
            printf("[ERROR] unsupported cartridge type: %02X\n", ctx.rom[CARTRIDGE_TYPE_ADDR]);
            exit(1);
    }
    ctx.rom_bank_count = 2 << ctx.rom[CARTRIDGE_BANK_ADDR];
    ctx.rom_bank = 0;

    ctx.ram_enable = 0;
    ctx.ram_bank = 0;

    return 1;
}

uint8_t cartridge_rom_read(uint16_t addr) {
    switch (addr) {
        case 0x0000 ... 0x3FFF:
            // ROM bank 0
            return ctx.rom[addr];
        case 0x4000 ... 0x7FFF:
            // ROM bank 1-N
            return ctx.rom[addr + ctx.rom_bank * 0x4000];
        default:
            printf("[ERROR] cartridge_read bad address %04X\n", addr);
            exit(1);
            return 0;
    }
}

void cartridge_bank_operation(uint16_t addr, uint8_t value) {
    if (ctx.cartridge_type == ROM_ONLY) { return; }
    // assume MBC1
    switch (addr) {
        case 0x0000 ... 0x1FFF:  // RAM enable/disable
            ctx.ram_enable = (value & 0x0F) == 0x0A;
            break;
        
        case 0x2000 ... 0x3FFF:  // ROM bank select
            value &= 0x1F;
            // Treat bank select of 0 as 1
            value += value == 0;
            ctx.rom_bank = (ctx.rom_bank & 0x60) | value;
            break;
        
        case 0x4000 ... 0x5FFF:  // RAM bank select
            // value &= 0x03;
            if (ctx.mode) {
                ctx.ram_bank = value & 0x03;
            } else {
                ctx.rom_bank = (ctx.rom_bank & 0x1F) | (value & 0x03) << 5;
            }
            break;
        case 0x6000 ... 0x7FFF:  // ROM/RAM mode select
            ctx.mode = value == 1;
            break;

    }
}

void cartridge_print_info(void) {
    char title[CARTRIDGE_TITLE_SIZE + 1];
    for (int i = 0; i < CARTRIDGE_TITLE_SIZE; i++) {
        title[i] = ctx.rom[CARTRIDGE_TITLE_ADDR + i];
    }
    title[CARTRIDGE_TITLE_SIZE] = 0;

    printf("* cartridge_print_info\n");
    printf("title: '%s'\n", title);
    printf("type : %02x\n", ctx.cartridge_type);
    printf("banks: %d\n", ctx.rom_bank_count);
    printf("size : %d\n", ctx.rom_size);
}

uint8_t cartridge_ram_read(uint16_t addr) {
    switch (ctx.cartridge_type) {
        case ROM_ONLY:
            return ctx.ram[addr];
        case MBC1:
            if (ctx.ram) {
                return ctx.ram[addr + ctx.ram_bank * CARTRIDGE_RAM_BANK_SIZE];
            } else {
                printf("RAM not enabled\n");
                // fall through
            }
        default:
            printf("[WARN] cartridge_ram_read: Unsupported cartridge type\n");
            exit(1);
            return 0;
    }
}

void cartridge_ram_write(uint16_t addr, uint8_t value) {
    switch (ctx.cartridge_type) {
        case ROM_ONLY:
            ctx.ram[addr] = value;
            break;
        default:
            printf("[WARN] cartridge_ram_write: Unsupported cartridge type\n");
            exit(1);
    }
}

void cartridge_cleanup(void) {
    if (ctx.rom) {
        free(ctx.rom);
    }
    if (ctx.ram) {
        free(ctx.ram);
    }
}