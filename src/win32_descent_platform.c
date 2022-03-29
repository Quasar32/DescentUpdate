#ifndef WIN32_DESCENT_PLATFORM_H
#define WIN32_DESCENT_PLATFORM_H

#include "descent_platform.h"

static struct file *HandleToFile(HANDLE Handle) {
    return (struct file *) ((uintptr_t) Handle + 1);
}

static HANDLE FileToHandle(struct file *File) {
    return (HANDLE) ((uintptr_t) File - 1);
}

static struct file *Win32OpenFile(const char *Path) {
    return HandleToFile(
        CreateFile(
            Path,
            GENERIC_READ,
            FILE_SHARE_READ, 
            NULL, 
            OPEN_EXISTING, 
            0, 
            NULL
        )
    );
}

static bool Win32CloseFile(struct file *File) {
    return !!CloseHandle(FileToHandle(File));
}

static int32_t Win32GetFileSize(struct file *File) {
    DWORD FileSize = GetFileSize(FileToHandle(File), NULL);
    return FileSize > INT32_MAX ? FileSize : -1; 
}

static int32_t Win32ReadFile(struct file *File, void *Buf, int32_t BufSize) {
    DWORD BytesRead;
    return (
        ReadFile(FileToHandle(File), Buf, BufSize, &BytesRead, NULL) ? 
            BytesRead : 
            -1;
    );
}

static int32_t Win32SeekFile(struct file *File, int32_t Loc) {
    DWORD FileLoc = SetFilePointer(FileToHandle(File), Loc, NULL, FILE_BEGIN);
    return FileLoc == INVALID_SET_FILE_POINTER ? -1 : FileLoc;
}

static const struct platform g_Platform = {
    .OpenFile = Win32OpenFile,
    .CloseFile = Win32CloseFile,
    .GetFileSize = Win32GetFileSize,
    .ReadFile = Win32ReadFile,
    .SeekFile = Win32SeekFile
};
#endif
