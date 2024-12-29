#include "ppu_fetcher.h"
#include "ppu.h"
#include "bus.h"

#define TILE_MAP_SIZE 16
#define TILE_MAP_WIDTH 32
#define TILE_SIDE_LENGTH 8

void queue_init(queue *q) {
    q->read = 0;
    q->write = 0;
    q->count = 0;
}

uint8_t queue_is_empty(queue *q) {
    return q->count == 0;
}

uint8_t queue_is_full(queue *q) {
    return q->count == FETCHER_QUEUE_SIZE;
}

uint8_t queue_count(queue *q) {
    return q->count;
}

void queue_push(queue *q, uint8_t byte) {
    q->buffer[q->write] = byte;
    q->write += 1;
    q->write %= FETCHER_QUEUE_SIZE;
    q->count += 1;
}

uint8_t queue_pop(queue *q) {
    uint8_t byte = q->buffer[q->read];
    q->read += 1;
    q->read %= FETCHER_QUEUE_SIZE;
    q->count -= 1;
    return byte;
}

void ppu_feetcher_init(ppu_fetcher *f) {
    f->cycles = 1;
    f->row_data[0] = 0;
    f->row_data[1] = 0;
    f->row_data[2] = 0;
    f->row_data[3] = 0;
    f->row_data[4] = 0;
    f->row_data[5] = 0;
    f->row_data[6] = 0;
    f->row_data[7] = 0;
}

// Prepare the PPU Fetcher to begin at a new scanline.
void ppu_fetcher_set(ppu_fetcher *f, uint16_t map_base_addr, uint8_t scx, uint8_t scy, uint8_t ly) {
    // To be used for scrolling
    (void) scx;
    (void) scy;
    
    // Adjust LY to fit the 8x8 scheme of tiles.
    uint8_t num_tile_rows_drawn = ly / TILE_SIDE_LENGTH;
    f->tile_map_line_addr = map_base_addr + num_tile_rows_drawn * TILE_MAP_WIDTH;
    f->tile_map_index_in_line = 0;

    // Adjust to the current line within the current tile
    f->tile_current_line = ly % TILE_SIDE_LENGTH;

    f->state = READ_ID;
    
    // Re-initialise to ensure any previous scanline values are not used
    queue_init(&f->queue);
}

void ppu_fetcher_step(ppu_fetcher *f, uint8_t signed_addr_mode) {
    // Do work every 2nd time the step function is called.
    f->cycles--;
    if (f->cycles > 0) {
        return;
    }
    f->cycles = 2;

    switch (f->state) {
        case READ_ID: {
            // Calculate address of tile id index
            uint16_t tile_map_entry_addr;

            tile_map_entry_addr = f->tile_map_line_addr + f->tile_map_index_in_line;
            f->tile_id = bus_read(tile_map_entry_addr);

            // Calculate address of tile id data
            if (signed_addr_mode) {
                f->tile_addr = 0x9000 + (int8_t) (f->tile_id * TILE_MAP_SIZE);
            } else {
                f->tile_addr = BUS_VRAM_ADDR + (f->tile_id * TILE_MAP_SIZE);
            }
            // Skip over already drawn rows of 8 in tile data
            f->tile_addr += f->tile_current_line * 2;  // 2 bits per pixel

            f->state = READ_DATA_LOW;
            break;
        }
        case READ_DATA_LOW: {
            // Low bits of tile data
            uint8_t low_bits = bus_read(f->tile_addr);
            for (int i = 0; i < 8; i++) {
                f->row_data[i] = (low_bits >> i) & 0x01;
            }

            f->state = READ_DATA_HIGH;
            break;
        }
        case READ_DATA_HIGH: {
            // High bits of tile data
            uint8_t high_bits = bus_read(f->tile_addr + 1);
            for (int i = 0; i < 8; i++) {
                f->row_data[i] = ((high_bits >> i) & 0x01) << 1;
            }

            f->state = PUSH;
            break;
        }
        case PUSH:
            if (queue_is_full(&f->queue)) {
                break;
            }
            if (queue_count(&f->queue) > 8) {
                break;
            }
            for (int i = 7; i >= 0; i--) {
                queue_push(&f->queue, f->row_data[i]);
            }
            f->tile_map_index_in_line++;  // tile IDs to be drawn are stored sequentially
            f->state = READ_ID;
            break;

    }
}