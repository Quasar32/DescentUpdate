#ifndef DESCENT_H 
#define DESCENT_H

#include <stdint.h>

typedef int bool;

#include "descent_platform.h"

#define DIB_WIDTH 640 
#define DIB_HEIGHT 480

#define TEX_LENGTH 32

#define TILE_WIDTH 20
#define TILE_HEIGHT 20 

#define SPR_CAP 256

enum game_buttons {
    BT_LEFT = 0,
    BT_UP = 1 ,
    BT_RIGHT = 2,
    BT_DOWN = 3
};
#define COUNTOF_BT 4

struct vec2 {
    float X;
    float Y;
};

struct sprite {
    struct vec2 Pos;
    int32_t Texture;
};

struct game_state {
    bool IsInit;
    /*OtherRendering*/
    uint32_t Pixels[DIB_HEIGHT][DIB_WIDTH];
    uint8_t TileMap[TILE_HEIGHT][TILE_WIDTH];
    uint32_t TexData[SPR_CAP][TEX_LENGTH][TEX_LENGTH];

    /*Sprite*/
    uint32_t SpriteCount;
    struct sprite Sprites[SPR_CAP];

    float ZBuffer[DIB_WIDTH];
    float SpriteSquareDis[SPR_CAP];
    
    /*Camera*/
    struct vec2 Pos; 
    struct vec2 Dir; 
    struct vec2 Plane; 

    /*Other*/
    uint32_t Buttons[COUNTOF_BT];

    float FrameDelta;

};

typedef void game_update(struct game_state *, const struct platform *);

#endif
