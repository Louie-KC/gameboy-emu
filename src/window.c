#include "window.h"
#include <SDL2/SDL.h>

static SDL_Window   *window;
static SDL_Renderer *renderer;
static SDL_Texture  *texture;

uint8_t window_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("[ERROR] Failed to initialise SDL2 video: %s\n", SDL_GetError());
        return 0;
    }
    window = SDL_CreateWindow("Gameboy Emulator", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, GB_SCREEN_RES_X, GB_SCREEN_RES_Y,
        SDL_WINDOW_SHOWN);
    if (!window) {
        printf("[ERROR] Failed to create SDL2 window: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("[ERROR] Failed to create SDL2 renderer: %s\n", SDL_GetError());
        window_exit();
        return 0;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STATIC, GB_SCREEN_RES_X, GB_SCREEN_RES_Y);
    if (!texture) {
        printf("[ERROR] Failed to create SDL2 texture: %s\n", SDL_GetError());
        window_exit();
        return 0;
    }

    return 1;
}

void window_step(void) {
    SDL_Event event;

    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) {
        emu_run = 0;    
    }
}

void window_exit(void) {
    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}