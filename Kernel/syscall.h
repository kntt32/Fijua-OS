#ifndef INCLUDED_SYSCALL_H
#define INCLUDED_SYSCALL_H

typedef struct {
    uintn x;
    uintn y;

    uintn width;
    uintn height;

    ascii* buff;
    uintn buffSize;

    uintn cursor_startX;
    uintn cursor_startY;
    uintn cursor_endX;
    uintn cursor_endY;

    uintn scroll;

    uintn allowInput;
} App_Syscall_EditBox_Data;

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


sintn Syscall_DrawButton(uintn layerId, uintn x, uintn y, uintn width, uintn height, in const ascii* str);

sintn Syscall_DrawButton_Pushed(uintn layerId, uintn x, uintn y, uintn width, uintn height, in const ascii* str);

#endif
