#include "worker.h"

#include <stdio.h>

DWORD WINAPI ThreadWorkerProc(LPVOID VoidWorker) {
    worker *Worker = (worker *) VoidWorker;

    while(WaitForSingleObject(Worker->StartEvent, INFINITE) == WAIT_OBJECT_0) {
        assert(Worker->Task != NULL && Worker->Data != NULL);

        Worker->Task(Worker->Data);
        Worker->Task = NULL;
        Worker->Data = NULL;
        SetEvent(Worker->EndEvent);
    }
    return 0UL;
}

void WorkerMultiWait(int WorkerCount, worker *Workers) {
    assert(WorkerCount < 1024);

    HANDLE EndEvents[WorkerCount]; 
    int TaskCount = 0;

    for(int I = 0; I < WorkerCount; I++) {
        if(Workers[I].Task) {
            EndEvents[TaskCount++] = Workers[I].EndEvent;
            SetEvent(Workers[I].StartEvent);
        }
    }
    WaitForMultipleObjects(TaskCount, EndEvents, TRUE, INFINITE);
}

void CreateWorker(worker *Worker) {
    *Worker = (worker) {
        .Thread = CreateThread(NULL, 0, ThreadWorkerProc, Worker, 0, NULL), 
        .StartEvent = CreateEvent(NULL, FALSE, FALSE, NULL),
        .EndEvent = CreateEvent(NULL, FALSE, FALSE, NULL)
    };
}

void DestroyWorker(worker *Worker) {
    CloseHandle(Worker->EndEvent); 
    CloseHandle(Worker->StartEvent); 
    TerminateThread(Worker->Thread, 0);
}
