#ifndef INCLUDED_KERNELINPUT_H
#define INCLUDED_KERNELINPUT_H

#include <types.h>
#include <efi.h>
#include <efi_loaded_image_protocol.h>

typedef struct {
    uintn mode;//1:rgb 2:bgr
    uintn startAddr;
    uintn scanlineWidth;
    uintn width;
    uintn height;
} GraphicData;

typedef struct {
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    EFI_HANDLE ImageHandle;
    GraphicData Graphic;
    struct {
        uintn PageCount;
        uintn* AvailableMemoryMap;//[PageCount] per 2 byte represent 1 pages'owner id (4KiB) (0:unavailable 1:available 2:kernel_heap <=3:owner_id)
    } Memory;
    void* tester;
} KernelInputStruct;

typedef int (KernelEntryPoint)(KernelInputStruct*);

#endif
