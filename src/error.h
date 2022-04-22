#ifndef ERROR_HPP
#define ERROR_HPP

#include <windows.h>

BOOL LogError(const char *Format, ...);

void MessageError(const char *Error);

#endif
