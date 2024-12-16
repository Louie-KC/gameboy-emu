#include "emulator.h"
#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "common.h"
#include "window.h"

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
        return 1;
    }

    if (!window_init()) {
        return 1;
    }

    cpu_init();
    emu_run = 1;

    while (emu_run) {
        cpu_step();
        window_step();
    }

    window_exit();
    return 0;
}