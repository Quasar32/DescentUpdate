#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "descent.h"

#define SWAP(A, B) do {\
    __auto_type A_ = (A);\
    __auto_type B_ = (B);\
    __auto_type Tmp = *A;\
    *A_ = *B_;\
    *B_ = Tmp;\
} while(0)

static const struct platform *g_Platform;

static bool IsInTileMap(size_t Row, size_t Col) {
    return Row < TILE_HEIGHT && Col < TILE_WIDTH;
}

/*Scalar Operations*/
static float FloatMagnitude(float Value) {
    return Value < 0.0F ? -Value : Value;
} 

static int32_t MinInt32(int32_t A, int32_t B) {
    return A < B ? A : B;
}

static int32_t MaxInt32(int32_t A, int32_t B) {
    return A > B ? A : B;
}

static int32_t AbsInt32(int32_t A) {
    return A < 0 ? -A : A; 
}

/*Vector Operations*/
static struct vec2 AddVec2(struct vec2 A, struct vec2 B) {
    return (struct vec2) {A.X + B.X, A.Y + B.Y};
}

static struct vec2 SubVec2(struct vec2 A, struct vec2 B) {
    return (struct vec2) {A.X - B.X, A.Y - B.Y};
}

static struct vec2 MulVec2(struct vec2 Vec, float Val) {
    return (struct vec2) {Vec.X * Val, Vec.Y * Val};
} 

static float DotVec2(struct vec2 Vec) {
    return Vec.X * Vec.X + Vec.Y * Vec.Y;
}

static struct vec2 RevTruncVec2(struct vec2 Vec) {
    return (struct vec2) {fmodf(Vec.X, 1), fmodf(Vec.Y, 1)};
}

static float SquareDisVec2(struct vec2 A, struct vec2 B) {
    return DotVec2(SubVec2(A, B));
}

static float DetVec2(struct vec2 ColA, struct vec2 ColB) {
    return ColA.X * ColB.Y - ColA.Y * ColB.X; 
}

/*Rendering*/
static uint32_t HalfColor(uint32_t Color) {
    return (Color >> 1) & 0x007F7F7F;
}

static void RenderWorld(struct game_state *GS) {
    /*RenderFloorAndCeiling*/
    for(int32_t CeilY = DIB_HEIGHT / 2 + 1; CeilY < DIB_HEIGHT; CeilY++) {
        /*CalcZCompVals*/
        struct vec2 RayDir = SubVec2(GS->Dir, GS->Plane); 
        int32_t Horizon = CeilY - DIB_HEIGHT / 2;
        float PosZ = 0.5F * DIB_HEIGHT;
        float RowDis = PosZ / (float) Horizon;

        /*CalcCeilStep*/
        struct vec2 PlaneTwice = MulVec2(GS->Plane, 2); 
        float UnitRowDis = RowDis / (float) DIB_WIDTH;
        struct vec2 CeilStep = MulVec2(PlaneTwice, UnitRowDis);

        /*CalcInitCeil*/
        struct vec2 DeltaCeil = MulVec2(RayDir, RowDis); 
        struct vec2 Ceil = AddVec2(GS->Pos, DeltaCeil); 

        for(int X = 0; X < DIB_WIDTH; X++) {
            struct vec2 TexPos = MulVec2(RevTruncVec2(Ceil), TEX_LENGTH); 
            int32_t TexX = TexPos.X;
            int32_t TexY = TexPos.Y;
            
            Ceil = AddVec2(Ceil, CeilStep);
            GS->Pixels[CeilY][X] = HalfColor(GS->TexData[1][TexY][TexX]); 
            int32_t FloorY = DIB_HEIGHT - CeilY - 1;
            GS->Pixels[FloorY][X] = (GS->TexData[1][TexY][TexX]); 
        } 
    }

    /*RenderWalls*/
    for(int32_t X = 0; X < DIB_WIDTH; X++) {
        float CameraX = (float) (X << 1) / (float) DIB_WIDTH - 1;

        /*CalcXCompVars*/
        float RayDirX = GS->Dir.X + GS->Plane.X * CameraX;
        float DeltaDistX = FloatMagnitude(1.0F / RayDirX);
        int32_t TileX = (int32_t) GS->Pos.X;
        int32_t StepX;
        float SideDistX;
        if(RayDirX < 0.0F) {
            StepX = -1;
            SideDistX = (GS->Pos.X - TileX) * DeltaDistX;
        } else {
            StepX = 1;
            SideDistX = (TileX + 1.0 - GS->Pos.X) * DeltaDistX;
        }

        /*CalcYCompVars*/
        float RayDirY = GS->Dir.Y + GS->Plane.Y * CameraX;
        float DeltaDistY = FloatMagnitude(1.0F / RayDirY);
        int32_t TileY = (int32_t) GS->Pos.Y;
        int32_t StepY;
        float SideDistY;
        if(RayDirY < 0.0F) {
            StepY = -1;
            SideDistY = (GS->Pos.Y - TileY) * DeltaDistY;
        } else {
            StepY = 1;
            SideDistY = (TileY + 1.0 - GS->Pos.Y) * DeltaDistY;
        }

        /*LocateTileHit*/
        int32_t Side = 0;
        int32_t TexI = 0;
        while(
            IsInTileMap(TileY, TileX) &&
            (TexI = GS->TileMap[TileY][TileX]) == 0
        ) {
            if(SideDistX < SideDistY) {
                SideDistX += DeltaDistX;
                TileX += StepX;
                Side = 0;
            } else {
                SideDistY += DeltaDistY;
                TileY += StepY;
                Side = 1;
            }
        }

        /*FindWall*/
        float PerpWallDist = (
            Side == 0 ? 
                SideDistX - DeltaDistX : 
                SideDistY - DeltaDistY
        );
        float WallX = (
            Side == 0 ?
                GS->Pos.Y + PerpWallDist * RayDirY :
                GS->Pos.X + PerpWallDist * RayDirX
        );
        WallX -= floorf(WallX);

        /*FindTexture*/
        int32_t TexX = (int) (WallX * (float) TEX_LENGTH); 
        if(Side == 0 ? RayDirX > 0.0 : RayDirY < 0.0) {
            TexX = TEX_LENGTH - TexX - 1; 
        }

        /*RenderLine*/
        int32_t LineHeight = (int32_t) (DIB_HEIGHT / PerpWallDist);
        int32_t HalfHeight = LineHeight / 2;

        int32_t DrawCenter = DIB_HEIGHT / 2;
        int32_t DrawStart = MaxInt32(0, DrawCenter - HalfHeight);
        int32_t DrawEnd = MinInt32(DIB_HEIGHT - 1, DrawCenter + HalfHeight);

        float Step = (float) TEX_LENGTH / (DIB_HEIGHT / PerpWallDist);
        float TexPos = (float) (DrawStart - DrawCenter + HalfHeight) * Step; 

        for(int32_t Y = DrawStart; Y < DrawEnd; Y++) {
            int32_t TexY = (int32_t) TexPos & (TEX_LENGTH - 1);
            TexPos += Step;
            int32_t Color = GS->TexData[TexI][TexY][TexX]; 
            if(Side == 1) {
                Color = HalfColor(Color);
            }
            GS->Pixels[Y][X] = Color;
        }
        GS->ZBuffer[X] = PerpWallDist;
    }

    /*SortSprites*/
    for(int32_t I = 0; I < GS->SpriteCount; I++) {
        GS->SpriteSquareDis[I] = SquareDisVec2(GS->Pos, GS->Sprites[I].Pos);
        for(int32_t J = I; J > 0; J--) {
            if(GS->SpriteSquareDis[J] < GS->SpriteSquareDis[J - 1]) {
                break;
            }
            SWAP(&GS->Sprites[J], &GS->Sprites[J - 1]); 
            SWAP(&GS->SpriteSquareDis[J], &GS->SpriteSquareDis[J - 1]); 
        }
    }

    /*RenderSprites*/
    for(int32_t I = 0; I < GS->SpriteCount; I++) {
        struct vec2 SpriteDis = SubVec2(GS->Sprites[I].Pos, GS->Pos);
        float InvDet = 1.0F / DetVec2(GS->Plane, GS->Dir);
        float TransformX = InvDet * DetVec2(SpriteDis, GS->Dir);
        float TransformY = InvDet * DetVec2(GS->Plane, SpriteDis);

        int32_t SpriteScreenX = (int) (
            (float) DIB_WIDTH / 2.0F * 
            (1.0F + TransformX / TransformY)
        );

        int32_t UDiv = 1;
        int32_t VDiv = 1;
        float VMove = 0.0F;
        int32_t VMoveScreen = (int) (VMove / TransformY);

        /*CalcYCompVars*/
        int32_t SpriteHeight = AbsInt32((int32_t) (DIB_HEIGHT / TransformY)) / VDiv;
        int32_t DrawStartY = MaxInt32(
            0, 
            (DIB_HEIGHT - SpriteHeight) / 2 + VMoveScreen
        ); 
        int32_t DrawEndY = MinInt32(
            DIB_HEIGHT - 1, 
            (SpriteHeight + DIB_HEIGHT) / 2 + VMoveScreen
        ); 

        /*CalcXCompXVars*/
        int32_t SpriteWidth = AbsInt32((int32_t) (DIB_HEIGHT / TransformY)) / UDiv;
        int32_t DrawStartX = MaxInt32(0, SpriteScreenX - SpriteWidth / 2);
        int32_t DrawEndX = MinInt32(DIB_WIDTH, SpriteWidth / 2 + SpriteScreenX);

        for(int32_t X = DrawStartX; X < DrawEndX; X++) {
            int TexX = (
                256 * 
                (X + SpriteWidth / 2 - SpriteScreenX) * 
                TEX_LENGTH / 
                (SpriteWidth * 256) 
            ); 

            if(TransformY > 0.0F && TransformY < GS->ZBuffer[X]) {
                for(int Y = DrawStartY; Y < DrawEndY; Y++) {
                    int D = (
                        (Y - VMoveScreen) * 256 + 
                        (SpriteHeight - DIB_HEIGHT) * 128 
                    );
                    int32_t TexY = D * TEX_LENGTH / SpriteHeight / 256; 
                    uint32_t Color = GS->TexData[GS->Sprites[I].Texture][TexY][TexX];

                    if(Color >> 24 != 0) {
                        GS->Pixels[Y][X] = Color;
                    }
                }
            }
        }
    }
}

static struct vec2 RotateVec2(struct vec2 Vec, float Rot) {
    return (struct vec2) {
        .X = Vec.X * cosf(Rot) - Vec.Y * sinf(Rot), 
        .Y = Vec.X * sinf(Rot) + Vec.Y * cosf(Rot)
    };
}

static void MoveCamera(
    struct game_state *GS, 
    float Velocity
) {
    float FrameVelocity = Velocity * GS->FrameDelta;
    struct vec2 PosDelta = MulVec2(GS->Dir, FrameVelocity);
    assert(DotVec2(PosDelta) < 0.5F);

    struct vec2 NewPos = AddVec2(GS->Pos, PosDelta);
    int32_t TileX = (int32_t) NewPos.X;
    int32_t TileY = (int32_t) NewPos.Y;
    if(IsInTileMap(TileY, TileX) && GS->TileMap[TileY][TileX] == 0) {
        GS->Pos = NewPos;
    }
}

static void RotateCamera(
    struct game_state *GS, 
    float RotPerSec 
) {
    float Rot = RotPerSec * GS->FrameDelta;
    GS->Dir = RotateVec2(GS->Dir, Rot); 
    GS->Plane = RotateVec2(GS->Plane, Rot);
}

static bool ReadObject(struct file *File, void *Obj, int32_t ObjSize) {
    return g_Platform->ReadFile(File, Obj, ObjSize) == ObjSize; 
}

static bool ReadTexture(
    const char *Path,
    uint32_t Texture[static TEX_LENGTH][TEX_LENGTH]
) {
    bool Ret = 0;

    struct file *File = g_Platform->OpenFile(Path);
    if(!File) {
        return Ret; 
    }

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

    struct bitmap_header BitmapHeader;
    if(!ReadObject(File, &BitmapHeader, sizeof(BitmapHeader))) {
        goto out;
    }

    if( 
        /*CheckFileHeader*/
        memcmp(&BitmapHeader.Signature, "BM", 2) != 0 ||

        /*CheckInfoHeader*/
        BitmapHeader.HeaderSize != 40 ||
        BitmapHeader.Width != 32 ||
        BitmapHeader.Height != 32 || 
        BitmapHeader.Planes != 1 ||
        BitmapHeader.BitsPerPixel != 32 ||
        BitmapHeader.Compression != 3 /*Bitfield compression*/
    ) {
        goto out;
    }

    /*ReadData*/
    int32_t NewLoc = g_Platform->SeekFile(File, BitmapHeader.DataOffset); 
    if(NewLoc != BitmapHeader.DataOffset) {
        goto out;
    }

    int32_t TextureSize = TEX_LENGTH * TEX_LENGTH * 4;
    if(!ReadObject(File, Texture, TextureSize)) {
        goto out;
    }
    Ret = 1;

out:
    g_Platform->CloseFile(File);
    return Ret;
}

void GameUpdate(struct game_state *GS, const struct platform *Platform) { 
    g_Platform = Platform; 
    if(!GS->IsInit) {
        for(int32_t X = 0; X < TILE_WIDTH; X++) {
            GS->TileMap[0][X] = 30;    
            GS->TileMap[TILE_HEIGHT - 1][X] = 30;    
        }

        for(int32_t Y = 0; Y < TILE_HEIGHT; Y++) {
            GS->TileMap[Y][0] = 30;    
            GS->TileMap[Y][TILE_WIDTH - 1] = 30; 
        }

        GS->Dir = (struct vec2) {-1.0F, 0.0F};
        GS->Pos = (struct vec2) {5.0F, 5.0F};
        GS->Plane = (struct vec2) {0.0F, 0.5F};

        GS->IsInit = 1;
        ReadTexture("textures.bmp", GS->TexData[30]);
        ReadTexture("other.bmp", GS->TexData[1]);

        GS->SpriteCount = 3;
        GS->Sprites[0] = (struct sprite) {
            .Pos = {8.0F, 8.0F},
            .Texture = 1
        };
        GS->Sprites[1] = (struct sprite) {
            .Pos = {8.0F, 10.0F},
            .Texture = 30
        };
        GS->Sprites[2] = (struct sprite) {
            .Pos = {10.0F, 8.0F},
            .Texture = 1
        };
    }

    if(GS->FrameDelta > 0.05F) {
        GS->FrameDelta = 0.05F;
    }

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

