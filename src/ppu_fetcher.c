#include "ppu_fetcher.h"
#include "ppu.h"
#include "bus.h"

#define TILE_MAP_SIZE 16

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

void ppu_fetcher_set(ppu_fetcher *f, uint16_t id_base_addr, uint8_t scx, uint8_t scy, uint8_t ly) {
    // To be used for scrolling
    (void) scx;
    (void) scy;
    
    f->state = READ_ID;

    f->tile_id_base_addr = id_base_addr + (ly / 8) * 32;
    f->tile_id_index = 0;
    f->tile_current_line = ly % 8;
    
    // Re-initialise to ensure any previous scanline values are not used
    queue_init(&f->queue);
}

void ppu_fetcher_step(ppu_fetcher *f) {
    uint16_t intermediate;

    f->cycles++;
    if (f->cycles < 2) {
        return;
    } else {
        f->cycles = 0;
    }

    switch (f->state) {
        case READ_ID:
            // Calculate address of tile id index
            intermediate = f->tile_id_base_addr + f->tile_id_index;
            f->tile_id = bus_read(intermediate);

            // Calculate address of tile id data
            f->tile_addr = BUS_VRAM_ADDR + (f->tile_id * TILE_MAP_SIZE)
                         + (f->tile_current_line) * 2;

            f->state = READ_DATA_LOW;
            break;
            
        case READ_DATA_LOW:
            // Low bits of tile data
            intermediate = bus_read(f->tile_addr);
            for (int i = 0; i < 8; i++) {
                f->row_data[i] = (intermediate >> i) & 0x01;
            }

            f->state = READ_DATA_HIGH;
            break;

        case READ_DATA_HIGH:
            // High bits of tile data
            intermediate = bus_read(f->tile_addr + 1);
            for (int i = 0; i < 8; i++) {
                f->row_data[i] = ((intermediate >> i) & 0x01) << 1;
            }

            f->state = PUSH;
            break;

        case PUSH:
            for (int i = 7; i >= 0; i--) {
                queue_push(&f->queue, f->row_data[i]);
            }
            f->tile_id_index++;  // tile IDs to be drawn are stored sequentially
            f->state = READ_ID;
            break;

    }
}