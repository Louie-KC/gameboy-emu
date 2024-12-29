#include "emulator.h"
#include "bus.h"
#include "cartridge.h"
#include "cpu.h"
#include "common.h"
#include "ppu.h"
#include "window.h"

#define USAGE "rom_path"

#define MIN_ARGC 2

int emulator_run(int argc, char *argv[]) {

    if (argc < MIN_ARGC) {
        printf("Incorrect args\n");
        printf("Usage: %s %s\n", argv[0], USAGE);
        return 1;
    }

    if (!cartridge_rom_load(argv[1])) {
        printf("Failed to load ROM\nExiting\n");
        return 1;
    }

    if (!window_init()) {
        return 1;
    }

    cpu_init();
    ppu_init();
    emu_run = 1;

    while (emu_run) {        
        cpu_step();
        ppu_step();
        window_step();
        
        if (ppu_view_updated) {
            window_draw();
            ppu_view_updated = 0;
        }
    }

    window_exit();
    return 0;
}