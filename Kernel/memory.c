#include <types.h>
#include <kernel.h>
#include "memory.h"
#include "functions.h"
#include "x64.h"
#include "console.h"


typedef enum {
    Memory_MemType_Unavailable,
    Memory_MemType_Available
} Memory_MemType;


extern KernelInputStruct* KernelInput;


static uintn   Memory_PageCount = 0;
static uint16* Memory_MemMap = NULL;

static uintn Memory_Mutex = 0;


void Memory_Init(void) {
    Memory_PageCount = KernelInput->Memory.PageCount;
    Memory_MemMap = (uint16*)(KernelInput->Memory.AvailableMemoryMap);
    if(Memory_MemMap[0x100000 >> 12] != Memory_MemType_Available) {
        Console_Print("Memory_Init: SyscallPage is'nt available\n");
        while(1) Hlt();
    }
    Memory_MemMap[0x100000 >> 12] = 2;

    return;
}


void* Memory_AllocPages(uint16 taskId, uintn pages) {
    /*
    taskId:
        0:firmware
        1:available
        2:kernel
        10~:app
    */
    if(taskId == Memory_MemType_Unavailable || taskId == Memory_MemType_Available) return NULL;
    if(pages == 0) return NULL;

    Mutex_Lock(&Memory_Mutex);

    uintn memareaPages = 0;
    for(uintn i=0; i<Memory_PageCount; i++) {
        if(Memory_MemMap[i] == Memory_MemType_Available) {
            memareaPages++;
        }else {
            memareaPages=0;
        }

        if(memareaPages == pages) {
            for(uintn k=i-pages+1; k<=i; k++) {
                Memory_MemMap[k] = taskId;
            }
            Mutex_UnLock(&Memory_Mutex);
            return (void*)((i-pages+1)<<12);
        }
    }

    Mutex_UnLock(&Memory_Mutex);

    return NULL;
}


//free pages
//if indecaded page isn't owned by ownerid, it is ignored.
uintn Memory_FreePages(uint16 taskId, uintn pages, void* pageaddr) {
    if(taskId == Memory_MemType_Unavailable || taskId == Memory_MemType_Available) return 1;
    if(pageaddr == NULL) return 2;
    if((((uintn)pageaddr)&0xfff) != 0) return 3;
    if(Memory_PageCount < (((uintn)pageaddr)>>12)+pages) return 4;
    if(pages == 0) return 0;

    Mutex_Lock(&Memory_Mutex);

    uintn pageCount = ((uintn)pageaddr)>>12;
    for(uintn i=0; i<pages; i++) {
        if(Memory_MemMap[i+pageCount] == taskId) {
            Memory_MemMap[i+pageCount] = Memory_MemType_Available;
        }
    }

    Mutex_UnLock(&Memory_Mutex);

    return 0;
}


uintn Memory_FreeAll(uint16 taskId) {
    if(taskId == Memory_MemType_Unavailable || taskId == Memory_MemType_Available) return 1;

    Mutex_Lock(&Memory_Mutex);

    for(uintn i=0; i<Memory_PageCount; i++) {
        if(Memory_MemMap[i] == taskId) {
            Memory_MemMap[i] = Memory_MemType_Available;
        }
    }

    Mutex_UnLock(&Memory_Mutex);

    return 0;
}




