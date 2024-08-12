#include <efi.h>
#include <efi_loaded_image_protocol.h>

#include <kernel.h>

#include "subroutine.h"

#define NULL ((void*)0)


KernelInputStruct kernelInput;
EFI_SYSTEM_TABLE* SysTbl = NULL;
EFI_HANDLE ImgHdl = NULL;
EFI_LOADED_IMAGE_PROTOCOL* LddImg = NULL;


EFI_STATUS status = 0;
uintn tempUintn = 0;
uintn* tempUintnptr = NULL;
uint8* tempUint8ptr = NULL;
uint16* tempUint16ptr = NULL;
EFI_PHYSICAL_ADDRESS tempPhysicalAddress = 0;

KernelEntryPoint* entryPoint = NULL;

void cac(uintn n) {
    CHAR16 a[2];
    a[0] = L'0' + (uint16)n;
    a[1] = L'\0';
    SysTbl->ConOut->OutputString(SysTbl->ConOut, a);
}

void __chkstk() {}

void a(uintn n1, uintn n2, uintn n3, uintn n4, uintn n5) {
    CHAR16 chartempbuff[16];
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"A::\n\r");

    Functions_SPrintIntX(n1, 16, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"\n\r");
    
    Functions_SPrintIntX(n2, 16, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"\n\r");

    Functions_SPrintIntX(n3, 16, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"\n\r");

    Functions_SPrintIntX(n4, 16, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"\n\r");

    Functions_SPrintIntX(n5, 16, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"\n\r");

    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"\n\r");
}

EFI_STATUS efi_main(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable) {
    SysTbl = SystemTable;
    ImgHdl = ImageHandle;
    EFI_GUID efiLoadedImageProtocol_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    status = SysTbl->BootServices->HandleProtocol(ImgHdl, &efiLoadedImageProtocol_guid, (VOID**)&LddImg);
    if(status) err();

    //console test
    SysTbl->ConOut->ClearScreen(SysTbl->ConOut);
    SysTbl->ConOut->EnableCursor(SysTbl->ConOut, 1);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Hello, World!!\n\r");

    get_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL();
    get_EFI_FILE_PROTOCOL_root_handle();
    get_EFI_FILE_PROTOCOL_kernelfile_handle();

    get_kernelfile_size();
    allocate_memory_to_load_kernelfile();
    load_kernelfile_to_buffer();

    allocate_pages_to_expand_kernelfile();
    expand_kernelfile();

    release_resource();
    disable_dogtimer();
    get_framebuffer_for_kernel();
    get_memory_for_kernel();
    kernelInput.LoadedImage = LddImg;
    kernelInput.ImageHandle = ImgHdl;

    //run kernel
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Starting Kernel...\n\r");
    kernelInput.tester = (void*)a;
    int retcode = entryPoint(&kernelInput);
    CHAR16 chartempbuff[16];
    Functions_SPrintIntX(retcode, 16, chartempbuff);
    SysTbl->ConOut->OutputString(SysTbl->ConOut, chartempbuff);


    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Kernel returned\n\r");

    while(1) SysTbl->BootServices->Stall(1000);

    return 0;
}
