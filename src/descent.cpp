#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "descent.hpp"
#include "scope.hpp"
#include "render.hpp"

static void MoveCamera(GameState *GS, float Velocity) {
    float FrameVelocity = Velocity * GS->FrameDelta;
    Vec2 PosDelta = GS->Dir * FrameVelocity;
    assert(DotVec2(PosDelta) < 0.5F);

    Vec2 NewPos = GS->Pos + PosDelta;
    int32_t TileX = (int32_t) NewPos.X;
    int32_t TileY = (int32_t) NewPos.Y;
    if(IsInTileMap(TileY, TileX) && GS->TileMap[TileY][TileX] == 0) {
        GS->Pos = NewPos;
    }
}

static void RotateCamera(GameState *GS, float RotPerSec) {
    float Rot = RotPerSec * GS->FrameDelta;
    GS->Dir = RotateVec2(GS->Dir, Rot); 
    GS->Plane = RotateVec2(GS->Plane, Rot);
}

static bool ReadObject(HANDLE File, LPVOID Obj, DWORD ObjSize) {
    DWORD BytesRead;
    return ReadFile(File, Obj, ObjSize, &BytesRead, NULL) && BytesRead == ObjSize; 
}

static bool ReadTexture(const char *Path, uint32_t Texture[TEX_LENGTH][TEX_LENGTH]) {
    bool Ret = false;

    HANDLE FileHandle = CreateFile(
        Path,
        GENERIC_READ,
        FILE_SHARE_READ, 
        NULL, 
        OPEN_EXISTING, 
        0, 
        NULL
    );
    if(FileHandle == INVALID_HANDLE_VALUE) {
        return Ret; 
    }
    auto FileScope = ScopeCreate([&]() {
        CloseHandle(FileHandle);
        if(!Ret) {
            FillColor(Texture, 0xFFFF00FF);
        }
    }); 

    /*CheckHeader*/
    struct __attribute__((packed)) bitmap_header {
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
    };

    bitmap_header BitmapHeader;
    if(!ReadObject(FileHandle, &BitmapHeader, sizeof(BitmapHeader))) {
        return Ret;
    }

    if( 
        /*CheckFileHeader*/
        memcmp(&BitmapHeader.Signature, "BM", 2) != 0 ||

        /*CheckInfoHeader*/
        BitmapHeader.HeaderSize != 40 ||
        BitmapHeader.Width != TEX_LENGTH ||
        BitmapHeader.Height != TEX_LENGTH || 
        BitmapHeader.Planes != 1 ||
        BitmapHeader.BitsPerPixel != 32 ||
        BitmapHeader.Compression != 3 /*Bitfield compression*/
    ) {
        return Ret;
    }

    /*ReadData*/
    DWORD NewLoc = SetFilePointer(
        FileHandle, 
        BitmapHeader.DataOffset, 
        NULL, 
        FILE_BEGIN
    );
    if(
        NewLoc == INVALID_SET_FILE_POINTER || 
        NewLoc != BitmapHeader.DataOffset
    ) {
        return Ret;
    }

    int32_t TextureSize = TEX_LENGTH * TEX_LENGTH * 4;
    if(!ReadObject(FileHandle, Texture, TextureSize)) {
        return Ret;
    }
    Ret = 1;

    return Ret;
}

GameState::GameState() {
    for(int32_t X = 0; X < TILE_WIDTH; X++) {
        TileMap[0][X] = 2;
        TileMap[TILE_HEIGHT - 1][X] = 2; 
    }

    for(int32_t Y = 0; Y < TILE_HEIGHT; Y++) {
        TileMap[Y][0] = 2; 
        TileMap[Y][TILE_WIDTH - 1] = 2; 
    }

    Dir = (Vec2) {-1.0F, 0.0F};
    Pos = (Vec2) {5.0F, 5.0F};
    Plane = (Vec2) {0.0F, 0.5F};

    ReadTexture("../tex/tex01.bmp", TexData[1]);
    ReadTexture("../tex/tex02.bmp", TexData[2]);
    ReadTexture("../tex/tex03.bmp", TexData[3]);

    SpriteCount = 3;
    Sprites[0] = (Sprite) {
        .Pos = {8.0F, 8.0F},
        .Texture = 3
    };
    Sprites[1] = (Sprite) {
        .Pos = {8.0F, 10.0F},
        .Texture = 3
    };
    Sprites[2] = (Sprite) {
        .Pos = {10.0F, 8.0F},
        .Texture = 3
    };
}

void GameState::Update() { 
    if(FrameDelta > 0.05F) {
        FrameDelta = 0.05F;
    }
    TotalTime += FrameDelta;

    if(Buttons[BT_LEFT]) {
        RotateCamera(this, 2.0F);
    }
    if(Buttons[BT_UP]) {
        MoveCamera(this, 3.0F); 
    }
    if(Buttons[BT_RIGHT]) {
        RotateCamera(this, -2.0F);
    }
    if(Buttons[BT_DOWN]) {
        MoveCamera(this, -3.0F); 
    }

    RenderWorld(this);
}
