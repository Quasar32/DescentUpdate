#include <stdatomic.h>
#include <stdbool.h>

#include "audio.h"
#include "error.h"
#include "procs.h"

#define OFFSET(Type, Member) ((size_t) &(((type *) 0)->Member))
#define CONTAINER(Ptr, Type, Member) ((type *) ((char *) (Ptr)) - OFFSET(Type, Member))

static void STDMETHODCALLTYPE StreamOnBufferEnd(
    [[maybe_unused]] IXAudio2VoiceCallback *This, 
    void *Context
) { 
    xaudio2 *XAudio2 = Context;
    SetEvent(XAudio2->BufferEndEvent);
    if(!atomic_load(&XAudio2->IsPlaying)) {
        IXAudio2SourceVoice_Stop(XAudio2->SourceVoice, 0, XAUDIO2_COMMIT_NOW);
        IXAudio2SourceVoice_FlushSourceBuffers(XAudio2->SourceVoice);
    }
}

static HRESULT SubmitSourceBuffer(
    IXAudio2SourceVoice *Voice, 
    const XAUDIO2_BUFFER *Buffer, 
    const XAUDIO2_BUFFER_WMA *WMA
) {
   return Voice->lpVtbl->SubmitSourceBuffer(Voice, Buffer, WMA); 
}

static void GetState(
    IXAudio2SourceVoice *Voice,
    XAUDIO2_VOICE_STATE* VoiceState, 
    UINT32 Flags
) {
    Voice->lpVtbl->GetState(Voice, VoiceState, Flags);
}

void STDMETHODCALLTYPE StreamStub1(IXAudio2VoiceCallback *) {}
void STDMETHODCALLTYPE StreamStub2(IXAudio2VoiceCallback *, void *) {}
void STDMETHODCALLTYPE StreamStub3(IXAudio2VoiceCallback *, void *, HRESULT) {}
void STDMETHODCALLTYPE StreamStub4(IXAudio2VoiceCallback *, UINT32) {}

static IXAudio2VoiceCallbackVtbl StreamCallbackVTBL = {
    .OnBufferEnd = StreamOnBufferEnd,
    .OnBufferStart = StreamStub2,
    .OnLoopEnd = StreamStub2,
    .OnStreamEnd = StreamStub1,
    .OnVoiceError = StreamStub3,
    .OnVoiceProcessingPassEnd = StreamStub1,
    .OnVoiceProcessingPassStart = StreamStub4
};

static const WAVEFORMATEX WaveFormat = {
    .wFormatTag = WAVE_FORMAT_PCM,
    .nChannels = 2,
    .nSamplesPerSec = 48000,
    .nAvgBytesPerSec = 192000,
    .nBlockAlign = 4,
    .wBitsPerSample = 16,
}; 
    
bool CreateCom(com *Com) {
    FARPROC Procs[2];
    *Com = (com) {
        .Lib = LoadProcs(
            "ole32.dll", 
            2, 
            (const char *[]) { 
                "CoInitializeEx", 
                "CoUninitialize" 
            },
            Procs
        ),
        .CoInitializeEx = (co_initialize_ex *) Procs[0],
        .CoUninitialize = (co_uninitialize *) Procs[1]
    };

    HRESULT Result = Com->CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if(FAILED(Result)) {
        FreeLibrary(Com->Lib);
        *Com = (com) {};
        return false;
    }
    return true;
}

void DestroyCom(com *Com) {
    if(Com->Lib) {
        Com->CoUninitialize();
        FreeLibrary(Com->Lib);
    }
    *Com = (com) {};
}

#define STREAM_BUFFER_COUNT 8
static void WaitForSamples(xaudio2 *XAudio2, HANDLE BufferEndEvent) {
    XAUDIO2_VOICE_STATE VoiceState;
    while(
        GetState(XAudio2->SourceVoice, &VoiceState, 0), 
        VoiceState.BuffersQueued >= STREAM_BUFFER_COUNT - 1
    ) {
        WaitForSingleObject(BufferEndEvent, INFINITE);
    }
}

static void WaitUntilEmpty(xaudio2 *XAudio2, HANDLE BufferEndEvent) {
    XAUDIO2_VOICE_STATE VoiceState;
    while(GetState(XAudio2->SourceVoice, &VoiceState, 0), VoiceState.BuffersQueued > 0) {
        WaitForSingleObject(BufferEndEvent, INFINITE);
    }
}

static DWORD StreamProc(LPVOID Ptr) {
    xaudio2 *XAudio2 = Ptr; 
    while(WaitForSingleObject(XAudio2->StreamStart, INFINITE) == WAIT_OBJECT_0) {
        short Bufs[STREAM_BUFFER_COUNT][3200];
        int BufI = 0;

        while(atomic_load(&XAudio2->IsPlaying)) {
            int Pairs = stb_vorbis_get_frame_short_interleaved(
                XAudio2->Vorbis, 
                2, 
                Bufs[BufI], 
                _countof(Bufs[BufI])
            );
            if(Pairs <= 0) break;

            XAUDIO2_BUFFER XBuf = {
                .Flags = XAUDIO2_END_OF_STREAM, 
                .pAudioData = (BYTE *) &Bufs[BufI],
                .AudioBytes = Pairs * 4, 
                .pContext = XAudio2
            }; 

            if(
                FAILED(SubmitSourceBuffer(XAudio2->SourceVoice, &XBuf, NULL)) ||
                FAILED(IXAudio2SourceVoice_Start(XAudio2->SourceVoice, 0, 0))
            ) {
                break;
            }

            BufI++;
            if(BufI >= (int) STREAM_BUFFER_COUNT) BufI = 0;

            WaitForSamples(XAudio2, XAudio2->BufferEndEvent);
        }
        WaitUntilEmpty(XAudio2, XAudio2->BufferEndEvent);
        stb_vorbis_close(XAudio2->Vorbis);
        XAudio2->Vorbis = NULL;
        SetEvent(XAudio2->StreamEnd);
    }
    return EXIT_SUCCESS;
}

bool CreateXAudio2(xaudio2 *XAudio2) {
    FARPROC Proc; 
    *XAudio2 = (xaudio2) {};

    bool Success = (
        (
            XAudio2->Lib = LoadProcsVersioned(
                3,
                (const char *[]) {
                    "XAudio2_9.dll",
                    "XAudio2_8.dll",
                    "XAudio2_7.dll"
                },
                1,
                (const char *[]) {"XAudio2Create"},
                &Proc
            )
        ) && (
            XAudio2->Create = (xaudio2_create *) Proc,
            SUCCEEDED(XAudio2->Create(&XAudio2->Engine, 0, XAUDIO2_DEFAULT_PROCESSOR))
        ) &&
        SUCCEEDED(
            IXAudio2_CreateMasteringVoice(
                XAudio2->Engine, 
                &XAudio2->MasterVoice,
                XAUDIO2_DEFAULT_CHANNELS,
                XAUDIO2_DEFAULT_SAMPLERATE,
                0,
                NULL,
                NULL,
                AudioCategory_GameEffects
            )
        ) &&
        (
            XAudio2->StreamCallback.lpVtbl = &StreamCallbackVTBL,
            SUCCEEDED(
                IXAudio2_CreateSourceVoice(
                    XAudio2->Engine, 
                    &XAudio2->SourceVoice,
                    &WaveFormat,
                    0,
                    XAUDIO2_DEFAULT_FREQ_RATIO,
                    &XAudio2->StreamCallback,
                    NULL,
                    NULL
                )
            )
        ) &&
        (XAudio2->StreamStart = CreateEvent(NULL, FALSE, FALSE, NULL)) &&
        (XAudio2->StreamEnd = CreateEvent(NULL, FALSE, TRUE, NULL)) &&
        (XAudio2->StreamThread = CreateThread(NULL, 0, StreamProc, XAudio2, 0, 0))
    );
    if(!Success) {
        DestroyXAudio2(XAudio2);
    }
    return true;
}

void DestroyXAudio2(xaudio2 *XAudio2) {
    if(XAudio2->StreamThread) CloseHandle(XAudio2->StreamThread); 
    if(XAudio2->StreamEnd) CloseHandle(XAudio2->StreamEnd);
    if(XAudio2->StreamStart) CloseHandle(XAudio2->StreamStart);
    if(XAudio2->Engine) IXAudio2_Release(XAudio2->Engine);
    if(XAudio2->Lib) FreeLibrary(XAudio2->Lib);
    *XAudio2 = (xaudio2) {};
}

bool PlayOgg(xaudio2 *XAudio2, const char *Path) {
    atomic_store(&XAudio2->IsPlaying, true);
    WaitForSingleObject(XAudio2->StreamEnd, INFINITE);
    XAudio2->Vorbis = stb_vorbis_open_filename(Path, NULL, NULL);
    if(!XAudio2->Vorbis) return false;

    SetEvent(XAudio2->StreamStart);
    return true;
}
