#pragma once

#include "common.h"

#define PPU_BG_SIZE 256

typedef enum {
    OAM_SCAN,
    DRAW_LINE,
    H_BLANK,
    V_BLANK
} ppu_state;

typedef struct {
    // from bus
    uint8_t bg_window_enable; // 0xFF40:0
    uint8_t obj_enable;       // 0xFF40:1
    uint8_t obj_size;         // 0xFF40:2
    uint8_t bg_tile;          // 0xFF40:3
    uint8_t bg_window_tile;   // 0xFF40:4
    uint8_t window_enable;    // 0xFF40:5
    uint8_t window_tile_map;  // 0xFF40:6
    uint8_t ppu_enable;       // 0xFF40:7
    uint8_t STAT;             // 0xFF41
    uint8_t SCY;
    uint8_t SCX;
    uint8_t LY;               // 0xFF44
    uint8_t LYC;              // 0xFF45
    uint8_t WX;
    uint8_t WY;

    // internal state
    uint8_t   bg[PPU_BG_SIZE * PPU_BG_SIZE][3];  // [r, g, b]
    uint32_t  bg_idx;
    ppu_state state;
    uint16_t  cycles;
    uint8_t   n_line_pixels_drawn;
} ppu_context;

uint8_t ppu_view[GB_SCREEN_RES_X * GB_SCREEN_RES_Y * 3];

// Indicate that SDL should draw the current PPU view
uint8_t ppu_view_updated;

void ppu_init(void);
void ppu_step(void);
void ppu_update_view(void);