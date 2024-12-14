#pragma once

#include "common.h"

#include <stdint.h>

typedef struct {
    uint8_t zero        : 1;  // z
    uint8_t subtraction : 1;  // n
    uint8_t half_carry  : 1;  // h
    uint8_t carry       : 1;  // c
    uint8_t _padding    : 4;
} cpu_flag;

typedef struct {
    uint8_t  registers[6]; // 0: B, 1: C, 2: D, 3: E, 4: H, 5: L
    uint8_t  register_a;
    cpu_flag register_f;
    uint16_t pc;
    uint16_t sp;
} cpu_context;

void cpu_init(void);
void cpu_step(void);
void cpu_execute(uint8_t op);