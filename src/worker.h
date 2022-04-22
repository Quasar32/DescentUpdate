#ifndef WORKER_HPP
#define WORKER_HPP

#include <windows.h>
#include <assert.h>
    
typedef struct worker {
    HANDLE Thread; 
    HANDLE StartEvent;
    HANDLE EndEvent;

    void (*Task)(void *);
    LPVOID Data;
} worker;

void CreateWorker(worker *Worker);
void DestroyWorker(worker *Worker);

DWORD WINAPI ThreadWorkerProc(LPVOID);
void WorkerMultiWait(int WorkerCount, worker *Workers);
    
#endif
