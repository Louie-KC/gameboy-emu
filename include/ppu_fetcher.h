#pragma once

#include "common.h"

#define FETCHER_QUEUE_SIZE 16

typedef struct {
    uint8_t buffer[FETCHER_QUEUE_SIZE];
    uint8_t read;
    uint8_t write;
    uint8_t count;
} queue;

void queue_init(queue *q);
uint8_t queue_empty(queue *q);
uint8_t queue_full(queue *q);
uint8_t queue_count(queue *q);
void queue_push(queue *q, uint8_t byte);
uint8_t queue_pop(queue *q);

typedef enum {
    READ_ID,
    READ_DATA_LOW,
    READ_DATA_HIGH,
    PUSH
} ppu_f_state;

typedef struct {
    queue       queue;
    uint8_t     cycles;
    ppu_f_state state;
    uint8_t     row_data[8];
    uint16_t    tile_map_line_addr;
    uint8_t     tile_map_index_in_line;
    uint8_t     tile_id;
    uint16_t    tile_addr;
    uint8_t     tile_current_line;
} ppu_fetcher;

void ppu_feetcher_init(ppu_fetcher *f);
void ppu_fetcher_set(ppu_fetcher *f, uint16_t map_base_addr, uint8_t scx, uint8_t scy, uint8_t ly);
void ppu_fetcher_step(ppu_fetcher *f, uint8_t signed_addr_mode);
