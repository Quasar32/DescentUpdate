#ifndef DESCENT_H 
#define DESCENT_H

#include <stdint.h>

/**
 * bool - is typedef to int because it is native type of a boolean expression 
 */
typedef int bool;

/*
 * DIB_WIDTH, DIB_HEIGHT - Dimensions of main pixel buffer
 */
#define DIB_WIDTH 640 
#define DIB_HEIGHT 480 

/**
 * TILE_LENGTH - Length of square tile 
 */
#define TILE_LENGTH 32

/**
 * TILE_MAP_WIDTH, TILE_MAP_HEIGHT - Dimensions of tile map
 */
#define TILE_MAP_WIDTH 20
#define TILE_MAP_HEIGHT 20 

/**
 * enum game_buttons - Meaning of each index in game_state.Buttons
 */
enum game_buttons {
    BT_LEFT = 0,
    BT_UP = 1 ,
    BT_RIGHT = 2,
    BT_DOWN = 3
};
#define COUNTOF_BT 4

/**
 * struct vec2 - 2D Vector of floats 
 * @X: X-Value
 * @Y: Y-Value
 */
struct vec2 {
    float X;
    float Y;
};

/**
 * struct game_state - Holds entire platform independent game_state 
 * @IsInit: GameState has been initialized
 * @Pixels: Main pixels buffer
 * @TileMap: Holds tiles
 * @Buttons: Holds number of frame buttons have been held 
 * @Pos: Position of camera 
 * @Dir: Direction of camera 
 * @Plane: Plane of camera
 */
struct game_state {
    bool IsInit;
    uint32_t Pixels[DIB_HEIGHT][DIB_WIDTH];
    uint8_t TileMap[TILE_MAP_HEIGHT][TILE_MAP_WIDTH];
    
    uint32_t Buttons[COUNTOF_BT];

    struct vec2 Pos; 
    struct vec2 Dir; 
    struct vec2 Plane; 
};


/**
 * game_update - Type of GameUpdate 
 */
typedef void game_update(struct game_state *);

#endif
