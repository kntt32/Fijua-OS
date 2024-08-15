#ifndef INCLUDED_ELFLOADER_H
#define INCLUDED_ELFLOADER_H

#include <types.h>

typedef struct {
    void* startAddr;
    uintn memSize;
} ElfLoader_MemLoadArea;


unsigned int ElfLoader_GetProperty(in const void* file, in optional void* loadAddr, out optional void** entryPointPtr, out optional uint16* machineTypePtr);

unsigned int ElfLoader_Load(in const void* file, in uintn loadAddr);
unsigned int ElfLoader_GetLoadArea(in void* file, in out uintn* loadAddr, out uintn* size);

#endif
