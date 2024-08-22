#include <elf.h>
#include <elfloader.h>

#define NULL ((void*)0)


static void ElfLoader_MemCopy(in const void* from, in const unsigned int size, out void* to);


//fill memoy zero from start
static void ElfLoader_Bss(in out void* start, in uintn size) {
    uint8* targetptr = (uint8*)start;

    for(uintn i=0; i<size; i++) {
        *targetptr = 0;
        targetptr++;
    }

    return;
}


//compare two string
static uintn ElfLoader_CmpStr(in char str1[], in char str2[]) {
    if(str1 == NULL || str2 == NULL) return 0;

    for(uintn i=0; 1; i++) {
        if(str1[i] == '\0') return i;
        if(str1[i] != str2[i]) return 0;
    }
}


//Get property about elf file
unsigned int ElfLoader_GetProperty(in const void* file, in optional void* loadAddr, out optional void** entryPointPtr, out optional uint16* machineTypePtr) {
    if(file == NULL) return 1;

    if(entryPointPtr != NULL) {
        switch(((Elf_Header*)file)->e_type) {
            case Elf_Header_ET_EXEC:
                *entryPointPtr = (void*)(((Elf_Header*)file)->e_entry);
                break;
            case Elf_Header_ET_DYN:
                *entryPointPtr = (void*)(((Elf_Header*)file)->e_entry + (uintn)loadAddr);
                break;
            default:
                return 2;
        }
    }
    if(machineTypePtr != NULL) *machineTypePtr = ((Elf_Header*)file)->e_machine;

    return 0;
}

void cac(uintn);


//check DYN
unsigned int ElfLoader_CheckDyn(in const void* file) {
    Elf_Header* elfHeader = (Elf_Header*)file;
    if(elfHeader->e_type == Elf_Header_ET_DYN) return 1;
    return 0;
}


//check elf
unsigned int ElfLoader_CheckElf(in const void* file) {
    const uint32* ptr = file;
    if(*ptr == 0x464c457f) return 1;
    return 0;
}


//Expand elf excutable file
unsigned int ElfLoader_Load(in const void* file, in uintn loadAddr) {
    if(file == NULL) return 1;

//load elf file
    Elf_Header* elfHeader = (Elf_Header*)file;
    Elf_ProgramHeader* programHeader = (void*)(elfHeader->e_phoffset + (uintn)file);
    uintn programHeader_entrySize = elfHeader->e_phentsize;
    uintn programHeader_number = elfHeader->e_phnum;

    //check loadAddr
    switch(elfHeader->e_type) {
        case Elf_Header_ET_EXEC:
            break;
        case Elf_Header_ET_DYN:
            break;
        default:
            return 3;
    }

    //expand elf
    for(unsigned int i=0; i<programHeader_number; i++) {
        if(programHeader->p_type == Elf_ProgramHeader_PT_LOAD) {
            ElfLoader_Bss((void*)(programHeader->p_vaddr+loadAddr), programHeader->p_memsz);
            //load
            ElfLoader_MemCopy((void*)(programHeader->p_offset+(uintn)file), programHeader->p_filesz, (void*)(programHeader->p_vaddr+loadAddr));
            //bss section
            //ElfLoader_Bss((void*)(programHeader->p_vaddr+programHeader->p_filesz+(uintn)loadAddr), programHeader->p_memsz-programHeader->p_filesz);
        }

        programHeader = (void*)((uintn)programHeader + programHeader_entrySize);
    }
    
    
    return 0;
}



unsigned int ElfLoader_GetLoadArea(in void* file, in out uintn* loadAddr, out uintn* size) {
    if(file == NULL || loadAddr == NULL || size == NULL) return 1;

    const Elf_ProgramHeader* programHeader = (const Elf_ProgramHeader*)(((uintn)(((Elf_Header*)file)->e_phoffset)) + (uintn)(file));
    const uintn programHeader_entrySize = (uintn)(((Elf_Header*)file)->e_phentsize);
    const uintn programHeader_number = (uintn)(((Elf_Header*)file)->e_phnum);
    if(programHeader == NULL) return 2;

    *size = 0;
    switch(((Elf_Header*)file)->e_type) {
        case Elf_Header_ET_DYN:
            for(uintn i=0; i<programHeader_number; i++) {
                if(programHeader->p_type == Elf_ProgramHeader_PT_LOAD) {
                    if(*size < programHeader->p_vaddr + programHeader->p_memsz) *size = programHeader->p_vaddr + programHeader->p_memsz;
                }
                programHeader = (Elf_ProgramHeader*)((uintn)(programHeader) + programHeader_entrySize);
            }
            break;
        case Elf_Header_ET_EXEC:
            for(uintn i=0; i<programHeader_number; i++) {
                if(programHeader->p_type == Elf_ProgramHeader_PT_LOAD) {
                    if(programHeader->p_vaddr < *loadAddr) *loadAddr = programHeader->p_vaddr;
                    if(*size < programHeader->p_vaddr + programHeader->p_memsz) *size = programHeader->p_vaddr + programHeader->p_memsz;
                }
                programHeader = (Elf_ProgramHeader*)((uintn)(programHeader) + programHeader_entrySize);
            }
            *size -= *loadAddr;
            break;
        default:
            return 3;
    }

    return 0;
}



static void ElfLoader_MemCopy(in const void* from, in const unsigned int size, out void* to) {
    if(from == NULL || to == NULL || size == 0) return;

    uint8* fromptr = (void*)from;
    uint8* toptr = (void*)to;
    unsigned int sizedec = size;

#if PLATFORM_64BIT
    uint64* fromptr64 = (void*)fromptr;
    uint64* toptr64 = (void*)toptr;
    for(unsigned int i=0; i<(size >> 3); i++) {
        *toptr64 = *fromptr64;
        toptr64++;
        fromptr64++;

        sizedec -= 8;
    }
    fromptr = (uint8*)fromptr64;
    toptr = (uint8*)toptr64;
#else
    uint32* fromptr32 = (void*)fromptr;
    uint32* toptr32 = (void*)toptr;
    for(unsigned int i=0; i<(size >> 2); i++) {
        *toptr32 = *fromptr32;
        toptr32++;
        fromptr32++;

        sizedec -= 4;
    }
    fromptr = (uint8*)fromptr32;
    toptr = (uint8*)toptr32;
#endif
    for(unsigned int i=0; i<sizedec; i++) {
        *toptr = *fromptr;
        toptr++;
        fromptr++;
    }

    return;
}