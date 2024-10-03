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


typedef struct {
    uintn x;
    uintn y;
    uintn height;
    uintn offset;
    uintn page_height;
} App_Syscall_Scrollbar_Data;

void Syscall_Init(void);

#endif
