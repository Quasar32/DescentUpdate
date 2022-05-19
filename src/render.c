#include "render.h"
#include "scalar.h"
#include "vec2.h"
#include "tile_data.h"

#define MAX_TILE_HITS 32 

#define SWAP(A, B) do {\
    __auto_type A_ = (A);\
    __auto_type B_ = (B);\
    __auto_type Tmp = *A_;\
    *A_ = *B_;\
    *B_ = Tmp;\
} while(0)

typedef struct sprite_render_info {
    float TransformY;
    int32_t SpriteScreenX;
    int32_t SpriteWidth; 
    int32_t SpriteHeight;
    float FogEffect;

    int32_t DrawStartX;
    int32_t DrawEndX;
    int32_t DrawStartY;
    int32_t DrawEndY;
    int32_t VMoveScreen;
} sprite_render_info;

typedef struct render_decks_data {
    game_state *GS;
    int StartY;
    int EndY;
} render_decks_data;

typedef struct render_facing_data {
    game_state *GS;
    int StartX;
    int EndX;
    sprite_render_info *SpriteRenderInfos;
} render_facing_data;

typedef struct tile_hit {
    float PerpWallDist;
    tile_data TileData;
    bool Side;
    int32_t SpriteI;
} tile_hit; 

void FillColor(color Texture[static TEX_LENGTH][TEX_LENGTH], color Color) {
    for(int Y = 0; Y < TEX_LENGTH; Y++) {
        for(int X = 0; X < TEX_LENGTH; X++) {
            Texture[Y][X] = Color;
        }
    } 
}

static float CalcFogEffect(float Dis) {
    return Dis < 5.0F ? 1.0F - Dis / 5.0F : 0.0F;
}

static void SortSprites(game_state *GS) {
    float SpriteSquareDis[SPR_CAP];
    for(uint32_t I = 0; I < GS->SpriteCount; I++) {
        SpriteSquareDis[I] = SquareDisVec2(GS->Pos, GS->Sprites[I].Pos);
        for(int32_t J = I; J > 0; J--) {
            if(SpriteSquareDis[J] < SpriteSquareDis[J - 1]) {
                break;
            }
            SWAP(&GS->Sprites[J], &GS->Sprites[J - 1]); 
            SWAP(&SpriteSquareDis[J], &SpriteSquareDis[J - 1]); 
        }
    }
} 

static void ComputeRenderSpriteInfos(
    game_state *GS, 
    sprite_render_info SpriteRenderInfos[SPR_CAP]
) {
    int32_t BobCycle = (int32_t) (GS->TotalTime * 16) % 16;
    int32_t Bob = BobCycle < 8 ? BobCycle : 16 - BobCycle; 

    for(uint32_t I = 0; I < GS->SpriteCount; I++) {
        vec2 SpriteDis = SubVec2(GS->Sprites[I].Pos, GS->Pos);
        float InvDet = 1.0F / DetVec2(GS->Plane, GS->Dir);
        float TransformX = InvDet * DetVec2(SpriteDis, GS->Dir);
        float TransformY = InvDet * DetVec2(GS->Plane, SpriteDis);

        int32_t SpriteScreenX = (int) (
            (float) DIB_WIDTH / 2.0F * 
            (1.0F + TransformX / TransformY)
        );

        int32_t UDiv = 1;
        int32_t VDiv = 1;
        float VMove = Bob;
        int32_t VMoveScreen = (int) (VMove / TransformY);

        /*UseFogEffect*/
        float PerpDist = TransformY;
        if(PerpDist > 5.0F) {
            continue;
        }
        float FogEffect = CalcFogEffect(TransformY);

        /*CalcYCompVars*/
        int32_t SpriteHeight = ABS((int32_t) (DIB_HEIGHT / TransformY)) / VDiv;
        int32_t DrawStartY = MAX(0, (DIB_HEIGHT - SpriteHeight) / 2 + VMoveScreen); 
        int32_t DrawEndY = MIN(DIB_HEIGHT - 1, (SpriteHeight + DIB_HEIGHT) / 2 + VMoveScreen); 
                    
        /*CalcXCompXVars*/
        int32_t SpriteWidth = ABS((int32_t) (DIB_HEIGHT / TransformY)) / UDiv;
        int32_t DrawStartX = MAX(0, SpriteScreenX - SpriteWidth / 2);
        int32_t DrawEndX = MIN(DIB_WIDTH, SpriteWidth / 2 + SpriteScreenX);

        SpriteRenderInfos[I] = (sprite_render_info) { 
            .TransformY = TransformY,
            .SpriteScreenX = SpriteScreenX,
            .SpriteWidth = SpriteWidth, 
            .SpriteHeight = SpriteHeight,
            .FogEffect = FogEffect,
            .DrawStartX = DrawStartX,
            .DrawEndX = DrawEndX,
            .DrawStartY = DrawStartY,
            .DrawEndY = DrawEndY,
            .VMoveScreen = VMoveScreen
        };
    }
}

static void RenderDecksTask(void *TaskData) {
    render_decks_data *P = TaskData; 
    for(int32_t FloorY = P->StartY; FloorY < P->EndY; FloorY++) {
        /*CalcZCompVals*/
        vec2 RayDir = SubVec2(P->GS->Dir, P->GS->Plane); 
        int32_t Horizon = DIB_HEIGHT / 2 - FloorY;
        float PosZ = 0.5F * DIB_HEIGHT;
        float RowDis = PosZ / (float) Horizon;
        float FogEffect = CalcFogEffect(RowDis);

        /*CalcFloorStep*/
        vec2 PlaneTwice = MulVec2(P->GS->Plane, 2); 
        float UnitRowDis = RowDis / (float) DIB_WIDTH;
        vec2 FloorStep = MulVec2(PlaneTwice, UnitRowDis);

        /*CalcInitFloor*/
        vec2 DeltaFloor = MulVec2(RayDir, RowDis); 
        vec2 Floor = AddVec2(P->GS->Pos, DeltaFloor); 

        for(int X = 0; X < DIB_WIDTH; X++) {
            vec2 TexPos = MulVec2(RevTruncVec2(Floor), TEX_LENGTH); 
            int32_t TexX = TexPos.X;
            int32_t TexY = TexPos.Y;
            
            Floor = AddVec2(Floor, FloorStep);

            P->GS->Pixels[FloorY][X] = MulColor(P->GS->TexData[1][TexY][TexX], FogEffect); 
            int32_t CeilY = DIB_HEIGHT - FloorY - 1;
            P->GS->Pixels[CeilY][X] = MulColor(P->GS->TexData[2][TexY][TexX], FogEffect / 2); 
        } 
    }
}

static void RenderSprite(render_facing_data *P, int X, const tile_hit *TileHit) {
    sprite_render_info *RenderInfo = &P->SpriteRenderInfos[TileHit->SpriteI];

    int TexX = (
        256 * 
        (X + RenderInfo->SpriteWidth / 2 - RenderInfo->SpriteScreenX) * 
        TEX_LENGTH / 
        (RenderInfo->SpriteWidth * 256) 
    ); 

    for(int Y = RenderInfo->DrawStartY; Y < RenderInfo->DrawEndY; Y++) {
        int D = (
            (Y - RenderInfo->VMoveScreen) * 256 + 
            (RenderInfo->SpriteHeight - DIB_HEIGHT) * 128 
        );
        int32_t TexY = D * TEX_LENGTH / RenderInfo->SpriteHeight / 256; 
        tile Texture = GetTileData(P->GS->Sprites[TileHit->SpriteI].Tile).TexI;
        color Color = P->GS->TexData[Texture][TexY][TexX];

        if(IsOpaque(Color)) {
            P->GS->Pixels[Y][X] = MulColor(Color, RenderInfo->FogEffect);
        }
    }
}

static void RenderFacingTask(void *TaskData) {
    render_facing_data *P = TaskData; 
    for(int32_t X = P->StartX; X < P->EndX; X++) {
        float CameraX = (float) (X << 1) / (float) DIB_WIDTH - 1;

        /*CalcXCompVars*/
        float RayDirX = P->GS->Dir.X + P->GS->Plane.X * CameraX;
        float DeltaDistX = ABS(1.0F / RayDirX);
        int32_t TileX = (int32_t) P->GS->Pos.X;
        int32_t StepX;
        float SideDistX;
        if(RayDirX < 0.0F) {
            StepX = -1;
            SideDistX = (P->GS->Pos.X - TileX) * DeltaDistX;
        } else {
            StepX = 1;
            SideDistX = (TileX + 1.0F - P->GS->Pos.X) * DeltaDistX;
        }

        /*CalcYCompVars*/
        float RayDirY = P->GS->Dir.Y + P->GS->Plane.Y * CameraX;
        float DeltaDistY = ABS(1.0F / RayDirY);
        int32_t TileY = (int32_t) P->GS->Pos.Y;
        int32_t StepY;
        float SideDistY;
        if(RayDirY < 0.0F) {
            StepY = -1;
            SideDistY = (P->GS->Pos.Y - TileY) * DeltaDistY;
        } else {
            StepY = 1;
            SideDistY = (TileY + 1.0F - P->GS->Pos.Y) * DeltaDistY;
        }

        /*LocateTileHit*/
        tile_hit TileHits[MAX_TILE_HITS];
        int32_t TileHitCount = 0;

        TileHits[TileHitCount++] = (tile_hit) {
            .SpriteI = -1
        };
        if(IsInTileMap(TileY, TileX)) {
            TileHits[0].TileData = GetTileData(P->GS->TileMap[TileY][TileX]);
            while(TileHitCount < MAX_TILE_HITS) {
                tile_hit *TileHitCur = &TileHits[TileHitCount];
                if(SideDistX < SideDistY) {
                    SideDistX += DeltaDistX;
                    TileX += StepX;
                    TileHitCur->Side = false;
                    TileHitCur->PerpWallDist = SideDistX - DeltaDistX;
                } else {
                    SideDistY += DeltaDistY;
                    TileY += StepY;
                    TileHitCur->PerpWallDist = SideDistY - DeltaDistY;
                    TileHitCur->Side = true;
                }
                TileHitCur->SpriteI = -1;
                if(!IsInTileMap(TileY, TileX)) {
                    break;
                }
                TileHitCur->TileData = GetTileData(P->GS->TileMap[TileY][TileX]);
                TileHitCount++;

                if(!(TileHitCur->TileData.Flags & TF_ALPHA)) {
                    break;
                }
            }
        }

        for(uint32_t I = 0; I < P->GS->SpriteCount && TileHitCount < MAX_TILE_HITS; I++) { 
            if(
                P->SpriteRenderInfos[I].TransformY <= 0.0F || 
                P->SpriteRenderInfos[I].DrawStartX > X ||
                P->SpriteRenderInfos[I].DrawEndX <= X
            ) {
                continue;
            }

            tile_hit NewTileHit = {
                .PerpWallDist = P->SpriteRenderInfos[I].TransformY,
                .TileData = GetTileData(P->GS->Sprites[I].Tile), 
                .SpriteI = I,
            };

            /*InsertTileHit*/
            for(int32_t J = 0; J < TileHitCount; J++) {
                if(NewTileHit.PerpWallDist < TileHits[J].PerpWallDist) {
                    memmove(&TileHits[J + 1], &TileHits[J], (TileHitCount - J) * sizeof(*TileHits));
                    TileHits[J] = NewTileHit; 
                    TileHitCount++;
                    break;
                }
            } 
        } 

        int32_t TileI = TileHitCount; 
        bool LayeredColor = false;
        while(TileI-- > 0) {
            tile_hit *TileHitCur = &TileHits[TileI];

            if(TileHitCur->SpriteI >= 0) {
                RenderSprite(P, X, TileHitCur);
            } else if(
                (TileHitCur->TileData.TexI != 0 || TileI == TileHitCount - 1) &&
                (TileHitCur->TileData.Flags & TF_VERT || !TileHitCur->Side) &&
                (TileHitCur->TileData.Flags & TF_HORZ || TileHitCur->Side)
            ) {
                /*FindWall*/
                float FogEffect = CalcFogEffect(TileHitCur->PerpWallDist);
                float WallX = (
                    TileHitCur->Side ?
                        P->GS->Pos.X + TileHitCur->PerpWallDist * RayDirX :
                        P->GS->Pos.Y + TileHitCur->PerpWallDist * RayDirY 
                );
                WallX -= floorf(WallX);

                /*FindTexture*/
                int32_t TexX = (int) (WallX * (float) TEX_LENGTH); 
                if(TileHitCur->Side ? RayDirY < 0.0 : RayDirX > 0.0) {
                    TexX = TEX_LENGTH - TexX - 1; 
                }

                /*RenderLine*/
                int32_t LineHeight = (int32_t) (DIB_HEIGHT / TileHitCur->PerpWallDist);
                int32_t HalfHeight = LineHeight / 2;

                int32_t DrawCenter = DIB_HEIGHT / 2;
                int32_t DrawStart = MAX(0, DrawCenter - HalfHeight);
                int32_t DrawEnd = MIN(DIB_HEIGHT - 1, DrawCenter + HalfHeight);

                float Step = (float) TEX_LENGTH / (DIB_HEIGHT / TileHitCur->PerpWallDist);
                float TexPos = (float) (DrawStart - DrawCenter + HalfHeight) * Step; 

                for(int32_t Y = DrawStart; Y < DrawEnd; Y++) {
                    int32_t TexY = (int32_t) TexPos & (TEX_LENGTH - 1);
                    TexPos += Step;
                    color TexColor = P->GS->TexData[TileHitCur->TileData.TexI][TexY][TexX]; 
                    color OutColor = MulColor(TexColor, FogEffect);
                    if(TileHitCur->TileData.Flags | TF_ALPHA) {
                        color ToLayerColor = LayeredColor ? P->GS->Pixels[Y][X] : OpaqueColor(0, 0, 0);
                        OutColor = LayerColor(ToLayerColor, OutColor); 
                    }
                    if(TileHitCur->Side) {
                        OutColor = HalfColor(OutColor);
                    }
                    P->GS->Pixels[Y][X] = OutColor;
                }
            }
            LayeredColor = true;
        }
    }
}

static void RenderDecks(game_state *GS) {
    render_decks_data TaskData[_countof(GS->Workers)];

    for(size_t I = 0; I < _countof(TaskData); I++) {
        TaskData[I] = (render_decks_data) {
            .GS = GS,
            .StartY = I * DIB_HEIGHT / _countof(TaskData) / 2, 
            .EndY  = (I + 1) * DIB_HEIGHT / _countof(TaskData) / 2
        };

        GS->Workers[I].Data = &TaskData[I]; 
        GS->Workers[I].Task = RenderDecksTask;
    }
    WorkerMultiWait(_countof(GS->Workers), GS->Workers); 
}

static void RenderFacing(game_state *GS, sprite_render_info SpriteRenderInfos[static SPR_CAP]) {
    render_facing_data TaskData[_countof(GS->Workers)];

    for(size_t I = 0; I < _countof(TaskData); I++) {
        TaskData[I] = (render_facing_data) {
            .GS = GS,
            .StartX = I * DIB_WIDTH / _countof(TaskData), 
            .EndX = (I + 1) * DIB_WIDTH / _countof(TaskData),
            .SpriteRenderInfos = SpriteRenderInfos
        };

        GS->Workers[I].Data = &TaskData[I]; 
        GS->Workers[I].Task = RenderFacingTask;
    }
    WorkerMultiWait(_countof(GS->Workers), GS->Workers); 
}

void RenderWorld(game_state *GS) {
    SortSprites(GS);

    sprite_render_info SpriteRenderInfos[SPR_CAP];
    ComputeRenderSpriteInfos(GS, SpriteRenderInfos);

    RenderDecks(GS);
    RenderFacing(GS, SpriteRenderInfos);
}

