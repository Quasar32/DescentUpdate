#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "descent.h"
#include "render.h"

typedef struct __attribute__((packed)) bitmap_header {
    /*FileHeader*/
    uint16_t Signature;
    uint32_t FileSize;
    uint32_t Reserved;
    uint32_t DataOffset;

    /*InfoHeader*/
    uint32_t HeaderSize;
    uint32_t Width;
    uint32_t Height;
    uint16_t Planes;
    uint16_t BitsPerPixel;
    uint32_t Compression; 
    uint32_t ImageSize;
    uint32_t PixelsPerMeterX;
    uint32_t PixelsPerMeterY;
    uint32_t ColorsUsed;
    uint32_t ImportantColors;
} bitmap_header;

static void MoveCamera(game_state *GS, float Velocity) {
    float FrameVelocity = Velocity * GS->FrameDelta;
    vec2 PosDelta = MulVec2(GS->Dir, FrameVelocity);
    assert(DotVec2(PosDelta) < 0.5F);

    vec2 NewPos = AddVec2(GS->Pos, PosDelta);
    int32_t TileX = (int32_t) NewPos.X;
    int32_t TileY = (int32_t) NewPos.Y;
    if(IsInTileMap(TileY, TileX)) {
        tile_data TileData = GetTileData(GS->TileMap[TileY][TileX]);
        if(!(TileData.Flags & TF_SOLID)) { 
            GS->Pos = NewPos;
        }
    }
}

static void RotateCamera(game_state *GS, float RotPerSec) {
    float Rot = RotPerSec * GS->FrameDelta;
    GS->Dir = RotateVec2(GS->Dir, Rot); 
    GS->Plane = RotateVec2(GS->Plane, Rot);
}

static bool ReadObject(HANDLE File, LPVOID Obj, DWORD ObjSize) {
    DWORD BytesRead;
    return ReadFile(File, Obj, ObjSize, &BytesRead, NULL) && BytesRead == ObjSize; 
}

static bool IsValidBitmapHeader(const bitmap_header *BitmapHeader) {
    return (
        /*CheckFileHeader*/
        memcmp(&BitmapHeader->Signature, "BM", 2) == 0 &&

        /*CheckInfoHeader*/
        BitmapHeader->HeaderSize == 40 &&
        BitmapHeader->Width == TEX_LENGTH &&
        BitmapHeader->Height == TEX_LENGTH && 
        BitmapHeader->Planes == 1 &&
        BitmapHeader->BitsPerPixel == 32 &&
        BitmapHeader->Compression == 3 /*Bitfield compression*/
    );
}

#define SIZEOF_TEX (TEX_LENGTH * TEX_LENGTH * 4)

static bool ReadTexture(const char *Path, color Texture[TEX_LENGTH][TEX_LENGTH]) {
    bool Success = false;
    HANDLE FileHandle = CreateFile(
        Path,
        GENERIC_READ,
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        0, 
        NULL
    );
    if(FileHandle == INVALID_HANDLE_VALUE) goto out;

    bitmap_header BmHeader;
    Success = (
        ReadObject(FileHandle, &BmHeader, sizeof(BmHeader)) &&
        IsValidBitmapHeader(&BmHeader) &&
        SetFilePointer(FileHandle, BmHeader.DataOffset, NULL, FILE_BEGIN) == BmHeader.DataOffset &&
        ReadObject(FileHandle, Texture, SIZEOF_TEX)
    );
    CloseHandle(FileHandle);

out:
    if(!Success) FillColor(Texture, OpaqueColor(0xFF, 0x00, 0xFF));
    return Success;
}

void CreateGameState(game_state *GS) {
    for(size_t I = 0; I < _countof(GS->Workers); I++) {
        CreateWorker(&GS->Workers[I]);
    }

    for(int32_t X = 0; X < TILE_WIDTH; X++) {
        GS->TileMap[0][X] = TD_WOOD;
        GS->TileMap[TILE_HEIGHT - 1][X] = TD_WOOD; 
    }

    for(int32_t Y = 0; Y < TILE_HEIGHT; Y++) {
        GS->TileMap[Y][0] = TD_WOOD; 
        GS->TileMap[Y][TILE_WIDTH - 1] = TD_WOOD; 
    }

    for(int Y = 4; Y < 15; Y++) {
        GS->TileMap[Y][6] = TD_GLASS_HORZ;
    }

    GS->Dir = (vec2) {-1.0F, 0.0F};
    GS->Pos = (vec2) {5.0F, 5.0F};
    GS->Plane = (vec2) {0.0F, 0.5F};

    ReadTexture("../tex/tex01.bmp", GS->TexData[1]);
    ReadTexture("../tex/tex02.bmp", GS->TexData[2]);
    ReadTexture("../tex/tex03.bmp", GS->TexData[3]);
    ReadTexture("../tex/tex04.bmp", GS->TexData[4]);

    GS->SpriteCount = 3;
    GS->Sprites[0] = (sprite) {
        .Pos = {8.0F, 8.0F},
        .Tile = TD_GHOST
    };
    GS->Sprites[1] = (sprite) {
        .Pos = {8.0F, 10.0F},
        .Tile = TD_GHOST
    };
    GS->Sprites[2] = (sprite) {
        .Pos = {10.0F, 8.0F},
        .Tile = TD_GHOST
    };
}

void UpdateGameState(game_state *GS) { 
    if(GS->FrameDelta > 0.05F) {
        GS->FrameDelta = 0.05F;
    }
    GS->TotalTime += GS->FrameDelta;

    if(GS->Buttons[BT_LEFT]) {
        RotateCamera(GS, 2.0F);
    }
    if(GS->Buttons[BT_UP]) {
        MoveCamera(GS, 3.0F); 
    }
    if(GS->Buttons[BT_RIGHT]) {
        RotateCamera(GS, -2.0F);
    }
    if(GS->Buttons[BT_DOWN]) {
        MoveCamera(GS, -3.0F); 
    }

    RenderWorld(GS);
}
