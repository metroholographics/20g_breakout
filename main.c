#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>

#define WIDTH 960
#define HEIGHT 640


typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
} GameState;

GameState *game = NULL;

bool  gamestate_create() {
    game = malloc(sizeof(GameState));

    game->window = SDL_CreateWindow("20g_pong", WIDTH, HEIGHT, 0);
    if (game->window == NULL) {
        printf("Error_window: %s\n", SDL_GetError());
        return false;
    }
    game->renderer = SDL_CreateRenderer(game->window, NULL);
    if (game->renderer == NULL) {
        printf("Error_renderer: %s\n", SDL_GetError());
        return false;
    }    


    return true;
}

void free_gamestate() {
    if (game == NULL) {
        printf("gamestate empty\n");
        return;
    }

    SDL_DestroyRenderer(game->renderer);
    game->renderer = NULL;
    SDL_DestroyWindow(game->window);
    game->window = NULL;
    

    free(game);
    game = NULL;
}


int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error_init: %s\n", SDL_GetError());
        return -1;
    }

    bool running = false;
    if (gamestate_create()) {
        running = true;
    }

    while (running) {
        
    }


    
    free_gamestate();
    SDL_Quit();
    return 0;
}
