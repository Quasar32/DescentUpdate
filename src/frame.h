#ifndef FRAME_HPP
#define FRAME_HPP

#include <stdint.h>
#include <stdbool.h>

typedef struct frame {
    bool IsGranular;
    int64_t PerfFreq;
    int64_t BeginCounter;
    int64_t DeltaCounter;
    int64_t PeriodCounter;
} frame;

frame CreateFrame(float FPS);
void DestroyFrame(frame *Frame);

void StartFrame(frame *Frame);
void EndFrame(frame *Frame);
float GetFrameDelta(frame *Frame);

#endif
