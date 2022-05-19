#ifndef AUDIO_C
#define AUDIO_C

#define COBJMACROS
#include <stdatomic.h>
#include <stdbool.h>
#include <xaudio2.h>

#include "stb_vorbis.h"

typedef HRESULT WINAPI co_uninitialize(void);
typedef HRESULT WINAPI co_initialize_ex(LPVOID, DWORD);
typedef HRESULT WINAPI xaudio2_create(IXAudio2 **, UINT32, XAUDIO2_PROCESSOR);

typedef struct com {
    HMODULE Lib;
    co_initialize_ex *CoInitializeEx;
    co_uninitialize *CoUninitialize;
} com;

typedef struct xaudio2 {
    /*XAudio2*/
    HMODULE Lib;
    xaudio2_create *Create;
    IXAudio2 *Engine;

    IXAudio2MasteringVoice *MasterVoice;

    IXAudio2VoiceCallback StreamCallback;
    IXAudio2SourceVoice *SourceVoice;

    /*Streaming*/
    HANDLE StreamStart;
    HANDLE StreamEnd;
    HANDLE StreamThread; 
    HANDLE BufferEndEvent;

    stb_vorbis *Vorbis;
    _Atomic bool IsPlaying;
} xaudio2;

bool CreateCom(com *Com);
void DestroyCom(com *Com);

bool CreateXAudio2(xaudio2 *XAudio2);
void DestroyXAudio2(xaudio2 *XAudio2);

bool PlayOgg(xaudio2 *XAudio2, const char *Path);

#endif
