#include "common.h"
#include "emulator.h"

#define USAGE "rom_path"

#define MIN_ARGC 2

int main(int argc, char *argv[]) {
    return emulator_run(argc, argv);
}