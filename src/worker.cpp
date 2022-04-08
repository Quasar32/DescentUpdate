#include "worker.hpp"
#include "error.hpp"

DWORD WINAPI Worker::ThreadWorkerProc(LPVOID ThisWorker) {
    Worker *This = (Worker *) ThisWorker;
    This->Update();
    return 0UL;
}

void Worker::Update() {
    while(WaitForSingleObject(StartEvent, INFINITE) == WAIT_OBJECT_0) {
        assert(ToRun != nullptr);
        ToRun->Run();
        ToRun = NULL;
        SetEvent(EndEvent);
    }
}

void Worker::MultiWait(int WorkerCount, Worker *Workers) {
    assert(WorkerCount < 1024);

    HANDLE EndEvents[WorkerCount]; 
    int TaskCount = 0;

    for(int I = 0; I < WorkerCount; I++) {
        if(Workers[I].ToRun) {
            EndEvents[TaskCount++] = Workers[I].EndEvent;
            SetEvent(Workers[I].StartEvent);
        }
    }
    WaitForMultipleObjects(TaskCount, EndEvents, TRUE, INFINITE);
}

Worker::Worker() {
    Thread = CreateThread(NULL, 0, Worker::ThreadWorkerProc, this, 0, NULL); 
    StartEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    EndEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

Worker::~Worker() {
    CloseHandle(EndEvent); 
    CloseHandle(StartEvent); 
    TerminateThread(Thread, 0);
}

void Worker::Schedule(Task &ToRun) {
    this->ToRun = &ToRun;
} 

