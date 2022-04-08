#ifndef FRAME_HPP
#define FRAME_HPP

#include <stdint.h>

class Frame {
private:
    static int64_t QueryPerfFreq();
    static int64_t QueryPerfCounter();

    bool IsGranular;
    int64_t PerfFreq;
    int64_t BeginCounter;
    int64_t DeltaCounter;
    int64_t PeriodCounter;

    void UpdateDeltaCounter();
    int64_t CounterToMS(int64_t Counter);

public:
    Frame(float FPS);
    ~Frame();

    void StartFrame();
    void EndFrame();
    float GetFrameDelta();
};

#endif
