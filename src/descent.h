#ifndef DESCENT_HPP 
#define DESCENT_HPP

#include <stdint.h>
#include <windows.h>
#include "color.h"
#include "vec2.h"
#include "worker.h"

#define DIB_WIDTH 640 
#define DIB_HEIGHT 480

#define TEX_LENGTH 16 

#define TILE_WIDTH 20
#define TILE_HEIGHT 20 

#define TILE_WIDTH 20
#define TILE_HEIGHT 20

#define SPR_CAP 256

typedef enum game_buttons {
    BT_LEFT = 0,
    BT_UP = 1 ,
    BT_RIGHT = 2,
    BT_DOWN = 3
} game_buttons;

#define COUNTOF_BT 4

typedef struct sprite {
    vec2 Pos;
    int32_t Texture;
} sprite;

typedef struct game_state {
    /*OtherRendering*/
    color Pixels[DIB_HEIGHT][DIB_WIDTH];
    color TexData[SPR_CAP][TEX_LENGTH][TEX_LENGTH];
    uint8_t TileMap[TILE_HEIGHT][TILE_WIDTH];

    /*Sprite*/
    uint32_t SpriteCount;
    sprite Sprites[SPR_CAP];

    float ZBuffer[DIB_WIDTH];
    float SpriteSquareDis[SPR_CAP];

    /*Camera*/
    vec2 Pos; 
    vec2 Dir; 
    vec2 Plane; 

    /*Other*/
    uint32_t Buttons[COUNTOF_BT];

    float FrameDelta;
    float TotalTime;

    worker Workers[4];
} game_state;

void CreateGameState(game_state *GS);
void UpdateGameState(game_state *GS);

#endif
