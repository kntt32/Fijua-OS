#ifndef INCLUDED_SUBROUTINE_H
#define INCLUDED_SUBROUTINE_H

#include <kernel.h>

int Functions_SPrintIntX(uintn number, uintn buffsize, CHAR16 buff[]);
void err();

void get_EFI_SIMPLE_FILE_SYSTEM_PROTOCOL();
void get_EFI_FILE_PROTOCOL_root_handle();
void get_EFI_FILE_PROTOCOL_kernelfile_handle();

void get_kernelfile_size();
void allocate_memory_to_load_kernelfile();
void load_kernelfile_to_buffer();

void get_memarea_to_expand_kernelfile();
void allocate_pages_to_expand_kernelfile();
void expand_kernelfile();

void release_resource();
void disable_dogtimer();
void get_framebuffer_for_kernel();
void get_memory_for_kernel();

int wrapper_callKernel(KernelInputStruct*);


#endif
