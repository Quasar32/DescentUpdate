#include "render.h"
#include "scalar.h"
#include "vec2.h"
#include "tile_data.h"

#define SWAP(A, B) do {\
    __auto_type A_ = (A);\
    __auto_type B_ = (B);\
    __auto_type Tmp = *A_;\
    *A_ = *B_;\
    *B_ = Tmp;\
} while(0)

void FillColor(
    color Texture[TEX_LENGTH][TEX_LENGTH],
    color Color
) {
    for(int Y = 0; Y < TEX_LENGTH; Y++) {
        for(int X = 0; X < TEX_LENGTH; X++) {
            Texture[Y][X] = Color;
        }
    } 
}

static float CalcFogEffect(float Dis) {
    return Dis < 5.0F ? 1.0F - Dis / 5.0F : 0.0F;
}

typedef struct render_decks_data {
    game_state *GS;
    int StartY;
    int EndY;
} render_decks_data;

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

typedef struct render_wall_data {
    game_state *GS;
    int StartX;
    int EndX;
} render_wall_data;

static void RenderWallTask(void *TaskData) {
    render_wall_data *P = TaskData; 
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
        typedef struct tile_hit {
            float SideDistX;
            float SideDistY;
            int32_t TileI;
            int32_t TileX;
            int32_t TileY;
            bool Side;
            tile_data TileData;
        } tile_hit; 

        tile_hit TileHits[32];
        int32_t TileHitCount = 0;

        TileHits[TileHitCount++] = (tile_hit) {
            .SideDistX = SideDistX,
            .SideDistY = SideDistY,
            .TileX = TileX,
            .TileY = TileY,
        };
        if(IsInTileMap(TileY, TileX)) {
            TileHits[0].TileI = P->GS->TileMap[TileHits[0].TileY][TileHits[0].TileX];
            while(TileHitCount < _countof(TileHits)) {
                tile_hit *TileHitPrev = &TileHits[TileHitCount - 1];
                tile_hit *TileHitCur = &TileHits[TileHitCount];
                *TileHitCur = *TileHitPrev;
                if(TileHitPrev->SideDistX < TileHitPrev->SideDistY) {
                    TileHitCur->SideDistX += DeltaDistX;
                    TileHitCur->TileX += StepX;
                    TileHitCur->Side = false;
                } else {
                    TileHitCur->SideDistY += DeltaDistY;
                    TileHitCur->TileY += StepY;
                    TileHitCur->Side = true;
                }
                if(!IsInTileMap(TileHitCur->TileY, TileHitCur->TileX)) {
                    break;
                }
                TileHitCur->TileI = P->GS->TileMap[TileHitCur->TileY][TileHitCur->TileX];
                TileHitCount++;

                TileHitCur->TileData = GetTileData(TileHitCur->TileI);
                if(!(TileHitCur->TileData.Flags & TF_ALPHA)) {
                    break;
                }
            }
        }

        int32_t TileI = TileHitCount; 
        bool LayeredColor = false;
        while(TileI-- > 0) {
            tile_hit *TileHitCur = &TileHits[TileI];
            if(TileHitCur->TileI == 0 && TileI != TileHitCount - 1) {
                continue;
            }

            /*FindWall*/
            float PerpWallDist = (
                TileHitCur->Side ? 
                    TileHitCur->SideDistY - DeltaDistY :
                    TileHitCur->SideDistX - DeltaDistX  
            );
            float FogEffect = CalcFogEffect(PerpWallDist);
            float WallX = (
                TileHitCur->Side ?
                    P->GS->Pos.X + PerpWallDist * RayDirX :
                    P->GS->Pos.Y + PerpWallDist * RayDirY 
            );
            WallX -= floorf(WallX);

            /*FindTexture*/
            int32_t TexX = (int) (WallX * (float) TEX_LENGTH); 
            if(TileHitCur->Side ? RayDirY < 0.0 : RayDirX > 0.0) {
                TexX = TEX_LENGTH - TexX - 1; 
            }

            /*RenderLine*/
            int32_t LineHeight = (int32_t) (DIB_HEIGHT / PerpWallDist);
            int32_t HalfHeight = LineHeight / 2;

            int32_t DrawCenter = DIB_HEIGHT / 2;
            int32_t DrawStart = MAX(0, DrawCenter - HalfHeight);
            int32_t DrawEnd = MIN(DIB_HEIGHT - 1, DrawCenter + HalfHeight);

            float Step = (float) TEX_LENGTH / (DIB_HEIGHT / PerpWallDist);
            float TexPos = (float) (DrawStart - DrawCenter + HalfHeight) * Step; 

            if(
                (!(TileHitCur->TileData.Flags & TF_VERT) && TileHitCur->Side) ||
                (!(TileHitCur->TileData.Flags & TF_HORZ) && !TileHitCur->Side)
            ) {
                continue;
            }
        

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
            LayeredColor = true;
            P->GS->ZBuffer[X] = PerpWallDist;
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

static void RenderWalls(game_state *GS) {
    render_wall_data TaskData[_countof(GS->Workers)];

    for(size_t I = 0; I < _countof(TaskData); I++) {
        TaskData[I] = (render_wall_data) {
            .GS = GS,
            .StartX = I * DIB_WIDTH / _countof(TaskData), 
            .EndX = (I + 1) * DIB_WIDTH / _countof(TaskData)
        };

        GS->Workers[I].Data = &TaskData[I]; 
        GS->Workers[I].Task = RenderWallTask;
    }
    WorkerMultiWait(_countof(GS->Workers), GS->Workers); 
}

static void SortSprites(game_state *GS) {
    for(uint32_t I = 0; I < GS->SpriteCount; I++) {
        GS->SpriteSquareDis[I] = SquareDisVec2(GS->Pos, GS->Sprites[I].Pos);
        for(int32_t J = I; J > 0; J--) {
            if(GS->SpriteSquareDis[J] < GS->SpriteSquareDis[J - 1]) {
                break;
            }
            SWAP(&GS->Sprites[J], &GS->Sprites[J - 1]); 
            SWAP(&GS->SpriteSquareDis[J], &GS->SpriteSquareDis[J - 1]); 
        }
    }
} 

static void RenderSprites(game_state *GS) {
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
        float PerpDist = GS->SpriteSquareDis[I];
        if(PerpDist > 25.0F) {
            continue;
        }
        float FogEffect = CalcFogEffect(PerpDist);

        /*CalcYCompVars*/
        int32_t SpriteHeight = ABS((int32_t) (DIB_HEIGHT / TransformY)) / VDiv;
        int32_t DrawStartY = MAX(
            0, 
            (DIB_HEIGHT - SpriteHeight) / 2 + VMoveScreen 
        ); 
        int32_t DrawEndY = MIN(
            DIB_HEIGHT - 1, 
            (SpriteHeight + DIB_HEIGHT) / 2 + VMoveScreen 
        ); 
                    
        /*CalcXCompXVars*/
        int32_t SpriteWidth = ABS((int32_t) (DIB_HEIGHT / TransformY)) / UDiv;
        int32_t DrawStartX = MAX(0, SpriteScreenX - SpriteWidth / 2);
        int32_t DrawEndX = MIN(DIB_WIDTH, SpriteWidth / 2 + SpriteScreenX);

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
                    color Color = GS->TexData[GS->Sprites[I].Texture][TexY][TexX];

                    if(IsOpaque(Color)) {
                        GS->Pixels[Y][X] = MulColor(Color, FogEffect);
                    }
                }
            }
        }
    }
}

void RenderWorld(game_state *GS) {
    RenderDecks(GS);
    RenderWalls(GS);
    SortSprites(GS);
    RenderSprites(GS);
}

