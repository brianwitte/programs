#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define PLAYER_SIZE 20
#define BLOCK_SIZE 40
#define GRAVITY 0.5f
#define JUMP_STRENGTH -12.0f
#define MOVE_SPEED 5.0f

/**
 * struct Player - represents the player character
 * @x: horizontal position in pixels
 * @y: vertical position in pixels
 * @vx: horizontal velocity
 * @vy: vertical velocity
 * @on_ground: true if player is standing on something solid
 */
typedef struct {
    float x, y;
    float vx, vy;
    bool on_ground;
} Player;

/**
 * struct Block - represents a solid platform block
 * @x: horizontal position in pixels
 * @y: vertical position in pixels
 */
typedef struct {
    int x, y;
} Block;

/**
 * level - the world geometry
 *
 * Simple hardcoded level layout. Don't like it? Edit it yourself.
 */
Block level[] = {
    {0, 560}, {40, 560}, {80, 560}, {120, 560}, {160, 560}, {200, 560}, {240, 560}, {280, 560},
    {320, 560}, {360, 560}, {400, 560}, {440, 560}, {480, 560}, {520, 560}, {560, 560}, {600, 560},
    {640, 560}, {680, 560}, {720, 560}, {760, 560}, /* Ground floor */
    {200, 480}, {240, 480}, {280, 480}, /* Platform 1 */
    {400, 400}, {440, 400}, /* Platform 2 */
    {600, 320}, {640, 320}, {680, 320}, /* Platform 3 */
    {100, 360}, {140, 360}, /* Platform 4 */
    {500, 240}, {540, 240}, {580, 240} /* Top platform */
};

int level_size = sizeof(level) / sizeof(Block);

/**
 * check_collision() - AABB collision detection
 * @px: player x position
 * @py: player y position
 * @bx: block x position
 * @by: block y position
 *
 * Returns: true if rectangles overlap, false otherwise
 *
 * Basic axis-aligned bounding box test. If this confuses you,
 * maybe stick to web development.
 */
bool check_collision(float px, float py, int bx, int by) {
    return px < bx + BLOCK_SIZE &&
           px + PLAYER_SIZE > bx &&
           py < by + BLOCK_SIZE &&
           py + PLAYER_SIZE > by;
}

/**
 * update_player() - handle player physics and input
 * @player: pointer to player struct
 * @keys: SDL keyboard state array
 *
 * Updates player position based on input and physics.
 * Handles collision detection because apparently we can't have nice things.
 */
void update_player(Player *player, const Uint8 *keys) {
    /* Horizontal movement - because walking is hard */
    player->vx = 0;
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) {
        player->vx = -MOVE_SPEED;
    }
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) {
        player->vx = MOVE_SPEED;
    }
    
    /* Jumping - defying gravity since 1981 */
    if ((keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) && player->on_ground) {
        player->vy = JUMP_STRENGTH;
        player->on_ground = false;
    }
    
    /* Apply gravity - Newton was right, deal with it */
    player->vy += GRAVITY;
    
    /**
     * Horizontal collision detection
     * 
     * We check X movement separately because apparently
     * doing both axes at once is too mainstream.
     */
    float new_x = player->x + player->vx;
    bool x_collision = false;
    
    for (int i = 0; i < level_size; i++) {
        if (check_collision(new_x, player->y, level[i].x, level[i].y)) {
            x_collision = true;
            break; /* Found a wall, abort mission */
        }
    }
    
    if (!x_collision && new_x >= 0 && new_x <= WINDOW_WIDTH - PLAYER_SIZE) {
        player->x = new_x;
    }
    
    /**
     * Vertical collision detection
     * 
     * Here's where the magic happens. Or the bugs.
     * Probably the bugs.
     */
    float new_y = player->y + player->vy;
    bool y_collision = false;
    player->on_ground = false; /* Guilty until proven innocent */
    
    for (int i = 0; i < level_size; i++) {
        if (check_collision(player->x, new_y, level[i].x, level[i].y)) {
            if (player->vy > 0) { /* Falling down - ouch */
                player->y = level[i].y - PLAYER_SIZE;
                player->vy = 0;
                player->on_ground = true;
            } else { /* Jumping up - bonk */
                player->y = level[i].y + BLOCK_SIZE;
                player->vy = 0;
            }
            y_collision = true;
            break; /* Physics applied, move along */
        }
    }
    
    if (!y_collision) {
        player->y = new_y;
    }
    
    /* Keep player in bounds - no escape allowed */
    if (player->y > WINDOW_HEIGHT) {
        player->x = 100; /* Back to square one, literally */
        player->y = 400;
        player->vy = 0;
    }
}

/**
 * main() - program entry point
 * @argc: argument count (ignored because we're rebels)
 * @argv: argument vector (see above)
 *
 * Initializes SDL, creates window, runs game loop.
 * Standard boilerplate that nobody reads anyway.
 *
 * Returns: 0 on success, 1 on failure (like most of life)
 */
int main(int argc, char *argv[]) {
    /* SDL initialization - pray to the graphics gods */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1; /* Exit stage left */
    }
    
    /**
     * Create window
     * 
     * If this fails, your system is probably cursed.
     * Have you tried turning it off and on again?
     */
    SDL_Window *window = SDL_CreateWindow("Minimal Platformer",
                                        SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_SHOWN);
    
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    
    /**
     * Create renderer
     * 
     * Hardware acceleration requested. If you don't have it,
     * welcome to 1995.
     */
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    /* Initialize player - spawn at a reasonable location */
    Player player = {100, 400, 0, 0, false};
    bool running = true;
    SDL_Event event;
    
    /**
     * Main game loop
     * 
     * The beating heart of our masterpiece.
     * Exit with X button or suffer forever.
     */
    while (running) {
        /* Event handling - because users insist on interacting */
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false; /* Finally, sweet release */
            }
        }
        
        /* Update game state - where the fun happens */
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        update_player(&player, keys);
        
        /* Clear screen with white background - so clean, so pure */
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        /* Draw level blocks (black) - the foundation of our world */
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        for (int i = 0; i < level_size; i++) {
            SDL_Rect block_rect = {level[i].x, level[i].y, BLOCK_SIZE, BLOCK_SIZE};
            SDL_RenderFillRect(renderer, &block_rect);
        }
        
        /* Draw player (red) - the star of the show */
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect player_rect = {(int)player.x, (int)player.y, PLAYER_SIZE, PLAYER_SIZE};
        SDL_RenderFillRect(renderer, &player_rect);
        
        SDL_RenderPresent(renderer); /* Show our artistic vision */
        SDL_Delay(16); /* ~60 FPS - any faster would be showing off */
    }
    
    /**
     * Cleanup
     * 
     * Being responsible adults and cleaning up our mess.
     * Unlike most game developers.
     */
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0; /* Mission accomplished */
}
