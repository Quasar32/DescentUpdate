#include "tile_data.h"

static tile_data TileData[] = {
    [TD_NONE] = {
        .Flags = TF_ALPHA
    },
    [TD_WOOD] = {
        .TexI = 2,
        .Flags = TF_HORZ | TF_VERT | TF_SOLID
    },
    [TD_GLASS_HORZ] = {
        .TexI = 4,
        .Flags = TF_HORZ | TF_SOLID | TF_ALPHA
    },
    [TD_GLASS_VERT] = {
        .TexI = 4,
        .Flags = TF_VERT | TF_SOLID | TF_ALPHA
    }
};

tile_data GetTileData(tile Tile) {
    return TileData[Tile >= 0 && Tile < _countof(TileData) ? Tile : TD_NONE];
}
