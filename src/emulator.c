#include "emulator.h"
#include "bus.h"
#include "cartridge.h"
#include "common.h"

#define USAGE "rom_path"

#define MIN_ARGC 2

int emulator_run(int argc, char *argv[]) {
    if (argc < MIN_ARGC) {
        printf("Incorrect args\n");
        printf("Usage: %s %s\n", argv[0], USAGE);
        return 1;
    }

    if (!cartridge_load(argv[1])) {
        printf("Failed to load ROM\nExiting\n");
        return 2;
    }
    cartridge_print_info();

    return 0;
}