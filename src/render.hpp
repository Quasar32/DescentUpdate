#ifndef RENDER_HPP
#define RENDER_HPP

#include <stdint.h>

#include "descent.hpp"

void RenderWorld(GameState *GS);
void FillColor(uint32_t Texture[TEX_LENGTH][TEX_LENGTH], uint32_t Color);

inline bool IsInTileMap(size_t Row, size_t Col) {
    return Row < TILE_HEIGHT && Col < TILE_WIDTH;
}



#endif
