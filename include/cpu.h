#pragma once

#include <stdint.h>

typedef struct {
    uint8_t  a;  // acc/arg
    uint8_t  b;
    uint8_t  c;
    uint8_t  d;
    uint8_t  e;
    uint8_t  f;  // flag
    uint8_t  h;  // addr
    uint8_t  l;  // addr
} cpu_registers;

typedef struct {
    cpu_registers registers;
    uint16_t pc;
    uint16_t sp;
} cpu_context;