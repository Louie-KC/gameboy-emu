#include "bus.h"
#include "cartridge.h"

uint8_t vram[BUS_VRAM_SIZE];
uint8_t wram[BUS_WRAM_SIZE];
uint8_t  oam[BUS_OAM_SIZE];
uint8_t mmio[BUS_IO_REG_SIZE];
uint8_t hram[BUS_HRAM_SIZE];
uint8_t enable_interrupt;

uint8_t bus_read(uint16_t addr) {
    switch (addr) {
        case 0x0000 ... BUS_VRAM_ADDR - 1:
            return cartridge_rom_read(addr);
        case BUS_VRAM_ADDR ... BUS_EXT_RAM_ADDR - 1:
            // Video RAM
            return vram[addr - BUS_VRAM_ADDR];
        case BUS_EXT_RAM_ADDR ... BUS_WRAM_ADDR - 1:
            // External RAM
            return cartridge_ram_read(addr - BUS_EXT_RAM_ADDR);
        case BUS_WRAM_ADDR ... BUS_ECHO_ADDR - 1:
            // Working RAM
            return wram[addr - BUS_WRAM_ADDR];
        case BUS_ECHO_ADDR ... BUS_OAM_ADDR - 1:
            // Echo (of workign RAM) RAM
            return wram[addr - BUS_ECHO_ADDR];
        case BUS_OAM_ADDR ... BUS_UNUSABLE_ADDR - 1:
            // Object Attribute Memory
            return oam[addr - BUS_OAM_ADDR];
        case BUS_UNUSABLE_ADDR ... BUS_IO_REG_ADDR - 1:
            // Not usable memory range
            return 0;
        case BUS_IO_REG_ADDR ... BUS_HRAM_ADDR - 1:
            // Input-Output Registers
            return mmio[addr - BUS_IO_REG_ADDR];
        case BUS_HRAM_ADDR ... BUS_IE_REG_ADDR - 1:
            // High RAM
            return hram[addr - BUS_HRAM_ADDR];
        case BUS_IE_REG_ADDR:
            return enable_interrupt;
        default:
            printf("[WARN] bus_read default path taken\n");
            return 0;
    }
}

void bus_write(uint16_t addr, uint8_t value) {
    switch (addr) {
        case 0x0000 ... BUS_VRAM_ADDR - 1:
            // Cartridge ROM
            cartridge_bank_operation(addr, value);
            break;
        case BUS_VRAM_ADDR ... BUS_EXT_RAM_ADDR - 1:
            // Video RAM
            vram[addr - BUS_VRAM_ADDR] = value;
            break;
        case BUS_EXT_RAM_ADDR ... BUS_WRAM_ADDR - 1:
            // External RAM
            cartridge_ram_write(addr, value);
            break;
        case BUS_WRAM_ADDR ... BUS_ECHO_ADDR - 1:
            // Working RAM
            wram[addr - BUS_WRAM_ADDR] = value;
            break;
        case BUS_ECHO_ADDR ... BUS_OAM_ADDR - 1:
            // Echo (of WRAM) RAM
            wram[addr - BUS_ECHO_ADDR] = value;
            break;
        case BUS_OAM_ADDR ... BUS_UNUSABLE_ADDR - 1:
            // Object Attribute Memory
            oam[addr - BUS_OAM_ADDR] = value;
            break;
        case BUS_UNUSABLE_ADDR ... BUS_IO_REG_ADDR - 1:
            // Not usable memory range
            break;
        case BUS_IO_REG_ADDR ... BUS_HRAM_ADDR - 1:
            // Input-Output Registers
            mmio[addr - BUS_IO_REG_ADDR] = value;
            break;
        case BUS_HRAM_ADDR ... BUS_IE_REG_ADDR - 1:
            // High RAM
            hram[addr - BUS_HRAM_ADDR] = value;
            break;
        case BUS_IE_REG_ADDR:
            enable_interrupt = value;
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