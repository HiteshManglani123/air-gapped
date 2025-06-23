#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "burger_stack_gui.h"
#include "secret_formula.h"

SDL_Window *window;
SDL_Renderer *renderer;

int window_x = 500;
int window_y = 1200;

uint32_t start_y = 10;
uint32_t padding = 1;

enum SECRET_FORMULA_LOOKUP images[100];
uint8_t image_idx = 0;

int setup_sdl(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        return 0;
    }

    window = SDL_CreateWindow("Secret Formula", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_x, window_y, 0);

    if (!window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);

    // Draws white image on screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    return 1;
}

void cleanup_sdl(void)
{
    // destroy renderer
    SDL_DestroyRenderer(renderer);

    // destroy window
    SDL_DestroyWindow(window);

    // close SDL
    SDL_Quit();
}

void add_image(enum SECRET_FORMULA_LOOKUP id)
{
    SDL_RenderClear(renderer);

    images[image_idx++] = id;

    for (int i = 0; i < image_idx; i++) {
        char *image_path;

        if (images[i] == UNKNOWN) {
            image_path = "images/unknown.jpg";
        } else {
            image_path = secret_formula_lookup[images[i]].image_path;
        }

        SDL_Surface *surface = IMG_Load(image_path);

        if (!surface) {
            printf("IMG_Load error: %s\n", IMG_GetError());
            return;
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_FreeSurface(surface);

        // control destination of image
        SDL_Rect destination;
        SDL_QueryTexture(texture, NULL, NULL, &destination.w, &destination.h);

        destination.w = 150;
        destination.h = 100;

        // center x
        destination.x = (window_x - destination.w) / 2;

        destination.y = start_y + i * (destination.h + padding);

        SDL_RenderCopy(renderer, texture, NULL, &destination);

        // destroy texture
        SDL_DestroyTexture(texture);

    }

    SDL_RenderPresent(renderer);
}
