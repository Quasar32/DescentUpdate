#ifndef TILE_DATA_H 
#define TILE_DATA_H

#include <stdint.h>
#include <stdlib.h>

#define TF_HORZ   0x01 
#define TF_VERT   0x02 
#define TF_SOLID  0x04 
#define TF_ALPHA  0x08
#define TF_SPRITE 0x10

typedef enum tile {
    TD_NONE,
    TD_WOOD,
    TD_GHOST,
    TD_GLASS_HORZ,
    TD_GLASS_VERT
} tile;

typedef struct tile_data {
    uint8_t TexI;
    uint8_t Flags;
} tile_data;

tile_data GetTileData(tile Tile);
#endif
