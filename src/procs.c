#include "procs.h"

HMODULE LoadProcs(
    const char *Path, 
    size_t ProcCount, 
    const char *ProcNames[static ProcCount],
    FARPROC Procs[static ProcCount]
) {
    HMODULE Lib = LoadLibrary(Path); 
    if(!Lib) {
        return NULL;
    }

    for(size_t I = 0; I < ProcCount; I++) {
        Procs[I] = GetProcAddress(Lib, ProcNames[I]);
        if(!Procs[I]) {
            FreeLibrary(Lib);
            return NULL;
        }
    }

    return Lib;
}

HMODULE LoadProcsVersioned(
    size_t PathCount,
    const char *Paths[static PathCount], 
    size_t ProcCount, 
    const char *ProcNames[static ProcCount],
    FARPROC Procs[static ProcCount]
) {
    for(size_t I = 0; I < PathCount; I++) {
        HMODULE Lib = LoadProcs(Paths[I], ProcCount, ProcNames, Procs);
        if(Lib) {
            return Lib;
        }
    }
    return NULL;
}
