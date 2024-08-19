#ifndef INCLUDED_MEMORY_H
#define INCLUDED_MEMORY_H

void Memory_Init(void);

void* Memory_AllocPages(uint16 taskId, uintn pages);

uintn Memory_Move(uint16 taskId, uint16 newTaskId, uintn pages, void* pageaddr);

uintn Memory_FreePages(uint16 taskId, uintn pages, void* pageaddr);

uintn Memory_FreeAll(uint16 taskId);

#endif
