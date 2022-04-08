#ifndef WORKER_HPP
#define WORKER_HPP

#include <windows.h>
#include <assert.h>
    
using WorkerProc = void (*) (void *);

struct Task {
    virtual void Run() = 0;
};

class Worker {
private:
    static DWORD WINAPI ThreadWorkerProc(LPVOID);
    void Update();

    HANDLE Thread; 
    HANDLE StartEvent;
    HANDLE EndEvent;

    Task *ToRun = nullptr;

public:
    static void MultiWait(int WorkerCount, Worker *Workers);

    Worker();
    ~Worker();

    void Schedule(Task &ToRun);

    template<typename TaskType>
    static void ScheduleAndWait(
        size_t WorkerCount, 
        Worker *Workers, 
        size_t TaskCount, 
        TaskType *Tasks
    ) {
        assert(TaskCount <= WorkerCount);
        for(size_t I = 0; I < TaskCount; I++) {
            Workers[I].Schedule(Tasks[I]);
        }
        MultiWait(WorkerCount, Workers); 
    }
};


#endif
