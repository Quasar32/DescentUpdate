#ifndef VEC2_HPP
#define VEC2_HPP

#include <math.h>

struct Vec2 {
    float X;
    float Y;
};

inline Vec2 operator +(Vec2 A, Vec2 B) {
    return (Vec2) {A.X + B.X, A.Y + B.Y};
}

inline Vec2 operator -(Vec2 A, Vec2 B) {
    return (Vec2) {A.X - B.X, A.Y - B.Y};
}

inline Vec2 operator *(Vec2 Vec, float Val) {
    return (Vec2) {Vec.X * Val, Vec.Y * Val};
} 

inline float DotVec2(Vec2 Vec) {
    return Vec.X * Vec.X + Vec.Y * Vec.Y;
}

inline Vec2 RevTruncVec2(Vec2 Vec) {
    return (Vec2) {fmodf(Vec.X, 1), fmodf(Vec.Y, 1)};
}

inline float SquareDisVec2(Vec2 A, Vec2 B) {
    return DotVec2(A - B);
}

inline float DetVec2(Vec2 ColA, Vec2 ColB) {
    return ColA.X * ColB.Y - ColA.Y * ColB.X; 
}

inline Vec2 RotateVec2(Vec2 Vec, float Rot) {
    return (Vec2) {
        .X = Vec.X * cosf(Rot) - Vec.Y * sinf(Rot), 
        .Y = Vec.X * sinf(Rot) + Vec.Y * cosf(Rot)
    };
}

#endif

