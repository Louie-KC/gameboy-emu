#include "cpu.h"
#include "bus.h"

// Pull out 3 bits of an op code following the format: XXYYYZZZ
#define YYY(op) ((op >> 3) & 0x07)
#define ZZZ(op) (op & 0x07)

// NOTE: the below must not be used for registers A and F as they are
//       stored separately from the `registers` array
#define REG_YYY(op) ctx.registers[YYY(op)]
#define REG_ZZZ(op) ctx.registers[ZZZ(op)]

// Name registers uniformly across array and individual flags
#define REG_B ctx.registers[0]
#define REG_C ctx.registers[1]
#define REG_D ctx.registers[2]
#define REG_E ctx.registers[3]
#define REG_H ctx.registers[4]
#define REG_L ctx.registers[5]
#define REG_A ctx.register_a
#define REG_F ctx.register_f

// Create uniform names for each of the flags
#define REG_F_Z ctx.register_f.zero
#define REG_F_N ctx.register_f.subtraction
#define REG_F_H ctx.register_f.half_carry
#define REG_F_C ctx.register_f.carry

// NOTE: `bit` should not be an operation. Only use with immediates or variables
#define REG_F_SET_Z(bit) ctx.register_f.zero        = bit
#define REG_F_SET_N(bit) ctx.register_f.subtraction = bit
#define REG_F_SET_H(bit) ctx.register_f.half_carry  = bit
#define REG_F_SET_C(bit) ctx.register_f.carry       = bit

#define REG_BC ((REG_B << 8) | REG_C)
#define REG_DE ((REG_D << 8) | REG_E)
#define REG_HL ((REG_H << 8) | REG_L)
#define REG_AF ((REG_A << 8) | REG_F)

// NOTE: `value` should not be an operation. Only use with immediates or variables
#define REG_BC_SET(value) REG_B = ((value) >> 8) & 0xFF; REG_C = value & 0xFF
#define REG_DE_SET(value) REG_D = ((value) >> 8) & 0xFF; REG_E = value & 0xFF
#define REG_HL_SET(value) REG_H = ((value) >> 8) & 0xFF; REG_L = value & 0xFF
#define REG_AF_SET(value) REG_A = ((value) >> 8) & 0xFF; REG_F = value & 0xFF

static cpu_context ctx;

void print_state(void) {
    uint8_t reg_f = 0;
    reg_f |= ctx.register_f.zero << 7
        | ctx.register_f.subtraction << 6
        | ctx.register_f.half_carry << 5
        | ctx.register_f.carry << 4;
    printf("A: %02X F: %02X B: %02X C: %02X D: %02X E: %02X H: %02X L: %02X SP: %04X PC: 00:%04X (%02X %02X %02X %02X)\n",
        REG_A, reg_f, REG_B, REG_C, REG_D, REG_E, REG_H, REG_L, ctx.sp, ctx.pc,
        bus_read(ctx.pc), bus_read(ctx.pc + 1), bus_read(ctx.pc + 2), bus_read(ctx.pc + 3));
}

uint8_t fetch(void) {
    uint8_t op = bus_read(ctx.pc);
    ctx.pc += 1;
    return op;
}

uint16_t fetch_16(void) {
    uint8_t low  = fetch();
    uint8_t high = fetch();
    return (high << 8) | low;
}

void execute(uint8_t op) {
    uint16_t intermediate;

    switch (op) {
        // NOP
        case 0x00:
            ctx.t_cycles = 4;
            break;

        // LD BC, u16
        case 0x01:
            ctx.t_cycles = 12;
            intermediate = fetch_16();
            REG_BC_SET(intermediate);
            break;

        // LD (BC), A
        case 0x02:
            ctx.t_cycles = 8;
            bus_write(REG_BC, REG_A);
            break;

        // INC BC
        case 0x03:
            ctx.t_cycles = 8;
            REG_BC_SET(REG_BC + 1);
            break;

        // RLCA - Rotate Left Carry register A
        case 0x07:
            ctx.t_cycles = 4;
            REG_F_SET_C(REG_A >> 7);
            intermediate = (REG_A << 1) | REG_F_C;
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(intermediate == 0);
            break;

        // LD (u16), SP
        case 0x08:
            ctx.t_cycles = 20;
            bus_write_16(fetch_16(), ctx.sp);
            break;

        // ADD HL, BC
        case 0x09:
            ctx.t_cycles = 8;
            intermediate = REG_HL + REG_BC;
            REG_HL_SET(intermediate);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x07FF);
            REG_F_SET_C(((uint32_t) REG_HL) + ((uint32_t) REG_BC) > 0x7FFF);
            break;

        // LD A, (BC) - Load into A the value at addr BC
        case 0x0A:
            ctx.t_cycles = 8;
            REG_A = bus_read(REG_BC);
            break;

        // DEC BC
        case 0x0B:
            ctx.t_cycles = 8;
            REG_BC_SET(REG_BC - 1);
            break;
   
        // INC r8 - minus A
        case 0x04: case 0x0C:
        case 0x14: case 0x1C:
        case 0x24: case 0x2C:
            ctx.t_cycles = 4;
            intermediate = REG_YYY(op) + 1;
            REG_YYY(op) = intermediate & 0xFF;
            REG_F_SET_Z(REG_YYY(op) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(REG_YYY(op) > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            break;
        
        // DEC r8 - minus (HL) and A
        case 0x05: case 0x0D:
        case 0x15: case 0x1D:
        case 0x25: case 0x2D:
            ctx.t_cycles = 4;
            intermediate = REG_YYY(op) - 1;
            REG_YYY(op) = intermediate & 0xFF;
            REG_F_SET_Z(REG_YYY(op) == 0);
            REG_F_SET_N(1);
            REG_F_SET_H(REG_YYY(op) > 0x0F);
            break;

        // LD r8, u8 - Load immediate to register (8) minus A
        case 0x06: case 0x0E:
        case 0x16: case 0x1E:
        case 0x26:
            ctx.t_cycles = 8;
            REG_YYY(op) = fetch();
            break;

        // RRCA
        case 0x0F:
            ctx.t_cycles = 4;
            REG_F_SET_C(REG_A & 0x01);
            intermediate = (REG_F_C << 7) | (REG_A >> 1);
            REG_A = intermediate;
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            break;

        // STOP
        case 0x10:
            ctx.t_cycles = 4;
            printf("STOP\n");
            break;
        
        // Load immediate 16 value into DE
        case 0x11:
            ctx.t_cycles = 12;
            intermediate = fetch_16();
            REG_DE_SET(intermediate);
            break;

        // LD (DE), A
        case 0x12:
            ctx.t_cycles = 8;
            bus_write(REG_DE, REG_A);
            break;

        // INC DE
        case 0x13:
            ctx.t_cycles = 8;
            intermediate = REG_DE + 1;
            REG_DE_SET(intermediate);
            break;

        // RLA - Rotate Left register A
        case 0x17:
            ctx.t_cycles = 4;
            intermediate = REG_A >> 7;
            REG_A = (REG_A << 1) | REG_F_C;
            REG_F_SET_C(intermediate);
            REG_F_SET_Z(0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            break;

        // Jump relative unconditional
        case 0x18:
            ctx.t_cycles = 12;
            intermediate = fetch();
            ctx.pc += (int8_t) intermediate;
            break;

        // ADD HL, DE
        case 0x19:
            ctx.t_cycles = 8;
            intermediate = REG_HL + REG_DE;
            REG_HL_SET(intermediate);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x07FF);
            REG_F_SET_C(((uint32_t) REG_HL) + ((uint32_t) REG_DE) > 0x7FFF);
            break;

        // LD A, (DE) - Load into A the value at addr DE
        case 0x1A:
            ctx.t_cycles = 8;
            REG_A = bus_read(REG_DE);
            break;

        // DEC DE
        case 0x1B:
            ctx.t_cycles = 8;
            intermediate = REG_DE - 1;
            REG_DE_SET(intermediate);
            break;

        // RRA
        case 0x1F:
            ctx.t_cycles = 4;
            intermediate = REG_A & 0x01;
            REG_A = (REG_F_C << 7) | (REG_A >> 1);
            REG_F_SET_Z(0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(intermediate);
            break;

        // Jump relative not zero (JR NZ)
        case 0x20:
            intermediate = fetch();
            if (!REG_F_Z) {
                ctx.t_cycles = 12;
                ctx.pc += (int8_t) intermediate;
            } else {
                ctx.t_cycles = 8;
            }
            break;

        // Load immediate 16 into HL
        case 0x21:
            ctx.t_cycles = 12;
            intermediate = fetch_16();
            REG_HL_SET(intermediate);
            break;

        // LD (HL+), A - Load register A to HL addr (then increment HL)
        case 0x22:
            ctx.t_cycles = 8;
            bus_write(REG_HL, REG_A);
            intermediate = REG_HL + 1;
            REG_HL_SET(intermediate);
            break;

        // INC HL
        case 0x23:
            ctx.t_cycles = 8;
            intermediate = REG_HL + 1;
            REG_HL_SET(intermediate);
            break;

        // TODO: 0x27 - DAA

        // JR Z, i8
        case 0x28:
            intermediate = fetch();
            if (REG_F_Z) {
                ctx.t_cycles = 12;
                ctx.pc += (int8_t) intermediate;
            } else {
                // do nothing
                ctx.t_cycles = 8;
            }
            break;

        // ADD HL, HL
        case 0x29:
            ctx.t_cycles = 8;
            intermediate = REG_HL + REG_HL;
            REG_HL_SET(intermediate);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x07FF);
            REG_F_SET_C(((uint32_t) REG_HL) + ((uint32_t) REG_HL) > 0x7FFF);
            break;

        // LD A, (HL+) - Load value at HL addr (and post increment HL) to register A
        case 0x2A:
            ctx.t_cycles = 8;
            REG_A = bus_read_16(REG_HL);
            intermediate = REG_HL + 1;
            REG_HL_SET(intermediate);
            break;
        
        // Decrement HL
        case 0x2B:
            ctx.t_cycles = 8;
            intermediate = REG_HL - 1;
            REG_HL_SET(intermediate);
            break;

        // LD A, u8
        case 0x2E:
            ctx.t_cycles = 8;
            REG_A = fetch();
            break;

        // Complement register A (CPL)
        case 0x2F:
            ctx.t_cycles = 4;
            REG_A = REG_A ^ 0xFF;
            REG_F_SET_N(1);
            REG_F_SET_H(1);
            break;

        // Jump relative if not carry
        case 0x30:
            intermediate = fetch();
            if (!REG_F_C) {
                ctx.t_cycles = 12;
                ctx.pc += (int8_t) intermediate;
            } else {
                // do nothing
                ctx.t_cycles = 8;
            }
            break;

        // LD SP, u16
        case 0x31:
            ctx.t_cycles = 12;
            ctx.sp = fetch_16();
            break;

        // LD (HL-), A
        case 0x32:
            ctx.t_cycles = 8;
            bus_write(REG_HL, REG_A);
            intermediate = REG_HL - 1;
            REG_HL_SET(intermediate);
            break;

        // Increment SP
        case 0x33:
            ctx.t_cycles = 8;
            ctx.sp++;
            break;
        
        // Increment value at addr HL
        case 0x34:
            ctx.t_cycles = 12;
            intermediate = bus_read(REG_HL);
            bus_write(REG_HL, intermediate + 1);
            break;
        
        // Decrement value at addr HL
        case 0x35:
            ctx.t_cycles = 12;
            intermediate = bus_read(REG_HL);
            bus_write(REG_HL, intermediate - 1);
            break;

        // Load immediate to HL addr
        case 0x36:
            ctx.t_cycles = 12;
            bus_write(REG_HL, fetch());
            break;

        // SCF - Set carry flag
        case 0x37:
            ctx.t_cycles = 4;
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(1);
            break;

        // JR C, i8 - Jump relative if carry
        case 0x38:
            intermediate = fetch();
            if (REG_F_C) {
                ctx.t_cycles = 12;
                ctx.pc += (int8_t) intermediate;
            } else {
                ctx.t_cycles = 8;
            }
            break;
        
        // ADD HL, SP
        case 0x39:
            ctx.t_cycles = 8;
            intermediate = REG_HL + ctx.sp;
            REG_HL_SET(intermediate);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x07FF);
            REG_F_SET_C(((uint32_t) REG_HL) + ((uint32_t) ctx.sp) > 0x7FFF);
            break;

        // Load value at HR addr (and decrement HL) to register A
        case 0x3A:
            ctx.t_cycles = 8;
            REG_A = bus_read_16(REG_HL);
            intermediate = REG_HL - 1;
            REG_HL_SET(intermediate);
            break;

        // Decrement SP
        case 0x3B:
            ctx.t_cycles = 8;
            ctx.sp--;
            break;
        
        // Increment A
        case 0x3C:
            ctx.t_cycles = 4;
            REG_A++;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(REG_A > 0x0F);
            break;
        
        // Decrement A
        case 0x3D:
            ctx.t_cycles = 4;
            REG_A--;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(1);
            REG_F_SET_H(REG_A > 0x0F);
            break;

        // Load immediate 8 to register A
        case 0x3E:
            ctx.t_cycles = 8;
            REG_A = fetch(); 
            break;
        
        // CCF - Complement Carry Flag
        case 0x3F:
            ctx.t_cycles = 4;
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(REG_F_C ^ 0x01);
            break;

        // LD r8, r8 (minus (HL) and A) - Load register to register
        case 0x40 ... 0x45:
        case 0x48 ... 0x4D:
        case 0x50 ... 0x55:
        case 0x58 ... 0x5D:
        case 0x60 ... 0x65:
        case 0x68 ... 0x6D:
            ctx.t_cycles = 4;
            REG_YYY(op) = REG_ZZZ(op);
            break;

        // LD r8, (HL) - Load HL addr value to register
        case 0x46: case 0x4E:
        case 0x56: case 0x5E:
        case 0x66: case 0x6E:
                   case 0x7E:
            ctx.t_cycles = 8;
            REG_YYY(op) = bus_read(REG_HL);
            break;

        // LD r8, A
        case 0x47: case 0x4F:
        case 0x57: case 0x5F:
        case 0x67: case 0x6F:
            ctx.t_cycles = 4;
            REG_YYY(op) = REG_A;
            break;

        // LD (HL), r8 - Minus register A
        case 0x70 ... 0x75:
            ctx.t_cycles = 8;
            bus_write(REG_HL, REG_ZZZ(op));
            break;

        // HALT - prevent PC increment
        case 0x76:
            ctx.t_cycles = 4;
            ctx.pc--;
            break;
        
        // LD (HL), A
        case 0x77:
            ctx.t_cycles = 8;
            bus_write(REG_HL, REG_A);
            break;

        // LD A, r8 - minus A, A
        case 0x78 ... 0x7D:
            ctx.t_cycles = 4;
            REG_A = REG_ZZZ(op);
            break;

        // LD A, A - Essentially a NOP
        case 0x7F:
            ctx.t_cycles = 4;
            // do nothing
            break;

        // ADD A, r8 - Minus A
        case 0x80 ... 0x85:
            ctx.t_cycles = 4;
            intermediate = REG_A + REG_ZZZ(op);
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            REG_A = intermediate & 0xFF;
            break;

        // ADD A, (HL)
        case 0x86:
            ctx.t_cycles = 8;
            intermediate = REG_A + bus_read(REG_HL);
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            REG_A = intermediate & 0xFF;
            break;

        // ADD A, A
        case 0x87:
            ctx.t_cycles = 4;
            intermediate = REG_A + REG_A;
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            REG_A = intermediate & 0xFF;
            break;

        // ADC A, r8 - minus A
        case 0x88 ... 0x8D:
            ctx.t_cycles = 4;
            intermediate = REG_A + REG_F_C + REG_ZZZ(op);
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            REG_A = intermediate & 0xFF;
            break;

        // ADC A, (HL)
        case 0x8E:
            ctx.t_cycles = 8;
            intermediate = REG_A + REG_F_C + bus_read(REG_HL);
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            REG_A = intermediate & 0xFF;
            break;

        case 0x8F:
            ctx.t_cycles = 4;
            intermediate = REG_A + REG_A + REG_F_C;
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            REG_A = intermediate & 0xFF;
            break;

        // SUB A, r8 - Subtract register from register A
        case 0x90 ... 0x95:
            ctx.t_cycles = 4;
            intermediate = REG_ZZZ(op);
            REG_F_SET_C(intermediate > REG_A);
            REG_F_SET_H(intermediate > 0x0F);
            intermediate -= REG_A;
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(1);
            break;

        // SUB A, (HL)
        case 0x96:
            ctx.t_cycles = 8;
            intermediate = bus_read(REG_HL);
            REG_F_SET_C(intermediate > REG_A);
            REG_F_SET_H(intermediate > 0x0F);
            intermediate -= REG_A;
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(1);
            break;

        // SUB A, A
        case 0x97:
            ctx.t_cycles = 4;
            REG_A = 0;
            REG_F_SET_Z(0);
            REG_F_SET_N(1);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // SBC A, r8 - Subtract carry
        case 0x98 ... 0x9D:
            ctx.t_cycles = 4;
            intermediate = REG_A - REG_F_C - REG_ZZZ(op);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(REG_ZZZ(op) + REG_F_C > REG_A);
            REG_A = intermediate & 0xFF;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(1);
            break;

        // SBC A, (HL)
        case 0x9E:
            ctx.t_cycles = 8;
            intermediate = bus_read(REG_HL);
            REG_F_SET_C(REG_F_C + intermediate > REG_A);
            intermediate = REG_A - REG_F_C - intermediate;
            REG_F_SET_H(intermediate > 0x0F);
            REG_A = intermediate & 0xFF;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(1);
            break;

        // SBC A, A
        case 0x9F:
            ctx.t_cycles = 4;
            REG_A = 0 - REG_F_C;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(1);
            REG_F_SET_H(REG_F_C == 1);  // underflow
            REG_F_SET_C(REG_F_C == 1);
            break;

        // AND A, r8
        case 0xA0 ... 0xA5:
            ctx.t_cycles = 4;
            REG_A = REG_A & REG_ZZZ(op);
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(1);
            REG_F_SET_C(0);
            break;

        // AND A, (HL)
        case 0xA6:
            ctx.t_cycles = 8;
            REG_A = REG_A & bus_read(REG_HL);
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(1);
            REG_F_SET_C(0);
            break;

        // AND A, A
        case 0xA7:
            ctx.t_cycles = 4;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(1);
            REG_F_SET_C(0);
            break;

        // XOR A, r8
        case 0xA8 ... 0xAD:
            ctx.t_cycles = 4;
            intermediate = REG_A ^ REG_ZZZ(op);
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;
        
        // XOR A, (HL)
        case 0xAE:
            ctx.t_cycles = 8;
            intermediate = REG_A ^ bus_read(REG_HL);
            REG_A = intermediate & 0xFF;
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // XOR A, A
        case 0xAF:
            ctx.t_cycles = 4;
            REG_A = 0;
            REG_F_SET_Z(1);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // OR A, r8
        case 0xB0 ... 0xB5:
            ctx.t_cycles = 4;
            intermediate = REG_A | REG_ZZZ(op);
            REG_A = intermediate;
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // OR A, (HL)
        case 0xB6:
            ctx.t_cycles = 8;
            intermediate = REG_A | fetch();
            REG_A = intermediate;
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // OR A, A
        case 0xB7:
            ctx.t_cycles = 4;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // CP A, r8 - Compare A to r8
        case 0xB8 ... 0xBD:
            ctx.t_cycles = 4;
            intermediate = REG_A - REG_ZZZ(op);
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(1);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            break;

        // CP A, (HL) - Compare A to (HL)
        case 0xBE:
            ctx.t_cycles = 8;
            intermediate = REG_A - bus_read(REG_HL);
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(1);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            break;

        // CP A, A - Compare A to A
        case 0xBF:
            ctx.t_cycles = 4;
            REG_F_SET_Z(1);
            REG_F_SET_N(1);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // RET NZ - return not zero
        case 0xC0:
            if (!REG_F_Z) {
                ctx.t_cycles = 20;
                ctx.pc = bus_read_16(ctx.sp);
                ctx.sp += 2;
            } else {
                ctx.t_cycles = 8;
                // Do nothing
            }
            break;

        // POP BC
        case 0xC1:
            ctx.t_cycles = 12;
            intermediate = bus_read_16(ctx.sp);
            ctx.sp += 2;
            REG_BC_SET(intermediate);
            break;

        // Jump not zero
        case 0xC2:
            intermediate = fetch_16();
            if (!REG_F_Z) {
                ctx.t_cycles = 16;
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
            }
            break;

        // Jump unconditional
        case 0xC3:
            ctx.t_cycles = 16;
            ctx.pc = fetch_16();
            break;

        // CALL NZ, u16
        case 0xC4:
            intermediate = fetch_16();
            if (!REG_F_Z) {
                ctx.t_cycles = 24;
                ctx.sp -= 2;
                bus_write_16(ctx.sp, ctx.pc);
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
                // Do nothing
            }
            break;

        // PUSH BC
        case 0xC5:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, REG_BC);
            break;
        
        // ADD A, u8
        case 0xC6:
            ctx.t_cycles = 8;
            intermediate = REG_A + fetch();
            REG_A = intermediate & 0xFF;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            break;

        // RST 00H
        case 0xC7:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x00;
            break;

        // RET Z
        case 0xC8:
            if (REG_F_Z) {
                ctx.t_cycles = 20;
                ctx.pc = bus_read_16(ctx.sp);
                ctx.sp += 2;
            } else {
                ctx.t_cycles = 8;
                // Do nothing
            }
            break;

        // RET
        case 0xC9:
            ctx.t_cycles = 16;
            ctx.pc = bus_read_16(ctx.sp);
            ctx.sp += 2;
            break;

        // JP Z, u16
        case 0xCA:
            intermediate = fetch_16();
            if (REG_F_Z) {
                ctx.t_cycles = 16;
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
            }
            break;

        // CB prefix table
        case 0xCB:
            op = fetch();
            ctx.t_cycles = 8;
            switch (op) {
                
                // RLC r8 - minus A
                case 0x00 ... 0x05:
                    REG_F_SET_C(REG_ZZZ(op) >> 7);
                    intermediate = (REG_ZZZ(op) << 1) | REG_F_C;
                    REG_ZZZ(op) = intermediate;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // RLC (HL)
                case 0x06:
                    ctx.t_cycles += 4;
                    intermediate = bus_read(REG_HL);
                    REG_F_SET_C(intermediate >> 7);
                    intermediate = (intermediate << 1) | REG_F_C;
                    bus_write(REG_HL, intermediate & 0xFF);
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // RLC A
                case 0x07:
                    REG_F_SET_C(REG_A >> 7);
                    intermediate = (REG_A << 1) | REG_F_C;
                    REG_A = intermediate;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // RRC r8 - minus A
                case 0x08 ... 0x0D:
                    REG_F_SET_C(REG_ZZZ(op) & 0x01);
                    intermediate = (REG_F_C << 7) | (REG_ZZZ(op) >> 1);
                    REG_ZZZ(op) = intermediate;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;
                
                // RRC (HL)
                case 0x0E:
                    ctx.t_cycles += 4;
                    intermediate = bus_read(REG_HL);
                    REG_F_SET_C(intermediate & 0x01);
                    intermediate = (REG_F_C << 7) | (intermediate >> 1);
                    bus_write(REG_HL, intermediate & 0xFF);
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // RRC A
                case 0x0F:
                    REG_F_SET_C(REG_A & 0x01);
                    intermediate = (REG_F_C << 7) | (REG_A >> 1);
                    REG_A = intermediate;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;
                
                // RL r8 - minus A
                case 0x10 ... 0x15:
                    intermediate = REG_ZZZ(op) >> 7;
                    REG_ZZZ(op) = (REG_ZZZ(op) << 1) | REG_F_C;
                    REG_F_SET_Z(REG_ZZZ(op) == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    REG_F_SET_C(intermediate);
                    break;

                // RL (HL)
                case 0x16:
                    ctx.t_cycles += 4;
                    intermediate = (bus_read(REG_HL) << 1) | REG_F_C;
                    REG_F_SET_C((intermediate >> 8) & 0x01);
                    bus_write(REG_HL, intermediate & 0xFF);
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // RL A
                case 0x17:
                    intermediate = REG_A >> 7;
                    REG_A = (REG_A << 1) | REG_F_C;
                    REG_F_SET_Z(REG_A == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    REG_F_SET_C(intermediate);
                    break;

                // RR r8 - minus A
                case 0x18 ... 0x1D:
                    intermediate = REG_ZZZ(op) & 0x01;
                    REG_ZZZ(op) = (REG_F_C << 7) | (REG_ZZZ(op) >> 1);
                    REG_F_SET_Z(REG_ZZZ(op) == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    REG_F_SET_C(intermediate);
                    break;
                
                // RR (HL)
                case 0x1E:
                    ctx.t_cycles += 4;
                    intermediate = (bus_read(REG_HL) << 1) | REG_F_C;
                    REG_F_SET_C((intermediate & 0x02) >> 1);
                    bus_write(REG_HL, intermediate & 0xFF);
                    REG_F_SET_Z(REG_ZZZ(op) == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // RR A
                case 0x1F:
                    intermediate = REG_A & 0x01;
                    REG_A = (REG_F_C << 7) | (REG_A >> 1);
                    REG_F_SET_Z(REG_A == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    REG_F_SET_C(intermediate);
                    break;
                
                // SLA r8 - minus A
                case 0x20 ... 0x25:
                    intermediate = REG_ZZZ(op);
                    REG_F_SET_C(intermediate >> 7);
                    REG_ZZZ(op) = intermediate << 1;
                    REG_F_SET_Z(REG_ZZZ(op) == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // SLA (HL)
                case 0x26:
                    ctx.t_cycles += 4;
                    intermediate = bus_read(REG_HL);
                    REG_F_SET_C(intermediate >> 7);
                    intermediate = intermediate << 1;
                    bus_write(REG_HL, intermediate);
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // SLA A
                case 0x27:
                    intermediate = REG_A;
                    REG_F_SET_C(intermediate >> 7);
                    REG_A = intermediate << 1;
                    REG_F_SET_Z(REG_A == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // SRA r8 - minus A
                case 0x28 ... 0x2D:
                    intermediate = REG_ZZZ(op);
                    REG_F_SET_C(intermediate & 0x01);
                    REG_ZZZ(op) = (intermediate & 0x80) | intermediate >> 1;
                    REG_F_SET_Z(REG_ZZZ(op) == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;
                
                // SRA (HL)
                case 0x2E:
                    ctx.t_cycles += 4;
                    intermediate = bus_read(REG_HL);
                    REG_F_SET_C(intermediate & 0x01);
                    intermediate = (intermediate & 0x80) | intermediate >> 1;
                    bus_write(REG_HL, intermediate);
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // SRA A
                case 0x2F:
                    intermediate = REG_A;
                    REG_F_SET_C(intermediate & 0x01);
                    REG_A = (intermediate & 0x80) | intermediate >> 1;
                    REG_F_SET_Z(REG_A == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;
                
                // SWAP r8 - Minus A
                case 0x30 ... 0x35:
                    intermediate = REG_ZZZ(op);
                    intermediate = (intermediate & 0xF0) >> 4 | (intermediate & 0x0F) << 4;
                    REG_ZZZ(op) = intermediate;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    REG_F_SET_C(0);
                    break;

                // SWAP (HL)
                case 0x36:
                    ctx.t_cycles += 4;
                    intermediate = bus_read(REG_HL);
                    intermediate = (intermediate & 0xF0) >> 4 | (intermediate & 0x0F) << 4;
                    bus_write(REG_HL, intermediate);
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    REG_F_SET_C(0);
                    break;

                // SWAP A
                case 0x37:
                    intermediate = REG_A;
                    intermediate = (intermediate & 0xF0) >> 4 | (intermediate & 0x0F) << 4;
                    REG_A = intermediate;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    REG_F_SET_C(0);
                    break;

                // SRL r8 - Minus A
                case 0x38 ... 0x3D:
                    intermediate = REG_ZZZ(op);
                    REG_F_SET_C(intermediate & 0x01);
                    REG_ZZZ(op) = intermediate >> 1;
                    REG_F_SET_Z(REG_ZZZ(op) == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;
                
                // SRL (HL)
                case 0x3E:
                    ctx.t_cycles += 4;
                    intermediate = bus_read(REG_HL);
                    REG_F_SET_C(intermediate & 0x01);
                    intermediate = intermediate >> 1;
                    bus_write(REG_HL, intermediate);
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);
                    break;

                // SRL A
                case 0x3F:
                    intermediate = REG_A;
                    REG_F_SET_C(intermediate & 0x01);
                    REG_A = intermediate >> 1;
                    REG_F_SET_Z(REG_A == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(0);

                // BIT n, r8 - minus A
                case 0x40 ... 0x45:
                case 0x48 ... 0x4D:
                case 0x50 ... 0x55:
                case 0x58 ... 0x5D:
                case 0x60 ... 0x65:
                case 0x68 ... 0x6D:
                case 0x70 ... 0x75:
                case 0x78 ... 0x7D:
                    intermediate = YYY(op);  // n
                    intermediate = (REG_ZZZ(op) >> intermediate) & 0x01;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(1);
                    break;

                // BIT n, (HL)
                case 0x46: case 0x4E:
                case 0x56: case 0x5E:
                case 0x66: case 0x6E:
                case 0x76: case 0x7E:
                    ctx.t_cycles += 4;
                    intermediate = YYY(op);  // n
                    intermediate = (bus_read(REG_HL) >> intermediate) & 0x01;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(1);
                    break;

                // BIT n, A
                case 0x47: case 0x4F:
                case 0x57: case 0x5F:
                case 0x67: case 0x6F:
                case 0x77: case 0x7F:
                    intermediate = YYY(op);  // n
                    intermediate = (REG_A >> intermediate) & 0x01;
                    REG_F_SET_Z(intermediate == 0);
                    REG_F_SET_N(0);
                    REG_F_SET_H(1);
                    break;

                // RES n r8 - minus A
                case 0x80 ... 0x85:
                case 0x88 ... 0x8D:
                case 0x90 ... 0x95:
                case 0x98 ... 0x9D:
                case 0xA0 ... 0xA5:
                case 0xA8 ... 0xAD:
                case 0xB0 ... 0xB5:
                case 0xB8 ... 0xBD:
                    intermediate = YYY(op);  // n
                    intermediate = 1 << intermediate;
                    REG_ZZZ(op) ^= intermediate;
                    break;

                // RES n (HL)
                case 0x86: case 0x8E:
                case 0x96: case 0x9E:
                case 0xA6: case 0xAE:
                case 0xB6: case 0xBE:
                    ctx.t_cycles += 4;
                    intermediate = YYY(op);  // n
                    intermediate = 1 << intermediate;
                    bus_write(REG_HL, bus_read(REG_HL) << intermediate);
                    break;

                // RES n, A
                case 0x87: case 0x8F:
                case 0x97: case 0x9F:
                case 0xA7: case 0xAF:
                case 0xB7: case 0xBF:
                    intermediate = YYY(op);  // n
                    intermediate = 1 << intermediate;
                    REG_A ^= intermediate;
                    break;
                
                // SET n r8 - minus A
                case 0xC0 ... 0xC5:
                case 0xC8 ... 0xCD:
                case 0xD0 ... 0xD5:
                case 0xD8 ... 0xDD:
                case 0xE0 ... 0xE5:
                case 0xE8 ... 0xED:
                case 0xF0 ... 0xF5:
                case 0xF8 ... 0xFD:
                    intermediate = YYY(op);  // n
                    REG_ZZZ(op) |= (1 << intermediate);
                    break;

                // SET n (HL)
                case 0xC6: case 0xCE:
                case 0xD6: case 0xDE:
                case 0xE6: case 0xEE:
                case 0xF6: case 0xFE:
                    ctx.t_cycles += 4;
                    intermediate = YYY(op);  // n
                    intermediate = (1 << intermediate);
                    bus_write_16(REG_HL, bus_read(REG_HL) | intermediate);
                    break;

                // SET n, A
                case 0xC7: case 0xCF:
                case 0xD7: case 0xDF:
                case 0xE7: case 0xEF:
                case 0xF7: case 0xFF:
                    intermediate = YYY(op);  // n
                    REG_A |= (1 << intermediate);
                    break;
            }
            break;  // end CB prefix

        // CALL Z, u16 - Call if zero
        case 0xCC:
            intermediate = fetch_16();
            if (REG_F_Z) {
                ctx.t_cycles = 24;
                ctx.sp -= 2;
                bus_write_16(ctx.sp, ctx.pc);
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
            }
            break;

        // CALL u16 - Call immediate 16
        case 0xCD:
            ctx.t_cycles = 24;
            intermediate = fetch_16();
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = intermediate;
            break;

        // ADC A, u8 - Add carry + immediate to register A (8)
        case 0xCE:
            ctx.t_cycles = 8;
            intermediate = REG_A + REG_F_C + fetch();
            REG_F_SET_Z((intermediate & 0xFF) == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            REG_A = intermediate & 0xFF;
            break;

        // RST 08H
        case 0xCF:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x08;
            break;

        // RET NC - return not carry
        case 0xD0:
            if (!REG_F_C) {
                ctx.t_cycles = 20;
                ctx.pc = bus_read_16(ctx.sp);
                ctx.sp += 2;
            } else {
                ctx.t_cycles = 8;
                // Do nothing
            }
            break;

        // POP DE
        case 0xD1:
            ctx.t_cycles = 12;
            intermediate = bus_read_16(ctx.sp);
            ctx.sp += 2;
            REG_DE_SET(intermediate);
            break;

        // JP NC, u16
        case 0xD2:
            intermediate = fetch_16();
            if (!REG_F_C) {
                ctx.t_cycles = 16;
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
            }
            break;

        // CALL NC, u16
        case 0xD4:
            intermediate = fetch_16();
            if (!REG_F_C) {
                ctx.t_cycles = 24;
                ctx.sp -= 2;
                bus_write_16(ctx.sp, ctx.pc);
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
                // do nothing
            }
            break;

        // PUSH DE
        case 0xD5:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, REG_DE);
            break;

        // SUB A, u8
        case 0xD6:
            ctx.t_cycles = 8;
            intermediate = fetch();
            REG_F_SET_C(intermediate > REG_A);
            REG_F_SET_H(intermediate > 0x0F);
            intermediate = REG_A - intermediate;
            REG_A = intermediate;
            REG_F_SET_Z((intermediate && 0xFF) == 0);
            REG_F_SET_N(1);
            break;

        // RST 10H
        case 0xD7:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x10;
            break;

        // RET C
        case 0xD8:
            if (REG_F_C) {
                ctx.t_cycles = 20;
                ctx.pc = bus_read_16(ctx.sp);
                ctx.sp += 2;
            } else {
                ctx.t_cycles = 8;
                // do nothing
            }
            break;

        // JP C, u16
        case 0xDA:
            intermediate = fetch_16();
            if (REG_F_C) {
                ctx.t_cycles = 16;
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
                // do nothing
            }
            break;

        // CALL C, u16
        case 0xDC:
            intermediate = fetch_16();
            if (REG_F_C) {
                ctx.t_cycles = 24;
                ctx.sp -= 2;
                bus_write_16(ctx.sp, ctx.pc);
                ctx.pc = intermediate;
            } else {
                ctx.t_cycles = 12;
                // do nothing
            }
            break;

        // SBC A, u8
        case 0xDE:
            ctx.t_cycles = 8;
            intermediate = REG_A - REG_F_C - fetch();
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(REG_ZZZ(op) + REG_F_C > REG_A);
            REG_A = intermediate & 0xFF;
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(1);
            break;

        // RST 18H
        case 0xDF:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x18;
            break;

        // LD (FF00+u8), A | LD [C], A
        case 0xE0:
            ctx.t_cycles = 12;
            intermediate = 0xFF00 + fetch();
            bus_write(intermediate, REG_A);
            break;

        // POP HL
        case 0xE1:
            ctx.t_cycles = 12;
            intermediate = bus_read_16(ctx.sp);
            ctx.sp += 2;
            REG_HL_SET(intermediate);
            break;

        // LD (FF00+C), A
        case 0xE2:
            ctx.t_cycles = 8;
            intermediate = 0xFF00 + REG_F_C;
            bus_write(intermediate, REG_A);
            break;

        // PUSH HL
        case 0xE5:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, REG_HL);
            break;

        // AND A, u8
        case 0xE6:
            ctx.t_cycles = 8;
            REG_A = REG_A & fetch();
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(1);
            REG_F_SET_C(0);
            break;

        // RST 20H
        case 0xE7:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x20;
            break;

        // ADD SP, i8
        case 0xE8:
            ctx.t_cycles = 16;
            intermediate = fetch();
            ctx.sp += (int8_t) intermediate;
            REG_F_SET_Z(0);
            REG_F_SET_N(0);
            REG_F_SET_H(ctx.sp > 0x0F);
            REG_F_SET_C(ctx.sp > 0xFF);
            break;

        // JP HL
        case 0xE9:
            ctx.t_cycles = 4;
            ctx.pc = REG_HL;
            break;

        // LD (u16), A
        case 0xEA:
            ctx.t_cycles = 16;
            intermediate = fetch_16();
            bus_write_16(intermediate, REG_A);
            break;

        // XOR A, r8
        case 0xEE:
            ctx.t_cycles = 8;
            REG_A = REG_A ^ fetch();
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // RST 28H
        case 0xEF:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x28;
            break;

        // LD A, (FF00+u8) | LD A,[C]
        case 0xF0:
            ctx.t_cycles = 12;
            intermediate = 0xFF00 + fetch();
            REG_A = bus_read(intermediate);
            break;

        // POP AF
        case 0xF1:
            ctx.t_cycles = 12;
            intermediate = bus_read_16(ctx.sp);
            ctx.sp += 2;
            REG_F_SET_Z((intermediate >> 15) & 0x01);
            REG_F_SET_N((intermediate >> 14) & 0x01);
            REG_F_SET_H((intermediate >> 13) & 0x01);
            REG_F_SET_C((intermediate >> 12) & 0x01);
            REG_A = intermediate & 0xFF;
            break;

        // LD A, (FF00 + C)
        case 0xF2:
            ctx.t_cycles = 8;
            intermediate = 0xFF00 + REG_F_C;
            REG_A = bus_read(intermediate);
            break;

        // DI - Disable Interrupts
        case 0xF3:
            ctx.t_cycles = 4;
            bus_write(BUS_IE_REG_ADDR, 0);
            break;

        // PUSH AF
        case 0xF5:
            ctx.t_cycles = 16;
            intermediate = 0;
            intermediate += REG_F.zero        << 15
                          | REG_F.subtraction << 14
                          | REG_F.half_carry  << 13
                          | REG_F.carry       << 12
                          | REG_A;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, intermediate);
            break;

        // OR A, u8
        case 0xF6:
            ctx.t_cycles = 8;
            REG_A = REG_A | fetch();
            REG_F_SET_Z(REG_A == 0);
            REG_F_SET_N(0);
            REG_F_SET_H(0);
            REG_F_SET_C(0);
            break;

        // RST 30H
        case 0xF7:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x30;
            break;

        // LD HL, SP+i8
        case 0xF8:
            ctx.t_cycles = 12;
            intermediate = ctx.sp + (int8_t) fetch();
            REG_HL_SET(intermediate);
            break;

        // LD SP, HL
        case 0xF9:
            ctx.t_cycles = 8;
            ctx.sp = REG_HL;
            break;

        // LD A, (u16)
        case 0xFA:
            ctx.t_cycles = 16;
            intermediate = bus_read(fetch_16());
            REG_A = intermediate;
            break;

        // EI - Enable Interrupts
        case 0xFB:
            ctx.t_cycles = 4;
            bus_write(BUS_IE_REG_ADDR, 1);
            break;

        // CP A, u8 - Compare register A to immediate 8
        case 0xFE:
            ctx.t_cycles = 8;
            intermediate = REG_A - fetch();
            REG_F_SET_Z(intermediate == 0);
            REG_F_SET_N(1);
            REG_F_SET_H(intermediate > 0x0F);
            REG_F_SET_C(intermediate > 0xFF);
            break;
        
        // RST 38H
        case 0xFF:
            ctx.t_cycles = 16;
            ctx.sp -= 2;
            bus_write_16(ctx.sp, ctx.pc);
            ctx.pc = 0x38;
            break;

        default:
            printf("* cpu execute - Unimplemented 0x%02X\n", op);
            exit(1);
    }
}

void cpu_init(void) {
    REG_A = 0x01;
    REG_F_SET_Z(1);
    REG_F_SET_N(0);
    REG_F_SET_H(0);
    REG_F_SET_C(0);

    REG_B = 0x00;
    REG_C = 0x13;
    REG_D = 0x00;
    REG_E = 0xd8;
    REG_H = 0x01;
    REG_L = 0x4d;

    ctx.pc = 0x0100;
    ctx.sp = 0xfffe;
}

void cpu_step(void) {
    ctx.t_cycles--;
    if (ctx.t_cycles > 0) {
        return;
    }
    // ctx.t_cycles to be added onto by execute

    // print_state();
    uint8_t op = fetch();
    execute(op);
}