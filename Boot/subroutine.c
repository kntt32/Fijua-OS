#include <types.h>
#include <efi.h>
#include <efi_types.h>
#include <efi_simple_file_system_protocol.h>
#include <efi_file_info.h>
#include <efi_graphics_output_protocol.h>
#include <efi_loaded_image_protocol.h>
#include <kernel.h>

#include <elfloader.h>
#include "subroutine.h"

#define NULL ((void*)0)


extern KernelInputStruct kernelInput;
extern EFI_SYSTEM_TABLE* SysTbl;
extern EFI_HANDLE ImgHdl;
extern EFI_LOADED_IMAGE_PROTOCOL* LddImg;

extern EFI_STATUS status;
extern uintn tempUintn;
extern uintn* tempUintnptr;
extern uint8* tempUint8ptr;
extern uint16* tempUint16ptr;
extern EFI_PHYSICAL_ADDRESS tempPhysicalAddress;

extern KernelEntryPoint* entryPoint;


EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* efiSimpleFileSystemProtocol_volume = NULL;
EFI_FILE_PROTOCOL* efiFileProtocol_root = NULL;
EFI_FILE_PROTOCOL* efiFileProtocol_kernelfile = NULL;
unsigned int kernelSize = 0;
char* buff_kernelfile = NULL;

uintn loadKernelBase = 0x200000;

int Functions_SPrintIntX(uintn number, uintn buffsize, CHAR16 buff[]) {
    uintn n = 0;
    for(sintn i=buffsize-2; 0<=i; i--) {
        n = number & 0xf;
        buff[i] = (n<10)?(n+L'0'):(n+L'a'-10);
        number >>= 4;
    }
    buff[buffsize-1] = L'\0';
    
    return 0;
}



void err() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Err");
    while(1) SysTbl->BootServices->Stall(1000);
}


void get_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Getting EFI_SIMPLE_FILE_SYSTEM_PROTOCOL\n\r");
    EFI_GUID efiSimpleFileSystemProtocol_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    efiSimpleFileSystemProtocol_volume = NULL;
    status = SysTbl->BootServices->HandleProtocol(LddImg->DeviceHandle, &efiSimpleFileSystemProtocol_guid, (VOID**)&efiSimpleFileSystemProtocol_volume);
    if(status) err();
    return;
}

void get_EFI_FILE_PROTOCOL_root_handle() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Getting EFI_FILE_PROTOCOL:root handle\n\r");
    status = efiSimpleFileSystemProtocol_volume->OpenVolume(efiSimpleFileSystemProtocol_volume, &efiFileProtocol_root);
    if(status) err();
    return;
}

void get_EFI_FILE_PROTOCOL_kernelfile_handle() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Getting EFI_FILE_PROTOCOL:kernelfile handle\n\r");
    status = efiFileProtocol_root->Open(efiFileProtocol_root, &efiFileProtocol_kernelfile, L"kernel.elf", EFI_FILE_MODE_READ, 0);
    if(status) err();
    return;
}

void get_kernelfile_size() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Getting kernelfile size\n\r");
    UINTN fileInfo_kernelfile_size = 0;
    EFI_GUID efiFileInfoID_guid = EFI_FILE_INFO_ID;
    efiFileProtocol_kernelfile->GetInfo(efiFileProtocol_kernelfile, &efiFileInfoID_guid, &fileInfo_kernelfile_size, NULL);
    char fileInfo_kernelfile_buff[fileInfo_kernelfile_size];
    EFI_FILE_INFO* fileInfo_kernelfile = (void*)fileInfo_kernelfile_buff;
    status = efiFileProtocol_kernelfile->GetInfo(efiFileProtocol_kernelfile, &efiFileInfoID_guid, &fileInfo_kernelfile_size, fileInfo_kernelfile);
    if(status) err();
    kernelSize = fileInfo_kernelfile->FileSize;
    return;
}

void allocate_memory_to_load_kernelfile() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Allocating memory to load kernelfile\n\r");
    buff_kernelfile = NULL;
    status = SysTbl->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, ((kernelSize+0xfff)>>12), (EFI_PHYSICAL_ADDRESS*)&buff_kernelfile);
    if(status) err();
    return;
}

void load_kernelfile_to_buffer() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Loading kernelfile to buffer\n\r");
    UINTN copyof_kernelSize = kernelSize;
    status = efiFileProtocol_kernelfile->Read(efiFileProtocol_kernelfile, &copyof_kernelSize, buff_kernelfile);
    if(status) err();
    return;
}

void allocate_pages_to_expand_kernelfile() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Allocating pages to expand kernelfile\n\r");
    uintn loadKernelSize = 0;
    status = ElfLoader_GetLoadArea(buff_kernelfile, &loadKernelBase, &loadKernelSize);
    if(status) err();

    status = SysTbl->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, ((loadKernelSize+0xfff)>>12), (EFI_PHYSICAL_ADDRESS*)&loadKernelBase);
    if(status) err();

    return;
}

void expand_kernelfile() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Expanding kernelfile\n\r");
    status = ElfLoader_Load(buff_kernelfile, loadKernelBase);
    if(status) err();

    ElfLoader_GetProperty(buff_kernelfile, (void*)loadKernelBase, (void**)&entryPoint, NULL);
    return;
}

void release_resource() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Releasing resource\n\r");
    efiFileProtocol_kernelfile->Close(efiFileProtocol_kernelfile);
    efiFileProtocol_root->Close(efiFileProtocol_root);
    SysTbl->BootServices->FreePages((EFI_PHYSICAL_ADDRESS)buff_kernelfile, ((kernelSize+0xfff)>>12));
    return;
}

void disable_dogtimer() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Disable dogtimer\n\r");
    status =  SysTbl->BootServices->SetWatchdogTimer(0, 0x10000, 0, NULL);
    if(status) err();
    return;
}

void get_framebuffer_for_kernel() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Getting framebuffer for kernel\n\r");
    EFI_GUID efiGraphicsOutputProtocol_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* efiGraphicsOutputProtocol_interface = NULL;
    status = SysTbl->BootServices->LocateProtocol(&efiGraphicsOutputProtocol_guid, NULL, (VOID**)&efiGraphicsOutputProtocol_interface);
    if(status) err();
    switch(efiGraphicsOutputProtocol_interface->Mode->Info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            kernelInput.Graphic.mode = 1;
            break;
        case PixelBlueGreenRedReserved8BitPerColor :
            kernelInput.Graphic.mode = 2;
            break;
        default:
            SysTbl->ConOut->OutputString(SysTbl->ConOut, L"This Graphic Mode is Unsupported.\n\r");
            err();
    }
    kernelInput.Graphic.startAddr = efiGraphicsOutputProtocol_interface->Mode->FrameBufferBase;
    kernelInput.Graphic.scanlineWidth = efiGraphicsOutputProtocol_interface->Mode->Info->PixelsPerScanLine;
    kernelInput.Graphic.width = efiGraphicsOutputProtocol_interface->Mode->Info->HorizontalResolution;
    kernelInput.Graphic.height = efiGraphicsOutputProtocol_interface->Mode->Info->VerticalResolution;
    return;
}

void get_memory_for_kernel() {
    SysTbl->ConOut->OutputString(SysTbl->ConOut, L"Getting memory for Kernel\n\r");
        //get ram size
        SysTbl->ConOut->OutputString(SysTbl->ConOut, L"  Getting ramsize\n\r");
        EFI_MEMORY_DESCRIPTOR* memoryMap;
        uintn memoryMapSize = 0;
        uintn mapKey;
        uintn descriptorSize;
        uint32 descriptorVersion;
        SysTbl->BootServices->GetMemoryMap(&memoryMapSize, NULL, &mapKey, &descriptorSize, &descriptorVersion);
        status = SysTbl->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, (memoryMapSize+descriptorSize+0xfff)>>12, (EFI_PHYSICAL_ADDRESS*)&memoryMap);
        if(status) err();
        uintn memoryMap_AllocatedSize = ((memoryMapSize+descriptorSize+0xfff)>>12)<<12;
        memoryMapSize = memoryMap_AllocatedSize;
        status = SysTbl->BootServices->GetMemoryMap(&memoryMapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
        if(status) err();
        uintn ramSize = 0;
        EFI_MEMORY_DESCRIPTOR* targetMemDescriptor = memoryMap;
        for(uintn i=0; i<memoryMapSize/descriptorSize; i++) {
            tempUintn = (uintn)(targetMemDescriptor->PhysicalStart) + (targetMemDescriptor->NumberOfPages << 12);
            if(targetMemDescriptor->Type == EfiConventionalMemory && ramSize < tempUintn) {
                ramSize = tempUintn;
            }

            targetMemDescriptor = (EFI_MEMORY_DESCRIPTOR*)((EFI_PHYSICAL_ADDRESS)targetMemDescriptor + descriptorSize);
        }

        //set availableRamMap
        SysTbl->ConOut->OutputString(SysTbl->ConOut, L"  Setting available ram map\n\r");
        kernelInput.Memory.PageCount = ramSize>>12;
        status = SysTbl->BootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, (((ramSize*2)>>12)+0xfff)>>12, (EFI_PHYSICAL_ADDRESS*)&(kernelInput.Memory.AvailableMemoryMap));
        if(status) err();
        tempUintnptr = kernelInput.Memory.AvailableMemoryMap;
        for(uintn i=0; i<((((ramSize*2)>>12)+0xfff)>>12)<<(12-TYPES_UINTN_LN2_SIZE); i++) {
            *tempUintnptr = 0;
            tempUintnptr++;
        }
        memoryMapSize = memoryMap_AllocatedSize;
        status = SysTbl->BootServices->GetMemoryMap(&memoryMapSize, memoryMap, &mapKey, &descriptorSize, &descriptorVersion);
        if(status) err();
        targetMemDescriptor = memoryMap;
        tempUint16ptr = (uint16*)(kernelInput.Memory.AvailableMemoryMap);
        for(uintn i=0; i<memoryMapSize/descriptorSize; i++) {
            if(targetMemDescriptor->Type == EfiConventionalMemory && ((0x100000 <= targetMemDescriptor->PhysicalStart) || (targetMemDescriptor->PhysicalStart <= 0x100000 && 0x1000000 < targetMemDescriptor->PhysicalStart+targetMemDescriptor->NumberOfPages*4096))) {//allocate memory larger than 0x500000
                tempPhysicalAddress = targetMemDescriptor->PhysicalStart;
                status = SysTbl->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, targetMemDescriptor->NumberOfPages, &tempPhysicalAddress);
                if(status) continue;
                for(uintn k=(uintn)(targetMemDescriptor->PhysicalStart >> 12); k<(uintn)(targetMemDescriptor->PhysicalStart >> 12) + targetMemDescriptor->NumberOfPages; k++) {
                    tempUint16ptr[k] = 1;
                }
            }
            targetMemDescriptor = (EFI_MEMORY_DESCRIPTOR*)((EFI_PHYSICAL_ADDRESS)targetMemDescriptor + descriptorSize);
        }

    SysTbl->BootServices->FreePages((EFI_PHYSICAL_ADDRESS)memoryMap, memoryMap_AllocatedSize>>12);
}


