#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdbool.h>
#include <stdint.h>

#include "descent.h"

void RenderWorld(game_state *GS);
void FillColor(color Texture[TEX_LENGTH][TEX_LENGTH], color Color);

static inline bool IsInTileMap(size_t Row, size_t Col) {
    return Row < TILE_HEIGHT && Col < TILE_WIDTH;
}

#endif
