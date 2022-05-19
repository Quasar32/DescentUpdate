#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include <xinput.h>

#include "audio.h"
#include "descent.h"
#include "error.h"
#include "frame.h"
#include "procs.h"

typedef DWORD WINAPI xinput_get_state(DWORD, XINPUT_STATE *);

typedef struct xinput {
    HMODULE Lib;
    xinput_get_state *GetState;
} xinput; 

#define MY_WS_FLAGS (WS_VISIBLE | WS_SYSMENU | WS_CAPTION) 

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

static game_state g_GameState;

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

typedef DWORD WINAPI xinput_get_state(DWORD, XINPUT_STATE *);

static DWORD XInputGetStateStub(
    [[maybe_unused]] DWORD UserIndex, 
    [[maybe_unused]] XINPUT_STATE *State
) {
    return ERROR_DEVICE_NOT_CONNECTED; 
}

static xinput LoadXInput(void) {
    xinput XInput;
    FARPROC Proc;
    XInput.Lib = LoadProcsVersioned(
        3,
        (const char *[]) {
            "xinput1_4.dll",
            "xinput1_3.dll",
            "xinput9_1_0.dll"
        },
        1,
        (const char *[]) {
            "XInputGetState" 
        },
        &Proc
    );
    XInput.GetState = XInput.Lib ? (xinput_get_state *) Proc : XInputGetStateStub;
    return XInput;
}

static int CappedInc(int Val) {
    return Val < INT32_MAX ? Val + 1: Val;
}

static bool UpdateButton(int I, bool IsDown) {
    if(I < 0 || I >= COUNTOF_BT) {
        return false;
    }

    g_GameState.Buttons[I] = ( 
        IsDown ? 
            CappedInc(g_GameState.Buttons[I]) : 
            0
    );
    return true;
}

static bool XInputToButton(const xinput *XInput) {
    XINPUT_STATE State;
    if(XInput->GetState(0, &State) != ERROR_SUCCESS) {
        return false;
    } 

    if(State.Gamepad.sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        State.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT; 
    }
    if(State.Gamepad.sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        State.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT; 
    }

    if(State.Gamepad.sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        State.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN; 
    }
    if(State.Gamepad.sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) {
        State.Gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP; 
    }

    static const WORD s_ButtonUsed[COUNTOF_BT] = {
        XINPUT_GAMEPAD_DPAD_LEFT,
        XINPUT_GAMEPAD_DPAD_UP,
        XINPUT_GAMEPAD_DPAD_RIGHT,
        XINPUT_GAMEPAD_DPAD_DOWN
    }; 

    for(int I = 0; I < COUNTOF_BT; I++) {
        UpdateButton(I, State.Gamepad.wButtons & s_ButtonUsed[I]);
    }

    return true;
}

static int FindButtonFromKey(size_t VKey) {
    static const uint8_t s_KeyUsed[COUNTOF_BT] = {
        VK_LEFT,
        VK_UP,
        VK_RIGHT,
        VK_DOWN,
    }; 

    for(int I = 0; I < COUNTOF_BT; I++) {
        if(VKey == s_KeyUsed[I]) {
            return I;
        } 
    }
    return -1;
}

static BOOL ToggleFullscreen(HWND Window) {
    static RECT RestoreRect = {}; 
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

static bool ProcessMessages(HWND Window, xaudio2 *XAudio2) {
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
                } else if(KeyI == 'X') {
                    atomic_store(&XAudio2->IsPlaying, false);
                } else {
                    UpdateButton(FindButtonFromKey(KeyI), true);
                }
            } return true;
        case WM_KEYUP:
            {
                size_t KeyI = Message.wParam;
                UpdateButton(FindButtonFromKey(KeyI), false);
            } return true;
        case WM_SYSKEYDOWN:
            memset(g_GameState.Buttons, 0, sizeof(g_GameState.Buttons));
            break; /*continue processing*/
        case WM_QUIT:
            return false;
        }

        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }
    return true;
}

static LRESULT WndProc(
    HWND Window, 
    UINT Message, 
    WPARAM WParam, 
    LPARAM LParam
) {
    switch(Message) {
    case WM_DESTROY:
        PostQuitMessage(EXIT_SUCCESS);
        return 0;
    case WM_KILLFOCUS: 
        memset(g_GameState.Buttons, 0, sizeof(g_GameState.Buttons));
        return 0;
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
                g_GameState.Pixels,
                &g_DIBInfo,
                DIB_RGB_COLORS
            );
            EndPaint(Window, &Paint);
        } return 0;
    }
    return DefWindowProc(Window, Message, WParam, LParam);
}

int WINAPI WinMain(
    HINSTANCE Instance, 
    [[maybe_unused]] HINSTANCE PrevInstance, 
    [[maybe_unused]] LPSTR CmdLine, 
    [[maybe_unused]] int CmdShow
) {
    /*InitAudio*/
    com Com;
    xaudio2 XAudio2;
    if(CreateCom(&Com)) {
        CreateXAudio2(&XAudio2);
    }
    PlayOgg(&XAudio2, "../music/z3r0-8bitSyndrome.ogg");

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

    /*InitMisc*/
    frame Frame = CreateFrame(60.0F);
    xinput XInput = LoadXInput();
    CreateGameState(&g_GameState); 

    /*MainLoop*/
    while(true) {
        g_GameState.FrameDelta = GetFrameDelta(&Frame);
        StartFrame(&Frame);

        if(!ProcessMessages(Window, &XAudio2)) {
            break;
        } 

        XInputToButton(&XInput);

        UpdateGameState(&g_GameState);
        InvalidateRect(Window, NULL, FALSE);

        EndFrame(&Frame);
    }

    DestroyFrame(&Frame);
    DestroyXAudio2(&XAudio2); 
    DestroyCom(&Com);

    return EXIT_SUCCESS;
}
