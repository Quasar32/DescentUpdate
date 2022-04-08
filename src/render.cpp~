#include "render.hpp"
#include "scalar.hpp"
#include "vec2.hpp"

template<typename Type>
void Swap(Type &A, Type &B) {
   auto Tmp = A;
   A = B;
   B = Tmp;
}

static uint32_t HalfColor(uint32_t Color) {
    return (Color >> 1) & 0x007F7F7F;

}

void FillColor(
    uint32_t Texture[TEX_LENGTH][TEX_LENGTH],
    uint32_t Color
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

static uint32_t MulColor(uint32_t Color, float Mul) {
    uint8_t OldR = Color & 0xFF;
    uint8_t OldG = (Color >> 8) & 0xFF;
    uint8_t OldB = (Color >> 16) & 0xFF;
    uint8_t NewR = OldR * Mul;
    uint8_t NewG = OldG * Mul;
    uint8_t NewB = OldB * Mul;

    return NewR + (NewG << 8) + (NewB << 16);
}

class RenderDecksTask : public Task {
private:
    GameState *GS;
    int StartY;
    int EndY;

public:
    RenderDecksTask(GameState *GS, int StartY, int EndY) :
        GS(GS),
        StartY(StartY),
        EndY(EndY) {}

    void Run() {
        for(int32_t CeilY = StartY; CeilY < EndY; CeilY++) {
            /*CalcZCompVals*/
            Vec2 RayDir = GS->Dir - GS->Plane; 
            int32_t Horizon = CeilY - DIB_HEIGHT / 2;
            float PosZ = 0.5F * DIB_HEIGHT;
            float RowDis = PosZ / (float) Horizon;
            float FogEffect = CalcFogEffect(RowDis);

            /*CalcCeilStep*/
            Vec2 PlaneTwice = GS->Plane * 2; 
            float UnitRowDis = RowDis / (float) DIB_WIDTH;
            Vec2 CeilStep = PlaneTwice * UnitRowDis;

            /*CalcInitCeil*/
            Vec2 DeltaCeil = RayDir * RowDis; 
            Vec2 Ceil = GS->Pos + DeltaCeil; 

            for(int X = 0; X < DIB_WIDTH; X++) {
                Vec2 TexPos = RevTruncVec2(Ceil) * TEX_LENGTH; 
                int32_t TexX = TexPos.X;
                int32_t TexY = TexPos.Y;
                
                Ceil = Ceil + CeilStep;

                GS->Pixels[CeilY][X] = MulColor(GS->TexData[2][TexY][TexX], FogEffect / 2); 
                int32_t FloorY = DIB_HEIGHT - CeilY - 1;
                GS->Pixels[FloorY][X] = MulColor(GS->TexData[1][TexY][TexX], FogEffect); 
            } 
        }
    }
};

class RenderWallTask : public Task {
private:
    GameState *GS;
    int StartX;
    int EndX;

public:
    RenderWallTask(GameState *GS, int StartX, int EndX) :
        GS(GS),
        StartX(StartX),
        EndX(EndX) {}

    void Run() {
        for(int32_t X = StartX; X < EndX; X++) {
            float CameraX = (float) (X << 1) / (float) DIB_WIDTH - 1;

            /*CalcXCompVars*/
            float RayDirX = GS->Dir.X + GS->Plane.X * CameraX;
            float DeltaDistX = Abs(1.0F / RayDirX);
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
            float DeltaDistY = Abs(1.0F / RayDirY);
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
            int32_t Repeat = 0; 
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
                Repeat++;
            }

            /*FindWall*/
            float PerpWallDist = (
                Side == 0 ? 
                    SideDistX - DeltaDistX : 
                    SideDistY - DeltaDistY
            );
            float FogEffect = CalcFogEffect(PerpWallDist);
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
            int32_t DrawStart = Max(0, DrawCenter - HalfHeight);
            int32_t DrawEnd = Min(DIB_HEIGHT - 1, DrawCenter + HalfHeight);

            float Step = (float) TEX_LENGTH / (DIB_HEIGHT / PerpWallDist);
            float TexPos = (float) (DrawStart - DrawCenter + HalfHeight) * Step; 

            for(int32_t Y = DrawStart; Y < DrawEnd; Y++) {
                int32_t TexY = (int32_t) TexPos & (TEX_LENGTH - 1);
                TexPos += Step;
                int32_t Color = GS->TexData[TexI][TexY][TexX]; 
                if(Side == 1) {
                    Color = HalfColor(Color);
                }
                GS->Pixels[Y][X] = MulColor(Color, FogEffect);
            }
            GS->ZBuffer[X] = PerpWallDist;
        }
    }
};

static void RenderDecks(GameState *GS) {
    RenderDecksTask RenderDecksTasks[] = {
        RenderDecksTask(GS, DIB_HEIGHT / 2 + 1, DIB_HEIGHT * 5 / 8),
        RenderDecksTask(GS, DIB_HEIGHT * 5 / 8, DIB_HEIGHT * 6 / 8),
        RenderDecksTask(GS, DIB_HEIGHT * 6 / 8, DIB_HEIGHT * 7 / 8),
        RenderDecksTask(GS, DIB_HEIGHT * 7 / 8, DIB_HEIGHT * 8 / 8)
    };
    Worker::ScheduleAndWait(
        _countof(GS->Workers), 
        GS->Workers, 
        _countof(RenderDecksTasks), 
        RenderDecksTasks
    );
}

static void RenderWalls(GameState *GS) {
    RenderWallTask RenderWallTasks[] = {
       RenderWallTask(GS, 0, DIB_WIDTH / 4), 
       RenderWallTask(GS, DIB_WIDTH / 4, DIB_WIDTH / 2), 
       RenderWallTask(GS, DIB_WIDTH / 2, 3 * DIB_WIDTH / 4), 
       RenderWallTask(GS, 3 * DIB_WIDTH / 4, DIB_WIDTH) 
    };
    Worker::ScheduleAndWait(
        _countof(GS->Workers), 
        GS->Workers, 
        _countof(RenderWallTasks), 
        RenderWallTasks 
    );
}

static void SortSprites(GameState *GS) {
    for(uint32_t I = 0; I < GS->SpriteCount; I++) {
        GS->SpriteSquareDis[I] = SquareDisVec2(GS->Pos, GS->Sprites[I].Pos);
        for(int32_t J = I; J > 0; J--) {
            if(GS->SpriteSquareDis[J] < GS->SpriteSquareDis[J - 1]) {
                break;
            }
            Swap(GS->Sprites[J], GS->Sprites[J - 1]); 
            Swap(GS->SpriteSquareDis[J], GS->SpriteSquareDis[J - 1]); 
        }
    }
} 

static void RenderSprites(GameState *GS) {
    int32_t BobCycle = (int32_t) (GS->TotalTime * 16) % 16;
    int32_t Bob = BobCycle < 8 ? BobCycle : 16 - BobCycle; 

    for(uint32_t I = 0; I < GS->SpriteCount; I++) {
        Vec2 SpriteDis = GS->Sprites[I].Pos - GS->Pos;
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
        int32_t SpriteHeight = Abs((int32_t) (DIB_HEIGHT / TransformY)) / VDiv;
        int32_t DrawStartY = Max(
            0, 
            (DIB_HEIGHT - SpriteHeight) / 2 + VMoveScreen 
        ); 
        int32_t DrawEndY = Min(
            DIB_HEIGHT - 1, 
            (SpriteHeight + DIB_HEIGHT) / 2 + VMoveScreen 
        ); 
                    
        /*CalcXCompXVars*/
        int32_t SpriteWidth = Abs((int32_t) (DIB_HEIGHT / TransformY)) / UDiv;
        int32_t DrawStartX = Max(0, SpriteScreenX - SpriteWidth / 2);
        int32_t DrawEndX = Min(DIB_WIDTH, SpriteWidth / 2 + SpriteScreenX);

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

                    if(Color != 0xFFFF00FF) {
                        GS->Pixels[Y][X] = MulColor(Color, FogEffect);
                    }
                }
            }
        }
    }
}

void RenderWorld(GameState *GS) {
    RenderDecks(GS);
    RenderWalls(GS);
    SortSprites(GS);
    RenderSprites(GS);
}

