#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>

#define TARGET_FPS 120
#define TARGET_DT (1.0 / TARGET_FPS)
#define WIDTH 960
#define HEIGHT 640
#define PADDLE_W WIDTH * 0.18
#define PADDLE_H HEIGHT * 0.03

SDL_Color off_black = {33, 33, 33, 255};
SDL_Color black = {10, 10, 10, 255};
SDL_Color yellow = {187, 165, 59};
SDL_Color green = {99, 141, 91, 255};
SDL_Color pink = {179, 100, 138};
SDL_Color red = {154, 78, 78};
SDL_Color grey = {195, 195, 195, 255};
SDL_Color white = {220, 220, 220, 255};

typedef struct {
    SDL_FRect shape;
    float velocity;
    float move_speed;
    bool move_left;
    bool move_right;
} Player;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    Player player;
} GameState;

GameState *game = NULL;

bool gamestate_create() {
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
    
    game->player = (Player) {
        .shape = (SDL_FRect) {.x = (WIDTH * 0.5f) - (PADDLE_W * 0.5f), .y = HEIGHT - (2*PADDLE_H), .w = PADDLE_W, .h = PADDLE_H},
        .move_speed = WIDTH * 0.5f,
        .move_left = false,
        .move_right = false
    };    


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

void update_game(double dt) {
    
    { 
        //::update player 
        if (game->player.move_left && !game->player.move_right) game->player.velocity = -1;
        else if (game->player.move_right && !game->player.move_left) game->player.velocity = 1;
        else game->player.velocity = 0;
        
        float new_x = game->player.shape.x + (game->player.velocity * game->player.move_speed * dt);
        if (new_x < 0 || new_x + PADDLE_W > WIDTH) new_x = game->player.shape.x;
        game->player.shape.x = new_x;     
    }
      
}

void draw_game() {
    SDL_SetRenderDrawColor(game->renderer, off_black.r, off_black.g, off_black.b, off_black.a);
    SDL_RenderClear(game->renderer);

    //::draw player
    SDL_SetRenderDrawColor(game->renderer, grey.r, grey.g, grey.b, grey.a);
    SDL_RenderFillRect(game->renderer, &game->player.shape);


    SDL_RenderPresent(game->renderer);
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

    SDL_Event e;

    uint64_t current_time = SDL_GetPerformanceCounter();
    uint64_t last_time = 0;
    double delta_time = 0.0f;
    
    while (running) {
        last_time = current_time;
        current_time = SDL_GetPerformanceCounter();
        delta_time = (double)((current_time - last_time) * 1000) / SDL_GetPerformanceFrequency();           
        printf("dt %lf\n", delta_time);
       
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                    case SDLK_A:
                    case SDLK_LEFT:
                        game->player.move_left = true;
                        break;
                    case SDLK_D:
                    case SDLK_RIGHT:
                        game->player.move_right = true;
                        break;
                }
            }
            if (e.type == SDL_EVENT_KEY_UP) {
                switch (e.key.key) {
                    case SDLK_A:
                    case SDLK_LEFT:
                        game->player.move_left = false;
                        break;
                    case SDLK_D:
                    case SDLK_RIGHT:
                        game->player.move_right = false;
                        break;
                }
            }
        }

        static double accumulator = 0.0f;
        accumulator += delta_time / 1000.0f;
        while (accumulator >= TARGET_DT) {
            update_game(TARGET_DT);
            accumulator -= TARGET_DT;
        }

        draw_game();

        double frame_time = (SDL_GetPerformanceCounter() - current_time) * 1000.0 / SDL_GetPerformanceFrequency();
        if (frame_time < (1000.0f / TARGET_FPS)) {
            SDL_Delay((1000.0f / TARGET_FPS) - frame_time);
        }

    }

    free_gamestate();
    SDL_Quit();
    return 0;
}
