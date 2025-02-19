#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TARGET_FPS 120
#define TARGET_DT (1.0 / TARGET_FPS)
#define WIDTH 960
#define HEIGHT 700

#define PADDLE_W WIDTH * 0.18
#define PADDLE_H HEIGHT * 0.03

#define BALL_SIZE 10
#define MIN_BALL_SPEED HEIGHT * 0.5

#define BLOCK_COLS 8
#define BLOCK_ROWS 14
#define BLOCK_X_OFFSET 2
#define BLOCK_Y_OFFSET 2
#define BLOCK_W (WIDTH - BLOCK_ROWS * BLOCK_X_OFFSET) / (BLOCK_ROWS)
#define BLOCK_H ((float)HEIGHT / 3) / (BLOCK_COLS)
#define BLOCK_W_GAP (float) (WIDTH - ((float)BLOCK_ROWS * BLOCK_W))
#define BLOCK_COLLISION_ANGLE 0.3f
#define RED_POINTS 7
#define PINK_POINTS 5
#define GREEN_POINTS 3
#define YELLOW_POINTS 1

#define HOTBAR_H 40
#define MAX_LIVES 3
#define MAX_SCORE (2 * BLOCK_ROWS) * (RED_POINTS + PINK_POINTS + GREEN_POINTS + YELLOW_POINTS)

const char* save_file = ".\\save_file.txt";
SDL_Color off_black = {33, 33, 33, 255};
SDL_Color black = {10, 10, 10, 255};
SDL_Color yellow = {187, 165, 59};
SDL_Color green = {99, 141, 91, 255};
SDL_Color pink = {179, 100, 138};
SDL_Color red = {154, 78, 78};
SDL_Color grey = {195, 195, 195, 255};
SDL_Color white = {220, 220, 220, 255};
char* font_path = ".\\fonts\\FiraCode-Bold.ttf";

typedef enum {
    LIVES=0,
    POINTS,
    TIME,
    HIGH_SCORE,
    MAX_UITypes
} UIType;


typedef enum {
    RESET_ROUND,
    GAME_OVER,
    IN_PLAY,
} PlayStatus;

typedef struct {
    int minutes;
    int seconds;
    time_t previous_time;
} Timer;

typedef struct {
    SDL_FRect shape;
    SDL_Color colour;
    float velocity;
    float move_speed;
    bool move_left;
    bool move_right;
    bool game_started;
} Player;

typedef struct {
    SDL_FRect shape;
    float vel_x;
    float vel_y;
    float move_speed;
} Ball;

typedef struct {
    SDL_FRect shape;
    bool alive;
    SDL_Color color;
    int points;
} Block;

typedef struct {
    UIType id;
    SDL_Texture* texture;
    float x;
    float y;
} TextElements;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    Player player;
    Ball ball;
    PlayStatus status;
    Block blocks[BLOCK_COLS][BLOCK_ROWS];
    SDL_FRect hotbar;
    int points;
    int lives;
    int hiscore;
    TextElements ui_elements[MAX_UITypes];
    TTF_Font* font;
    int brick_count;
    Timer time;
} GameState;

GameState *game = NULL;

void populate_ui_textures(UIType i) {
    SDL_Surface* s;
    switch (i) {
        case LIVES: {
                char buffer[15];
                sprintf(buffer, "Lives: %d", game->lives);
                s = TTF_RenderText_Blended(game->font, buffer, 0, white);
                game->ui_elements[LIVES].x = 0;
                game->ui_elements[LIVES].y = 0;
                break;
            }
        case POINTS: {
                char buffer[15];
                sprintf(buffer, "Points: %d", game->points);
                s = TTF_RenderText_Blended(game->font, buffer, 0, white);
                game->ui_elements[POINTS].x = 0.2f*WIDTH;
                game->ui_elements[POINTS].y = 0;
                break;   
            }
        case TIME: {
                char buffer[10];
                sprintf(buffer, "%02d:%02d", game->time.minutes, game->time.seconds);
                if (game->time.minutes > 99) sprintf(buffer, "just stop");
                s = TTF_RenderText_Blended(game->font, buffer, 0, white);
                game->ui_elements[TIME].x = 0.5f*WIDTH;
                game->ui_elements[TIME].y = 0;
                break; 
            }
        case HIGH_SCORE: {
                char buffer[20];
                sprintf(buffer, "High Score: %03d", game->hiscore);
                s = TTF_RenderText_Blended(game->font, buffer, 0, white);
                game->ui_elements[HIGH_SCORE].x = 0.75 * WIDTH;
                game->ui_elements[HIGH_SCORE].y = 0;
                break;
            }  
        default:
            return;
    }

    if (s == NULL) {
        printf("Error_surface_text\n");
        return;
    }

    if (game->ui_elements[i].texture) {
        SDL_DestroyTexture(game->ui_elements[i].texture);
    }
    
    game->ui_elements[i].texture = SDL_CreateTextureFromSurface(game->renderer, s);
    SDL_DestroySurface(s);
    return;    
}


int load_save_file(void) {
    FILE* fp = fopen(save_file, "a+");
    if (!fp) {
        printf("Error loading file_load..\n");
        return -1;
    }
    int hi_score;
    int ret_code = fscanf(fp, "%d\n", &hi_score);
    if (ret_code < 1) {
        hi_score = 0;
        rewind(fp);
        fprintf(fp, "%d", hi_score);
    }
    if (hi_score > MAX_SCORE || hi_score < 0) {
        printf("invalid high score. resetting to 0\n");
        hi_score = 0;
    }
    
    fclose(fp); 
    
    return hi_score;
}

void update_hiscore(void) {
    if (game->points < game->hiscore) {
        return;
    }
    if (game->points > MAX_SCORE) {
        printf("Error - high score too high\n");
        game->points =  MAX_SCORE;
        return;
    }

    if (game->points < 0) {
        printf("Error - high score too low\n");
        game->points = 0;
        return;
    }
    
    FILE* fp = fopen(save_file, "w");
    if (!fp) {
        printf("Error loading file_update...\n");
        return;
    }
    fprintf(fp,"%d", game->points);
    fclose(fp);
    return;
}

bool gamestate_create() {
    game = malloc(sizeof(GameState));

    game->window = SDL_CreateWindow("20g_breakout", WIDTH, HEIGHT, 0);
    if (game->window == NULL) {
        printf("Error_window: %s\n", SDL_GetError());
        return false;
    }
    game->renderer = SDL_CreateRenderer(game->window, NULL);
    if (game->renderer == NULL) {
        printf("Error_renderer: %s\n", SDL_GetError());
        return false;
    }

    game->font = TTF_OpenFont(font_path, 16);
    if (game->font == NULL) {
        printf("Error_font: %s\n", SDL_GetError());
        return false;
    }

    game->hiscore = load_save_file();

    game->time = (Timer) {.minutes = 0, .seconds = 0, .previous_time = 0};

    game->brick_count = BLOCK_COLS * BLOCK_ROWS;

    game->hotbar = (SDL_FRect) {.x = 0, .y = 0, .w = WIDTH, .h = HOTBAR_H};

    game->status = RESET_ROUND;

    game->points = 0;
    game->lives = MAX_LIVES;
    
    game->player = (Player) {
        .shape = (SDL_FRect) {.x = (WIDTH * 0.5f) - (PADDLE_W * 0.5f), .y = HEIGHT - (2*PADDLE_H), .w = PADDLE_W, .h = PADDLE_H},
        .move_speed = WIDTH * 0.5f,
        .move_left = false,
        .move_right = false,
        .colour = grey,
        .game_started = false,
    };
    
    game->ball = (Ball) {
        .shape = (SDL_FRect) {
            .x = (game->player.shape.x + (0.5f*game->player.shape.w)),
            .y = (game->player.shape.y - BALL_SIZE),
            .w = BALL_SIZE, .h = BALL_SIZE
        },
        .move_speed = MIN_BALL_SPEED,   
    };

    for (int y = 0; y < BLOCK_COLS; y++) {
        for (int x = 0; x < BLOCK_ROWS; x++) {
            game->blocks[y][x].alive = true;
            game->blocks[y][x].shape = (SDL_FRect) {
                .x = (0.5f * BLOCK_W_GAP + BLOCK_X_OFFSET) + (x * (float)BLOCK_W),
                .y = (HOTBAR_H + BLOCK_Y_OFFSET + ((y * BLOCK_H) + (y * BLOCK_Y_OFFSET))),
                .w = (float)BLOCK_W - BLOCK_X_OFFSET,
                .h = BLOCK_H
            };
            switch (y) {
                case 0:
                case 1:
                    game->blocks[y][x].color = red;
                    game->blocks[y][x].points = RED_POINTS;
                    break;
                case 2:
                case 3:
                    game->blocks[y][x].color = pink;
                    game->blocks[y][x].points = PINK_POINTS;
                    break;
                case 4:
                case 5:
                    game->blocks[y][x].color = green;
                    game->blocks[y][x].points = GREEN_POINTS;
                    break;
                case 6:
                case 7:
                    game->blocks[y][x].color = yellow;
                    game->blocks[y][x].points = YELLOW_POINTS;
                    break;
                default:
                    break;
            }
        }
    }
    
    for (int i = 0; i < MAX_UITypes; i++) {
        game->ui_elements[i].id = i;
        populate_ui_textures(i);
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
    
    for (int i = 0; i < MAX_UITypes; i++) {
        if (game->ui_elements[i].texture != NULL) {
            SDL_DestroyTexture(game->ui_elements[i].texture);
            game->ui_elements[i].texture = NULL;
        }
    }
    
    free(game);
    game = NULL;
}

bool get_collision(SDL_FRect a, SDL_FRect b) {

    if (a.x < b.x + b.w && a.x + a.w > b.x && a.y < b.y + b.h && a.y + a.h > b.y) {
        return true;
    }

    return false;
}



void update_game(double dt) {

    bool life_update = false;
    bool score_update = false;

    {
        //::update timer
        if (game->status != GAME_OVER && game->player.game_started) {
            time_t current_time = SDL_GetTicks();
        
            if (current_time - game->time.previous_time >= 1000) {
                game->time.seconds += 1;
                if (game->time.seconds >= 60) {
                    game->time.seconds = 0;
                    game->time.minutes += 1;
                }
            
                populate_ui_textures(TIME);
                game->time.previous_time = current_time;
            }    
        }
    }

    {
        //::update ball
        float new_x = 0.0f;
        float new_y = 0.0f;
        
        if (game->status == RESET_ROUND || game->status == GAME_OVER) {
            game->ball.shape.x = game->player.shape.x + (0.5f*game->player.shape.w);
            game->ball.shape.y = game->player.shape.y - BALL_SIZE;
            game->ball.vel_y = -1.0f;
            game->ball.vel_x = 0.0f;
        } else {
            if (game->ball.vel_x == 0.0f || game->ball.vel_x == -0.0f) game->ball.vel_x = 0.003f;
            
            float dynamic_move_x = (game->ball.move_speed + ((1.0f - SDL_fabsf(game->ball.vel_x)) * game->ball.move_speed));
            float dynamic_move_y = (game->ball.move_speed * (1.0f + (1.0f - SDL_fabs(game->ball.vel_x))));
             
            new_x = (game->ball.vel_x * dynamic_move_x * dt);
            new_y = (game->ball.vel_y * dynamic_move_y * dt);
            Ball collider = game->ball;
            collider.shape.x += new_x;
            collider.shape.y += new_y;
                
            if (collider.shape.x <= 0 || collider.shape.x + BALL_SIZE > WIDTH) {
                collider.vel_x *= -1;
                goto exit_collision;
            }
            
            if (collider.shape.y <= HOTBAR_H) {
                collider.vel_y *= -1;      
                goto exit_collision;
            }

            if (collider.shape.y > game->player.shape.y + (0.5* game->player.shape.h)) {
                game->lives -=1;
                life_update = true;
                game->status = RESET_ROUND;
                goto exit_collision;
            }
            
            if (get_collision(collider.shape, game->player.shape)) {
                float mid_collider = collider.shape.x + (0.5f * collider.shape.w);
                float mid_paddle = game->player.shape.x + (0.5f * game->player.shape.w);
                float end_paddle = game->player.shape.x + game->player.shape.w;
                float half_paddle_size = PADDLE_W * 0.5f;

                if (mid_collider > mid_paddle) {
                    float relative_pos = half_paddle_size  - (end_paddle - mid_collider);
                    collider.vel_x = relative_pos / 100.0f;
                } else if (collider.shape.x < game->player.shape.x + (0.5f * game->player.shape.w)) {
                    float relative_pos = half_paddle_size - (mid_collider - game->player.shape.x);
                    collider.vel_x = -1 * (relative_pos / 100.0f);
                } 
                collider.vel_y = -1;
                goto exit_collision;
            }

            for (Block *b = &game->blocks[0][0]; b < &game->blocks[0][0] + (BLOCK_ROWS * BLOCK_COLS); b++) {
                if (b->alive) {
                    if (get_collision(collider.shape, b->shape)) {
                        collider.vel_x = (collider.vel_x >= 0.0f) ? BLOCK_COLLISION_ANGLE : -BLOCK_COLLISION_ANGLE;
                        collider.vel_y *= -1;
                        b->alive = false;
                        game->brick_count -=1;
                        game->points += b->points;
                        score_update = true;
                        goto exit_collision;
                                                
                    }    
                }
            }
            exit_collision:           
            game->ball = collider; 
        }
    }
    
    { 
        //::update player 
        if (game->player.move_left && !game->player.move_right) game->player.velocity = -1;
        else if (game->player.move_right && !game->player.move_left) game->player.velocity = 1;
        else game->player.velocity = 0;
        
        float new_x = game->player.shape.x + (game->player.velocity * game->player.move_speed * dt);
        if (new_x < 0 || new_x + PADDLE_W > WIDTH) new_x = game->player.shape.x;
        game->player.shape.x = new_x;     
    }

    //::update state

    if (life_update) populate_ui_textures(LIVES);
    if (score_update) populate_ui_textures(POINTS);

    if (game->brick_count <= 0) {
        game->status = GAME_OVER;
        game->player.colour = green;
        update_hiscore();
    } else if (game->lives <= 0) {
        game->status = GAME_OVER;
        game->player.colour = red;
        update_hiscore();
    }
}

void draw_game() {
    SDL_SetRenderDrawColor(game->renderer, off_black.r, off_black.g, off_black.b, off_black.a);
    SDL_RenderClear(game->renderer);

    //::draw ui
    for (int i = 0; i < MAX_UITypes; i++) {
        if (game->ui_elements[i].texture != NULL) {
            SDL_FRect dest;
            SDL_GetTextureSize(game->ui_elements[i].texture, &dest.w, &dest.h);
            float remaining_h = HOTBAR_H - dest.h;
            dest.y = game->ui_elements[i].y + (0.5f * remaining_h);
            dest.x = game->ui_elements[i].x + (0.5f * BLOCK_W_GAP + BLOCK_X_OFFSET);
            SDL_RenderTexture(game->renderer, game->ui_elements[i].texture, NULL, &dest);
        }
    }

    //::draw grid
    for (Block *b = &game->blocks[0][0]; b < &game->blocks[0][0] + (BLOCK_COLS * BLOCK_ROWS); b++) {
        if (b->alive) {
            SDL_SetRenderDrawColor(game->renderer, b->color.r, b->color.g, b->color.b, b->color.a);
            SDL_RenderFillRect(game->renderer, &b->shape);
        }
    }    
    //::draw ball
    SDL_SetRenderDrawColor(game->renderer, white.r, white.g, white.b, white.a);
    SDL_RenderFillRect(game->renderer, &game->ball.shape);

    //::draw player
    Player p = game->player;
    SDL_SetRenderDrawColor(game->renderer, p.colour.r, p.colour.g, p.colour.b, p.colour.a);
    SDL_RenderFillRect(game->renderer, &p.shape);

    SDL_RenderPresent(game->renderer);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        printf("Error_init: %s\n", SDL_GetError());
        return -1;
    }

    if (!TTF_Init()) {
        printf("Error_ttf_init: %s\n", SDL_GetError());
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
        
       
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
                break;
            }
            if (e.type == SDL_EVENT_KEY_DOWN) {
                switch (e.key.key) {
                    case SDLK_A:
                    case SDLK_LEFT:
                        if (game->status != GAME_OVER) game->player.move_left = true;
                        break;
                    case SDLK_D:
                    case SDLK_RIGHT:
                        if (game->status != GAME_OVER) game->player.move_right = true;
                        break;
                    case SDLK_SPACE:
                        if (game->status == RESET_ROUND) game->status = IN_PLAY; game->player.game_started = true;
                }
            }
            if (e.type == SDL_EVENT_KEY_UP && game->status != GAME_OVER) {
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
            if (game->status != GAME_OVER) update_game(TARGET_DT);
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
