#include <stdio.h>
#include <windows.h>
#include "descent.h"
#include "win32_descent_platform.c"

#define MY_WS_FLAGS (WS_VISIBLE | WS_SYSMENU | WS_CAPTION) 

typedef MMRESULT WINAPI winmm_func(UINT);

static const BITMAPINFO g_DIBInfo = {
    .bmiHeader = {
        .biSize = sizeof(g_DIBInfo.bmiHeader),
        .biWidth = DIB_WIDTH,
        .biHeight = DIB_HEIGHT,
        .biPlanes = 1,
        .biBitCount = 32,
        .biCompression = BI_RGB
    }
};

static struct game_state *g_GameState;

__attribute__((format(printf, 1, 2)))
static BOOL LogError(const char *Format, ...) {
    /*OpenErrorHandle*/
    static HANDLE s_ErrorHandle = INVALID_HANDLE_VALUE;
    if(s_ErrorHandle == INVALID_HANDLE_VALUE) {
        s_ErrorHandle = CreateFile(
            "win32_error.log",
            FILE_APPEND_DATA,
            FILE_SHARE_READ, 
            NULL, 
            OPEN_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL, 
            NULL
        );
        if(s_ErrorHandle == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
    }

    /*GetTimeString*/
    SYSTEMTIME Time;
    GetLocalTime(&Time);

    char Error[1024];
    int DateLength = sprintf_s(
        Error, 
        _countof(Error),
        "%04hu-%02hu-%02huT%02hu:%02hu:%02hu.%03hu ", 
        Time.wYear, 
        Time.wMonth, 
        Time.wDay, 
        Time.wHour, 
        Time.wMinute, 
        Time.wSecond, 
        Time.wMilliseconds
    );
    if(DateLength < 0) {
        return FALSE; 
    }

    /*AppendFormatString*/
    va_list FormatArgs;
    va_start(FormatArgs, Format);

    char *ErrorPtr = &Error[DateLength];
    int CharsLeft = (int) _countof(Error) - DateLength;
    int FormatLength = vsprintf_s(ErrorPtr, CharsLeft, Format, FormatArgs); 

    va_end(FormatArgs);
    if(FormatLength < 0) {
        return FALSE;
    }

    /*WriteError*/
    ErrorPtr[FormatLength] = '\n';
    int ToWrite = DateLength + FormatLength + 1;
    DWORD Written;
    return (
        WriteFile(s_ErrorHandle, Error, ToWrite, &Written, NULL) &&
        ToWrite == (int) Written 
    );
}

static void MessageError(const char *Error) {
    MessageBox(NULL, Error, "Error", MB_ICONERROR); 
    if(!LogError(Error)) { 
        MessageBox(NULL, "LogError failed", "Error", MB_ICONERROR); 
    }
}

[[nodiscard]]
static HMODULE LoadProcs(
    const char *LibName, 
    size_t ProcCount, 
    const char *ProcNames[],
    FARPROC Procs[]
) {
    HMODULE Library = LoadLibrary(LibName);
    if(!Library) {
        return NULL;
    }
    for(size_t I = 0; I < ProcCount; I++) {
        Procs[I] = GetProcAddress(Library, ProcNames[I]);
        if(!Procs[I]) {
            FreeLibrary(Library);
            return NULL; 
        } 
    }
    return Library;
} 

static int64_t GetPerfFreq(void) {
    LARGE_INTEGER PerfFreq;
    QueryPerformanceFrequency(&PerfFreq);
    return PerfFreq.QuadPart;
}

static int64_t GetPerfCounter(void) {
    LARGE_INTEGER PerfCounter;
    QueryPerformanceCounter(&PerfCounter);
    return PerfCounter.QuadPart;
}

static int64_t GetDeltaCounter(int64_t BeginCounter) {
    return GetPerfCounter() - BeginCounter;
}

static float ConvertCounterToSeconds(int64_t DeltaCounter, int64_t PerfFreq) {
    return (float) DeltaCounter / PerfFreq; 
} 

static void SetWindowState(
    HWND Window, 
    DWORD Style, 
    int X,
    int Y,
    int Width,
    int Height
) {
    SetWindowLongPtr(Window, GWL_STYLE, Style);
    SetWindowPos(
        Window, 
        HWND_TOP, 
        X, 
        Y, 
        Width, 
        Height, 
        SWP_FRAMECHANGED
    );
}

static uint32_t *FindButtonFromKey(size_t VKey) {
    static const uint8_t s_KeyUsed[COUNTOF_BT] = {
        VK_LEFT,
        VK_UP,
        VK_RIGHT,
        VK_DOWN,
    }; 

    for(int I = 0; I < COUNTOF_BT; I++) {
        if(VKey == s_KeyUsed[I]) {
            return &g_GameState->Buttons[I];
        } 
    }
    return NULL;
}

static BOOL ToggleFullscreen(HWND Window) {
    static RECT RestoreRect = {0}; 
    static BOOL IsFullscreen = FALSE;

    /*RestoreFromFullscreen*/
    if(IsFullscreen) { 
        if(ChangeDisplaySettings(NULL, 0) != DISP_CHANGE_SUCCESSFUL) {
            return FALSE;
        }
        SetWindowState(
            Window, 
            MY_WS_FLAGS, 
            RestoreRect.left, 
            RestoreRect.top,
            DIB_WIDTH,
            DIB_HEIGHT
        ); 
        ShowCursor(TRUE);
        IsFullscreen = FALSE;
        return TRUE;
    } 

    /*MakeFullscreen*/
    GetWindowRect(Window, &RestoreRect);

    DEVMODE DevMode = {
        .dmSize = sizeof(DevMode),
        .dmFields = DM_PELSWIDTH | DM_PELSHEIGHT,
        .dmPelsWidth = DIB_WIDTH,
        .dmPelsHeight = DIB_HEIGHT
    };
    LONG DispState = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
    if(DispState != DISP_CHANGE_SUCCESSFUL) {
        return FALSE;
    }
    SetWindowState(Window, WS_POPUP | WS_VISIBLE, 0, 0, DIB_WIDTH, DIB_HEIGHT); 
    ShowCursor(FALSE);
    IsFullscreen = TRUE;
    return TRUE;
}

static LRESULT WndProc(
    HWND Window, 
    UINT Message, 
    WPARAM WParam, 
    LPARAM LParam
) {
    switch(Message) {
    case WM_DESTROY:
        {
            PostQuitMessage(EXIT_SUCCESS);
        } return 0;
    case WM_KILLFOCUS: 
        { 
            memset(g_GameState->Buttons, 0, sizeof(g_GameState->Buttons));
        } return 0;
    case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            SetDIBitsToDevice(
                DeviceContext,
                0,
                0,
                DIB_WIDTH,
                DIB_HEIGHT,
                0,
                0,
                0U,
                DIB_HEIGHT,
                g_GameState->Pixels,
                &g_DIBInfo,
                DIB_RGB_COLORS
            );
            EndPaint(Window, &Paint);
        } return 0;
    }
    return DefWindowProc(Window, Message, WParam, LParam);
}

/*LoadPlatform*/
int WINAPI WinMain(
    HINSTANCE Instance, 
    [[maybe_unused]] HINSTANCE PrevInstance, 
    [[maybe_unused]] LPSTR CmdLine, 
    [[maybe_unused]] int CmdShow
) {
    /*SetUpTiming*/
    const int64_t PerfFreq = GetPerfFreq(); 
    const int64_t FinalDeltaCounter = PerfFreq / 60LL;

    /*AllocGameState*/
    g_GameState = (struct game_state *) VirtualAlloc( 
        NULL, 
        sizeof(*g_GameState), 
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );
    if(!g_GameState) {
        MessageError("VirtualAlloc failed");
        return EXIT_FAILURE;
    }

    /*InitWindowClass*/
    WNDCLASS WindowClass = {
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = WndProc,
        .hInstance = Instance,
        .hCursor = LoadCursor(NULL, IDI_APPLICATION),
        .lpszClassName = "GameWindowClass"
    }; 
    if(!RegisterClass(&WindowClass)) {
        MessageError("RegisterClass failed");
        return EXIT_FAILURE;
    }

    /*InitWindow*/
    RECT WindowRect = {
        .right = DIB_WIDTH, 
        .bottom = DIB_HEIGHT 
    };
    AdjustWindowRect(&WindowRect, MY_WS_FLAGS, FALSE);

    HWND Window = CreateWindow(
        WindowClass.lpszClassName, 
        "Game", 
        MY_WS_FLAGS,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL,
        NULL,
        Instance,
        NULL
    );
    if(!Window) {
        MessageError("CreateWindow failed"); 
        return EXIT_FAILURE;
    }

    /*LoadWinmm*/
    union {
        FARPROC Procs[2];
        struct {
            winmm_func *TimeBeginPeriod;
            winmm_func *TimeEndPeriod;
        };
    } WinmmProcs;

    BOOL IsGranular = FALSE;
    LPCSTR WinmmProcNames[] = {
        "timeBeginPeriod",
        "timeEndPeriod"
    };

    HMODULE WinmmLibrary = LoadProcs(
        "winmm.dll",
        _countof(WinmmProcNames),
        WinmmProcNames,
        WinmmProcs.Procs
    );
    if(WinmmLibrary) {
        IsGranular = (WinmmProcs.TimeBeginPeriod(1) == TIMERR_NOERROR); 
        if(!IsGranular) {
            FreeLibrary(WinmmLibrary);
            WinmmLibrary = NULL;
            LogError("timeBeginPeriod failed"); 
        }
    } else {
        LogError("LoadProcs failed: winmm.dll"); 
    }

    /*LoadGameCode*/
    union {
        FARPROC Proc; 
        game_update *Update; 
    } GameProcs;

    LPCSTR GameProcNames[] = {
        "GameUpdate" 
    }; 
    HMODULE GameLibrary = LoadProcs(
        "descent.dll", 
        _countof(GameProcNames),
        GameProcNames, 
        &GameProcs.Proc
    );
    if(!GameLibrary) {
        LogError("LoadProcs failed: descent.dll"); 
    }

    /*FirstFrameConsiderations*/
    g_GameState->FrameDelta = 1.0F / 60.0F; 

    /*MainLoop*/
    bool IsRunning = 1;
    while(IsRunning) {
        int64_t BeginCounter = GetPerfCounter();

        /*ProcessMessages*/
        MSG Message;
        while(PeekMessage(&Message, NULL, 0U, 0U, PM_REMOVE)) {
            switch(Message.message) {
            case WM_KEYDOWN:
                {
                    size_t KeyI = Message.wParam;
                    if(KeyI == VK_F11) { 
                        if(!ToggleFullscreen(Window)) {
                            MessageError("ToggleFullscreen failed"); 
                        }
                    } else {
                        uint32_t *Key = FindButtonFromKey(Message.wParam);
                        if(Key && *Key < INT32_MAX) {
                            (*Key)++;
                        }
                    }
                } break;
            case WM_KEYUP:
                {
                    size_t KeyI = Message.wParam;
                    uint32_t *Key = FindButtonFromKey(KeyI);
                    if(Key) {
                        *Key = 0;
                    }
                } break;
            case WM_QUIT:
                {
                    IsRunning = 0;
                } break;
            default:
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                } break;
            }
        }

        /*UpdateGameCode*/
        if(GameLibrary) {
            GameProcs.Update(g_GameState, &g_Platform);
        }

        /*UpdateFrame*/
        InvalidateRect(Window, NULL, FALSE);

        /*SleepTillNextFrame*/
        int64_t DeltaCounter = GetDeltaCounter(BeginCounter);
        if(DeltaCounter < FinalDeltaCounter) {
            if(IsGranular) {
                int64_t RemainCounter = FinalDeltaCounter - DeltaCounter;
                DWORD SleepMS = (DWORD) (1000LL * RemainCounter / PerfFreq);
                if(SleepMS > 0UL) {
                    Sleep(SleepMS);
                }
            }
            do {
                DeltaCounter = GetDeltaCounter(BeginCounter); 
            } while(DeltaCounter < FinalDeltaCounter);
        }

        /*UpdateFrameDelta*/
        g_GameState->FrameDelta = ConvertCounterToSeconds(DeltaCounter, PerfFreq);
    }

    /*CleanUp*/
    if(WinmmLibrary) {
        WinmmProcs.TimeEndPeriod(1);
    }
    return EXIT_SUCCESS;
}
