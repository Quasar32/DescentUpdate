#include <stdio.h>
#include <windows.h>
#include <stdbool.h>
#include "descent.h"
#include "error.h"
#include "frame.h"

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

static uint32_t *FindButtonFromKey(size_t VKey) {
    static const uint8_t s_KeyUsed[COUNTOF_BT] = {
        VK_LEFT,
        VK_UP,
        VK_RIGHT,
        VK_DOWN,
    }; 

    for(int I = 0; I < COUNTOF_BT; I++) {
        if(VKey == s_KeyUsed[I]) {
            return &g_GameState.Buttons[I];
        } 
    }
    return NULL;
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

static bool ProcessMessages(HWND Window) {
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
            } return true;
        case WM_KEYUP:
            {
                size_t KeyI = Message.wParam;
                uint32_t *Key = FindButtonFromKey(KeyI);
                if(Key) {
                    *Key = 0;
                }
            } return true;
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
    frame Frame = CreateFrame(60.0F);

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

    /*InitGameState*/
    CreateGameState(&g_GameState); 

    /*MainLoop*/
    while(true) {
        g_GameState.FrameDelta = GetFrameDelta(&Frame);
        StartFrame(&Frame);

        if(!ProcessMessages(Window)) {
            break;
        } 
        UpdateGameState(&g_GameState);
        InvalidateRect(Window, NULL, FALSE);

        EndFrame(&Frame);
    }

    DestroyFrame(&Frame);

    return EXIT_SUCCESS;
}
