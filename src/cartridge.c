#include "cartridge.h"

static cartridge_context ctx;

uint8_t cartridge_load(const char *rom_path) {
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
    ctx.data = (uint8_t *) malloc(ctx.rom_size);
    if (!ctx.data) {
        printf("[ERROR] cartridge_load: malloc fail\n");
        return 0;
    }

    // Read file content to cartridge
    if (!fread(ctx.data, ctx.rom_size, 1, f)) {
        printf("[ERROR] cartridge_load: fread fail\n");
        return 0;
    }

    // Finished with the file
    fclose(f);

    // Some header data
    ctx.cartridge_type = ctx.data[CARTRIDGE_TYPE_ADDR];
    ctx.bank_count = 2 << ctx.data[CARTRIDGE_BANK_ADDR];

    ctx.selected_bank = 0;

    return 1;
}

uint8_t cartridge_read(uint16_t addr) {
    // TODO: bank switching
    return ctx.data[addr];
}

void cartridge_print_info(void) {
    char title[CARTRIDGE_TITLE_SIZE + 1];
    for (int i = 0; i < CARTRIDGE_TITLE_SIZE; i++) {
        title[i] = ctx.data[CARTRIDGE_TITLE_ADDR + i];
    }
    title[CARTRIDGE_TITLE_SIZE] = 0;

    printf("* cartridge_print_info\n");
    printf("title: '%s'\n", title);
    printf("type : %02x\n", ctx.cartridge_type);
    printf("banks: %d\n", ctx.bank_count);
    printf("size : %d\n", ctx.rom_size);
}

void cartridge_cleanup(void) {
    if (ctx.data) {
        free(ctx.data);
    }
}