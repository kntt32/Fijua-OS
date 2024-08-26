#ifndef INCLUDED_SYSCALL_H
#define INCLUDED_SYSCALL_H

typedef struct {
    ascii* buff;
    uintn buffSize;

    uintn width;
    uintn height;

    uintn scroll;
    uintn cursorX;
    uintn cursorY;
} App_Syscall_TextBox_Data;

void Syscall_Init(void);

#endif
