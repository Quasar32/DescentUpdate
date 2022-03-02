#include "descent.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/**
 * IsInBounds2D() - Checks if index pair is in 2D array
 * @RowCount: Number of rows in array
 * @ColCount: Number of cols in array
 * @Row: Row index to test 
 * @Col: Col index to test
 *
 * Return: If is in bounds return nonzero, eleswise zero
 */
static bool IsInBounds2D(size_t RowCount, size_t ColCount, size_t Row, size_t Col) {
    return Row < RowCount && Col < ColCount;
} 

static bool IsInTileMap(size_t Row, size_t Col) {
    return Row < TILE_MAP_HEIGHT && Col < TILE_MAP_WIDTH;
}

/**
 * GetColorForSolidColorTile() - Gets color value of tile
 * @Tile: Tile value
 *
 * Return: Color value 
 */
static uint32_t GetColorForSolidColorTile(uint8_t Tile) {
    uint8_t Red = (Tile & 0x30) * 85;
    uint8_t Green = (Tile & 0x0C) * 85;  
    uint8_t Blue = (Tile & 0x03) * 85;
    return (Red << 16) + (Green << 8) + Blue; 
}

/**
 * FloatMagnitude() - Gets absolute value of single-precision float
 * @Value:
 */
static float FloatMagnitude(float Value) {
    return Value < 0.0F ? -Value : Value;
} 

/**
 * MinInt32() - Get min of two 32-bit integers 
 * @A: First value
 * @B: Second value
 *
 * Return: Min of two values 
 */
static int32_t MinInt32(int32_t A, int32_t B) {
    return A < B ? A : B;
}

/**
 * MaxInt32() - Get max of two 32-bit integers 
 * @A: First value
 * @B: Second value
 *
 * Return: Max of two values 
 */
static int32_t MaxInt32(int32_t A, int32_t B) {
    return A > B ? A : B;
}

/**
 * RenderRaycastWorld() - Renders raycasted tilemap 
 * @State: Uses camera and tiles
 */
static void RenderRaycastWorld(struct game_state *State) {
    memset(State->Pixels, 0, sizeof(State->Pixels));
    for(int32_t X = 0; X < DIB_WIDTH; X++) {
        float CameraX = (float) (X << 1) / (float) (DIB_WIDTH - 1);

        /*CalcXCompVars*/
        float RayDirX = State->Dir.X + State->Plane.X * CameraX;
        float DeltaDistX = FloatMagnitude(1.0F / RayDirX);
        int32_t TileX = (int32_t) State->Pos.X;
        int32_t StepX;
        float SideDistX;
        if(RayDirX < 0.0F) {
            StepX = -1;
            SideDistX = (State->Pos.X - TileX) * DeltaDistX;
        } else {
            StepX = 1;
            SideDistX = (TileX + 1.0 - State->Pos.X) * DeltaDistX;
        }

        /*CalcYCompVars*/
        float RayDirY = State->Dir.Y + State->Plane.Y * CameraX;
        float DeltaDistY = FloatMagnitude(1.0F / RayDirY);
        int32_t TileY = (int32_t) State->Pos.Y;
        int32_t StepY;
        float SideDistY;
        if(RayDirY < 0.0F) {
            StepY = -1;
            SideDistY = (State->Pos.Y - TileY) * DeltaDistY;
        } else {
            StepY = 1;
            SideDistY = (TileY + 1.0 - State->Pos.Y) * DeltaDistY;
        }

        /*LocateTileHit*/
        int32_t Side;
        uint8_t Tile = 0;
        while(
            IsInBounds2D(TILE_MAP_HEIGHT, TILE_MAP_WIDTH, TileY, TileX) &&
            (Tile = State->TileMap[TileY][TileX]) == 0
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

        /*RenderRaycastLine*/
        float PerpWallDist = (
            Side == 0 ? 
                SideDistX - DeltaDistX : 
                SideDistY - DeltaDistY
        );

        int32_t Pitch = 100;
        int32_t LineHeight = (int32_t) (DIB_HEIGHT / PerpWallDist);
        int32_t HalfHeight = LineHeight / 2;
        int32_t DrawCenter = DIB_HEIGHT / 2 + Pitch;
        int32_t DrawStart = MaxInt32(0, DrawCenter - HalfHeight);
        int32_t DrawEnd = MinInt32(DIB_HEIGHT - 1, DrawCenter + HalfHeight);

        for(int32_t Y = DrawStart; Y < DrawEnd; Y++) {
            uint32_t Color = GetColorForSolidColorTile(Tile);
            if(Side == 1) {
                /*HalfAllColorComponents*/
                Color = (Color >> 1) & 0x007F7F7F;
            }
            State->Pixels[Y][X] = Color;
        }
    }
}

/**
 * RotateVec2() - Rotates vector by number of radians 
 * @Vec: Vector to rotate
 * @Rot: Radian angle to rotate 
 *
 * TODO: Need to check which clock direction it rotates 
 *
 * Return: Returns rotated vector
 */
static struct vec2 RotateVec2(struct vec2 Vec, float Rot) {
    return (struct vec2) {
        .X = Vec.X * cos(Rot) - Vec.Y * sin(Rot), 
        .Y = Vec.X * sin(Rot) + Vec.Y * cos(Rot)
    };
}

/** 
 * SquareMagVec2() - Get square of magnitude of 2D vector
 * @Vec: Vector to get square magnitude of
 *
 * Return: Square of magnitude of vector
 */
static float SquareMagVec2(struct vec2 Vec) {
    return Vec.X * Vec.X + Vec.Y * Vec.Y;
}

/**
 * ScalarMultiplyVec2() - Multiples 2D vector by a scalar
 * @Vec: Vector to multiply
 * @Val: Value to multiply
 */
static struct vec2 ScalarMultiplyVec2(struct vec2 Vec, float Val) {
    return (struct vec2) {Vec.X * Val, Vec.Y * Val};
} 

/**
 * AddVec2() - Adds two 2D vectors 
 * @A: Left vector
 * @B: Right vector 
 * 
 * Return: Sum of vectors
 */
static struct vec2 AddVec2(struct vec2 A, struct vec2 B) {
    return (struct vec2) {A.X + B.X, A.Y + B.Y};
}

/**
 * MoveCamera() - Moves camera forward 
 * @State: Uses camera from game state
 * @Velocity: Change in distance forward per second 
 * @FrameDelta: Seconds passed
 */
static void MoveCamera(
    struct game_state *State, 
    float Velocity,
    float FrameDelta
) {
    float FrameVelocity = Velocity * FrameDelta;
    struct vec2 PosDelta = ScalarMultiplyVec2(State->Dir, FrameVelocity);
    assert(SquareMagVec2(PosDelta) < 0.5F);

    struct vec2 NewPos = AddVec2(State->Pos, PosDelta);
    int32_t TileX = (int32_t) NewPos.X;
    int32_t TileY = (int32_t) NewPos.Y;
    if(IsInTileMap(TileY, TileX) && State->TileMap[TileY][TileX] == 0) {
        State->Pos = NewPos;
    }
}

/**
 * RotateCamera() - Rotates camera by radians
 * @State: Uses camera from game state
 * @RotPerSec: Change in angle counterclockwise 
 * @FrameDelta: Seconds passed
 */
static void RotateCamera(
    struct game_state *State, 
    float RotPerSec, 
    float FrameDelta
) {
    float Rot = RotPerSec * FrameDelta;
    State->Dir = RotateVec2(State->Dir, Rot); 
    State->Plane = RotateVec2(State->Plane, Rot);
}

/**
 * GameUpdate() - Platform independent game code 
 * @State: Game state
 */
void GameUpdate(struct game_state *State) { 
    if(!State->IsInit) {
        for(int32_t X = 0; X < TILE_MAP_WIDTH; X++) {
            State->TileMap[0][X] = 30;    
            State->TileMap[TILE_MAP_HEIGHT - 1][X] = 30;    
        }

        for(int32_t Y = 0; Y < TILE_MAP_HEIGHT; Y++) {
            State->TileMap[Y][0] = 30;    
            State->TileMap[Y][TILE_MAP_WIDTH - 1] = 30; 
        }

        State->Dir = (struct vec2) {-1.0F, 0.0F};
        State->Pos = (struct vec2) {5.0F, 5.0F};
        State->Plane = (struct vec2) {0.0F, 0.66F};

        State->IsInit = 1;
    }

    float FrameDelta = 1.0F / 60.0F;
    if(State->Buttons[BT_LEFT]) {
        RotateCamera(State, 2.0F, FrameDelta);
    }
    if(State->Buttons[BT_UP]) {
        MoveCamera(State, 3.0F, FrameDelta); 
    }
    if(State->Buttons[BT_RIGHT]) {
        RotateCamera(State, -2.0F, FrameDelta);
    }
    if(State->Buttons[BT_DOWN]) {
        MoveCamera(State, -3.0F, FrameDelta); 
    }

    RenderRaycastWorld(State);
}

