#include <stdio.h>
#include "error.hpp"

__attribute__((format(printf, 1, 2)))
BOOL LogError(const char *Format, ...) {
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

void MessageError(const char *Error) {
    MessageBox(NULL, Error, "Error", MB_ICONERROR); 
    if(!LogError(Error)) { 
        MessageBox(NULL, "LogError failed", "Error", MB_ICONERROR); 
    }
}

