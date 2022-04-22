#include <windows.h>
#include "frame.h"

static int64_t QueryPerfFreq(void) {
    LARGE_INTEGER PerfFreq;
    QueryPerformanceFrequency(&PerfFreq);
    return PerfFreq.QuadPart;
}

static int64_t QueryPerfCounter(void) {
    LARGE_INTEGER PerfCounter;
    QueryPerformanceCounter(&PerfCounter);
    return PerfCounter.QuadPart;
}

static int64_t CounterToMS(int64_t Counter, int64_t PerfFreq) {
    return 1000LL * Counter / PerfFreq;
}

static void UpdateDeltaCounter(frame *Frame) {
    Frame->DeltaCounter = QueryPerfCounter() - Frame->BeginCounter; 
}

frame CreateFrame(float FPS) {
    frame Result = {
        .IsGranular = (timeBeginPeriod(1U) == TIMERR_NOERROR), 
        .PerfFreq = QueryPerfFreq(),
        .BeginCounter = QueryPerfFreq(),
        .DeltaCounter = 0LL,
        .PeriodCounter = (int64_t) ((float) Result.PerfFreq / FPS)
    };
    return Result;
}

void DestroyFrame(frame *Frame) {
    if(Frame->IsGranular) {
        timeEndPeriod(1U);
    } 
}

void StartFrame(frame *Frame) {
    Frame->BeginCounter = QueryPerfCounter();
}

void EndFrame(frame *Frame) {
    UpdateDeltaCounter(Frame);
    if(Frame->IsGranular) {
        int64_t SleepMS = CounterToMS(Frame->PeriodCounter - Frame->DeltaCounter, Frame->PerfFreq);
        if(SleepMS > 0LL) {
            Sleep(SleepMS);
        }
    }
    while(Frame->DeltaCounter < Frame->PeriodCounter) {
        UpdateDeltaCounter(Frame);
    }
}

float GetFrameDelta(frame *Frame) {
    return (
        (float) (Frame->DeltaCounter == 0LL ? Frame->PeriodCounter : Frame->DeltaCounter) / 
        (float) Frame->PerfFreq
    );
}
