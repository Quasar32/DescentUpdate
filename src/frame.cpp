#include <windows.h>
#include "frame.hpp"

int64_t Frame::QueryPerfFreq() {
    LARGE_INTEGER PerfFreq;
    QueryPerformanceFrequency(&PerfFreq);
    return PerfFreq.QuadPart;
}

int64_t Frame::QueryPerfCounter() {
    LARGE_INTEGER PerfCounter;
    QueryPerformanceCounter(&PerfCounter);
    return PerfCounter.QuadPart;
}

void Frame::UpdateDeltaCounter() {
    DeltaCounter = QueryPerfCounter() - BeginCounter; 
}

Frame::Frame(float FPS) {
    IsGranular = (timeBeginPeriod(1U) == TIMERR_NOERROR); 
    PerfFreq = QueryPerfFreq();
    BeginCounter = QueryPerfFreq();
    DeltaCounter = 0LL;
    PeriodCounter = (int64_t) ((float) PerfFreq / FPS);
}

Frame::~Frame() {
    if(IsGranular) {
        timeEndPeriod(1U);
    } 
}

void Frame::StartFrame() {
    BeginCounter = QueryPerfCounter();
}

int64_t Frame::CounterToMS(int64_t Counter) {
    return 1000LL * Counter / PerfFreq;
}

void Frame::EndFrame() {
    UpdateDeltaCounter();
    if(IsGranular) {
        int64_t SleepMS = CounterToMS(PeriodCounter - DeltaCounter);
        if(SleepMS > 0LL) {
            Sleep(SleepMS);
        }
    }
    while(DeltaCounter < PeriodCounter) {
        UpdateDeltaCounter();
    }
}

float Frame::GetFrameDelta() {
    return (
        (float) (DeltaCounter == 0LL ? PeriodCounter : DeltaCounter) / 
        (float) PerfFreq
    );
}
