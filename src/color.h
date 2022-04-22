#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>

typedef union color {
    uint32_t Raw;
    struct {
        uint8_t Red;
        uint8_t Green;
        uint8_t Blue;
        uint8_t Alpha;
    };
} color;

static inline color OpaqueColor(uint8_t Red, uint8_t Green, uint8_t Blue) {
    return (color) {.Red = Red, .Green = Green, .Blue = Blue, .Alpha = 255};
}

static inline bool IsOpaque(color Color) {
    return Color.Alpha == 255; 
}

static inline color MulColor(color Color, float Mul) {
    return (color) {
        .Red = Color.Red * Mul,
        .Green = Color.Green * Mul,
        .Blue = Color.Blue * Mul,
        .Alpha = Color.Alpha 
    };
}

static inline color HalfColor(color Color) {
    return (color) ((Color.Raw >> 1) & 0x007F7F7F);
}

static inline color AddColor(color Left, color Right) {
    return (color) {
        .Red = Left.Red + Right.Red,
        .Green = Left.Green + Right.Green,
        .Blue = Left.Blue + Right.Blue, 
        .Alpha = Left.Alpha + Right.Alpha
    };
}

static inline color LayerColor(color AddTo, color ToAdd) {
    color Left = (MulColor(AddTo, 1.0F - (ToAdd.Alpha) / 255.0F));
    color Right = MulColor(ToAdd, ToAdd.Alpha / 255.0F);
    return AddColor(Left, Right);
}

#endif
