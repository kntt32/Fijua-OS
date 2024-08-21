#ifndef INCLUDED_TERMINAL_X64_H
#define INCLUDED_TERMINAL_X64_H

#define KEYBOARD_SCANCODE_NULL (0x00)
#define KEYBOARD_SCANCODE_UPARROW (0x01)
#define KEYBOARD_SCANCODE_DOWNARROW (0x02)
#define KEYBOARD_SCANCODE_RIGHTARROW (0x03)
#define KEYBOARD_SCANCODE_LEFTARROW (0x04)
#define KEYBOARD_SCANCODE_HOME (0x05)
#define KEYBOARD_SCANCODE_END (0x06)
#define KEYBOARD_SCANCODE_INSERT (0x07)
#define KEYBOARD_SCANCODE_DELETE (0x08)
#define KEYBOARD_SCANCODE_PAGEUP (0x09)
#define KEYBOARD_SCANCODE_PAGEDOWN (0x0a)
#define KEYBOARD_SCANCODE_ESC (0x17)

sintn App_Syscall_NewWindow(out uintn* layerId, in sintn x, in sintn y, in uintn width, in uintn height, in ascii title[]);

sintn App_Syscall_CloseWindow(in uintn layerId);

sintn App_Syscall_YieldCpu(void);

sintn App_Syscall_DrawSquare(in uintn layerId, in uintn x, in uintn y, in uintn width, in uintn height, in Graphic_Color color);

sintn App_Syscall_DrawFont(in uintn layerId, in uintn x, in uintn y, in ascii asciicode, in Graphic_Color color);

sintn App_Syscall_ReadMessage(out Task_Message* message);

sintn App_Syscall_CheckMessage(out Task_Message* message);

sintn App_Syscall_SendIPCMessage(in uint16 taskId, in const ascii message[32]);

sintn App_Syscall_Exit(sintn retcode);

sintn App_Syscall_GetStdIoTaskId(out uint16* taskId);

sintn App_Syscall_StdOut(in const ascii str[], uintn count);

sintn App_Syscall_StdIn(out ascii str[], uintn strBuffSize);

sintn App_Syscall_StdOut_Cls(void);

sintn App_Syscall_GetFileList(const ascii path[], uintn pathLength, uintn* buffCount, File_DirectoryEntry buff[]);

sintn App_Syscall_GetDirEntryByPath(const ascii path[], uintn pathLength, File_DirectoryEntry* buff);

sintn App_Syscall_MMapFile(const ascii path[], uintn pathLength, uintn buffSize, void* buff);

sintn App_Syscall_WriteFileFromMem(const ascii path[], uintn pathLength, uintn buffSize, void* buff);

sintn App_Syscall_RemoveFile(const ascii path[], uintn pathLength);

sintn App_Syscall_MkDir(const ascii path[], uintn pathLength);

sintn App_Syscall_ShutDown(void);

sintn App_Syscall_AllocPage(uintn pages, void** pageAddr);

sintn App_Syscall_FreePages(uintn pages, void* pageAddr);

sintn App_Syscall_RunApp(const ascii path[], uintn pathLength, optional ascii arg[32]);

sintn App_Syscall_ExitStdIo(void);

sintn App_Syscall_Confirm(const ascii* question, uintn strlength);

sintn App_Syscall_GetThisTaskId(out uint16* taskId);

sintn App_Syscall_GetDisplaySize(optional out uintn* width, optional out uintn* height);

#endif
