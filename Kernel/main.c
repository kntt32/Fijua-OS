#include <types.h>
#include <efi.h>
#include <kernel.h>
#include "functions.h"
#include "graphic.h"
#include "console.h"
#include "timer.h"
#include "memory.h"
#include "queue.h"
#include "task.h"
#include "layer.h"
#include "mouse.h"
#include "keyboard.h"
#include "syscall.h"
#include "message.h"
#include "file.h"
#include "x64.h"

#include "taskbar.h"
#include "terminal.h"
#include "shell.h"

#include "test.h"

KernelInputStruct* KernelInput = NULL;

int Main(KernelInputStruct* kernelInput) {
    KernelInput = kernelInput;

    Graphic_Init();
    Console_Init();
    
    Memory_Init();
    Mouse_Init();
    Keyboard_Init();

    Task_Init();
    Layer_Init();

    Syscall_Init();
    Message_Init();
    
    Timer_Init();

    File_Init();

    uint16 taskbar = Task_New(Taskbar_Main, 0);
    Layer_Taskbar_SetTaskId(taskbar);

    Functions_StartShell();

/*
    Layer_Window_New(2, "testWindow", 10, 10, 300, 400);

    Layer_Window_New(2, "test2", 50, 50, 100, 60);
*/
    HltLoop();

    return 1;
}
