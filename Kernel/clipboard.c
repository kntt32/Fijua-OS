#include <types.h>
#include "clipboard.h"
#include "memory.h"

static uintn Clip_size = 0;
static ascii* Clip_data = NULL;

uintn Clip_Set(const ascii* data, uintn length) {
    if(Clip_size != 0) {
        Memory_FreePages(2, (Clip_size+0xfff)>>12, (void*)Clip_data);
        Clip_size = 0;
    }

    Clip_data = Memory_AllocPages(2, (length+0xfff)>>12);
    if(Clip_data == NULL) {
        return 1;
    }

    for(uintn i=0; i<length; i++) {
        Clip_data[i] = data[i];
    }
    Clip_size = length;

    return 0;
}


uintn Clip_Get(out ascii* buff, in out uintn* buffsize) {
    if(buffsize == NULL) return 1;
    if(buff == NULL) {
        *buffsize = Clip_size+1;
        return 1;
    }

    if(*buffsize < Clip_size+1) {
        *buffsize = Clip_size+1;
        return 2;
    }

    for(uintn i=0; i<Clip_size; i++) {
        buff[i] = Clip_data[i];
    }
    buff[Clip_size] = '\0';

    return 0;
}

