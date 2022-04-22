#ifndef VEC2_HPP
#define VEC2_HPP

#include <math.h>

typedef struct vec2 {
    float X;
    float Y;
} vec2;

static inline vec2 AddVec2(vec2 A, vec2 B) {
    return (vec2) {A.X + B.X, A.Y + B.Y};
}

static inline vec2 SubVec2(vec2 A, vec2 B) {
    return (vec2) {A.X - B.X, A.Y - B.Y};
}

static inline vec2 MulVec2(vec2 Vec, float Val) {
    return (vec2) {Vec.X * Val, Vec.Y * Val};
} 

static inline float DotVec2(vec2 Vec) {
    return Vec.X * Vec.X + Vec.Y * Vec.Y;
}

static inline vec2 RevTruncVec2(vec2 Vec) {
    return (vec2) {fmodf(Vec.X, 1), fmodf(Vec.Y, 1)};
}

static inline float SquareDisVec2(vec2 A, vec2 B) {
    return DotVec2(SubVec2(A, B));
}

static inline float DetVec2(vec2 ColA, vec2 ColB) {
    return ColA.X * ColB.Y - ColA.Y * ColB.X; 
}

static inline vec2 RotateVec2(vec2 Vec, float Rot) {
    return (vec2) {
        .X = Vec.X * cosf(Rot) - Vec.Y * sinf(Rot), 
        .Y = Vec.X * sinf(Rot) + Vec.Y * cosf(Rot)
    };
}

#endif

