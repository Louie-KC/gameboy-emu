#include "bus.h"
#include "cartridge.h"

uint8_t vram[BUS_VRAM_SIZE];
uint8_t wram[BUS_WRAM_SIZE];

uint8_t bus_read(uint16_t addr) {
    switch (addr) {
        case 0x0000 ... BUS_VRAM_ADDR - 1:
            return cartridge_read(addr);
        case BUS_VRAM_ADDR ... BUS_EXT_RAM_ADDR - 1:
            // Video RAM
            return vram[addr - BUS_VRAM_ADDR];
        case BUS_EXT_RAM_ADDR ... BUS_WRAM_ADDR - 1:
            // TODO: External RAM
            return 0;
        case BUS_WRAM_ADDR ... BUS_ECHO_ADDR - 1:
            // Working RAM
            return wram[addr - BUS_WRAM_ADDR];
        default:
            // TODO: Echo, OAM, IO registers, HRAM, IE
            return 0;
    }
}

void bus_write(uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... BUS_VRAM_ADDR - 1:
            break;  // ROM, READ-ONLY
        case BUS_VRAM_ADDR ... BUS_EXT_RAM_ADDR - 1:
            // Video RAM
            vram[addr - BUS_VRAM_ADDR] = value;
            break;
        case BUS_EXT_RAM_ADDR ... BUS_WRAM_ADDR - 1:
            // TODO: External RAM
            break;
        case BUS_WRAM_ADDR ... BUS_ECHO_ADDR - 1:
            // Working RAM
            wram[addr - BUS_WRAM_ADDR] = value;
            break;
        default:
            // TODO: Echo, OAM, IO registers, HRAM, IE
            break;
    }
}

uint16_t bus_read_16(uint16_t addr) {
    uint16_t low  = bus_read(addr);
    uint16_t high = bus_read(addr + 1);
    return (high << 8) | low;
}

void bus_write_16(uint16_t addr, uint16_t value) {
    uint8_t low  = value & 0xFF;
    uint8_t high = (value >> 8) & 0xFF;
    bus_write(addr, low);
    bus_write(addr + 1, high);
}