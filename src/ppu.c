#include "ppu.h"
#include "ppu_fetcher.h"
#include "bus.h"

#define LCD_CTRL_ADDR 0xFF40
#define LCD_STAT_ADDR 0xFF41
#define LCD_SCY_ADDR  0xFF42
#define LCD_SCX_ADDR  0xFF43
#define LCD_LY_ADDR   0xFF44
#define LCD_LYC_ADDR  0xFF45
#define LCD_WY_ADDR   0xFF4A
#define LCD_WX_ADDR   0xFF4B

static ppu_context ctx;

static ppu_fetcher fetcher;

void read_reg_to_ctx(void);
void write_reg_from_ctx(void);

void ppu_init(void) {
    ctx.state = OAM_SCAN;
    ctx.cycles = 0;
    ctx.n_line_pixels_drawn = 0;
    ctx.bg_idx = 0;

    bus_write(LCD_CTRL_ADDR, 0x91);
    bus_write(LCD_STAT_ADDR, 0x81);
    bus_write(LCD_SCY_ADDR,  0x00);
    bus_write(LCD_SCX_ADDR,  0x00);
    bus_write(LCD_LY_ADDR,   0x91);
    bus_write(LCD_LYC_ADDR,  0x00);
}

void ppu_step(void) {
    read_reg_to_ctx();

    if (!ctx.ppu_enable) {
        return;
    }

    ctx.cycles++;

    switch (ctx.state) {
        case OAM_SCAN:
            // TODO

            if (ctx.cycles == 40) {
                uint16_t addr = 0x9800;
                ctx.n_line_pixels_drawn = 0;
                ppu_fetcher_set(&fetcher, addr, ctx.SCX, ctx.SCY, ctx.LY);
                ctx.state = DRAW_LINE;
            }
            break;
        
        case DRAW_LINE:
            ppu_fetcher_step(&fetcher);
            // there must be at least 8 pixels in the queue to draw
            if (queue_count(&fetcher.queue) <= 8) {
                break;
            }

            // 85 = 255/3
            uint8_t pixel = queue_pop(&fetcher.queue) * 85;

            ctx.bg_idx = (ctx.bg_idx + 1) % (PPU_BG_SIZE * PPU_BG_SIZE);

            ctx.bg[ctx.bg_idx][0] = pixel;
            ctx.bg[ctx.bg_idx][1] = pixel;
            ctx.bg[ctx.bg_idx][2] = pixel;

            ctx.n_line_pixels_drawn++;
            if (ctx.n_line_pixels_drawn == GB_SCREEN_RES_X) {
                ctx.state = H_BLANK;
            }
            break;
        
        case H_BLANK:
            if (ctx.cycles == 456) {
                ctx.cycles = 0;
                ctx.LY++;
                if (ctx.LY == 144) {
                    ctx.state = V_BLANK;
                } else {
                    ctx.state = OAM_SCAN;
                }
            }
            break;

        case V_BLANK:
            if (ctx.cycles == 456) {
                ctx.cycles = 0;
                ctx.LY++;
                if (ctx.LY == 153) {
                    ctx.LY = 0;
                    ctx.bg_idx = 0;  // stops the sliding around in BG
                    ctx.state = OAM_SCAN;
                }
            }
            break;
    }

    write_reg_from_ctx();
}

void ppu_update_view(void) {
    uint32_t i;
    uint32_t bg_i;
    uint8_t screen_x;
    uint8_t screen_y;

    for (int y = 0; y < GB_SCREEN_RES_Y; y++) {
        for (int x = 0; x < GB_SCREEN_RES_X; x++) {
            screen_x = (x + ctx.SCX) % PPU_BG_SIZE;
            screen_y = (y + ctx.SCY) % PPU_BG_SIZE;
            bg_i = screen_y * GB_SCREEN_RES_X + screen_x;
            i = y * GB_SCREEN_RES_X + x;
            ppu_view[i * 3]     = ctx.bg[bg_i][0];
            ppu_view[i * 3 + 1] = ctx.bg[bg_i][1];
            ppu_view[i * 3 + 2] = ctx.bg[bg_i][2];

            // // Tile grid lines
            // if (x % 8 == 0 || y % 8 == 0) {
            //     ppu_view[i * 3]     = 255;
            //     ppu_view[i * 3 + 1] = 255;
            //     ppu_view[i * 3 + 2] = 255;
            // }
        }
    }

    // Print BG to stdout
    // uint8_t chars[4] = {' ', '/', '%', '#'};
    // for (int x = 0; x < PPU_BG_SIZE; x++) {
    //     printf("-");
    // }
    // printf("\n");
    // for (int y = 0; y < PPU_BG_SIZE; y++) {
    //     printf("|");
    //     for (int x = 0; x < ctx.SCX; x++) {
    //         if (x == ctx.SCX || y == ctx.SCY || x - GB_SCREEN_RES_X == ctx.SCX || y - GB_SCREEN_RES_Y == ctx.SCY) {
    //             printf("v");
    //         } else {
    //             printf(" ");
    //         }
    //     }
    //     for (int x = 0; x < GB_SCREEN_RES_X; x++) {
    //         if (x == ctx.SCX || y == ctx.SCY || x - GB_SCREEN_RES_X == ctx.SCX || y - GB_SCREEN_RES_Y == ctx.SCY) {
    //             printf("v");
    //         } else {
    //             // printf(" ");
    //             i = y * GB_SCREEN_RES_X + x;
    //             printf("%c", chars[ctx.bg[i][0] / 85]);
    //         }
    //     }
    //     for (int x = ctx.SCX + GB_SCREEN_RES_X; x < PPU_BG_SIZE - 1; x++) {
    //         if (x == ctx.SCX || y == ctx.SCY || x - GB_SCREEN_RES_X == ctx.SCX || y - GB_SCREEN_RES_Y == ctx.SCY) {
    //             printf("v");
    //         } else {
    //             printf(" ");
    //         }
    //     }
    //     printf("|\n");
    // }
    // for (int x = 0; x < PPU_BG_SIZE; x++) {
    //     printf("-");
    // }
    // printf("\n\n");

}

void read_reg_to_ctx(void) {
    uint8_t LCDC = bus_read(LCD_CTRL_ADDR);

    ctx.bg_window_enable = LCDC & 0x01;
    ctx.obj_enable       = (LCDC >> 1) & 0x01;
    ctx.obj_size         = (LCDC >> 2) & 0x01;
    ctx.bg_tile          = (LCDC >> 3) & 0x01;
    ctx.bg_window_tile   = (LCDC >> 4) & 0x01;
    ctx.window_enable    = (LCDC >> 5) & 0x01;
    ctx.window_tile_map  = (LCDC >> 6) & 0x01;
    ctx.ppu_enable       = (LCDC >> 7) & 0x01;

    ctx.STAT = bus_read(LCD_STAT_ADDR);
    ctx.SCY  = bus_read(LCD_SCY_ADDR);
    ctx.SCX  = bus_read(LCD_SCX_ADDR);
    ctx.LY   = bus_read(LCD_LY_ADDR);
    ctx.LYC  = bus_read(LCD_LYC_ADDR);
    ctx.WY   = bus_read(LCD_WY_ADDR);
    ctx.WX   = bus_read(LCD_WX_ADDR);
}

void write_reg_from_ctx(void) {
    uint8_t LCDC = ctx.bg_window_enable
        | (ctx.obj_enable      << 1)
        | (ctx.obj_size        << 2)
        | (ctx.bg_tile         << 3)
        | (ctx.bg_window_tile  << 4)
        | (ctx.window_enable   << 5)
        | (ctx.window_tile_map << 6)
        | (ctx.ppu_enable      << 7);
    
    bus_write(LCD_CTRL_ADDR, LCDC);
    bus_write(LCD_STAT_ADDR, ctx.STAT);
    bus_write(LCD_SCY_ADDR,  ctx.SCY);
    bus_write(LCD_SCX_ADDR,  ctx.SCX);
    bus_write(LCD_LY_ADDR,   ctx.LY);
    bus_write(LCD_LYC_ADDR,  ctx.LYC);
    bus_write(LCD_WY_ADDR,   ctx.WY);
    bus_write(LCD_WX_ADDR,   ctx.WX);
}