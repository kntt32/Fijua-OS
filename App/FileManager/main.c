#include <types.h>
#include "../../Kernel/graphic.h"
#include "../../Kernel/queue.h"
#include "../../Kernel/task.h"
#include "../../Kernel/file.h"
#include <app_x64.h>

sintn main() {
    App_Syscall_StdOut("Hello, World!\n", sizeof("Hello, World!\n"));

    App_Syscall_ExitStdIo();

    uintn layerId;
    if(App_Syscall_NewWindow(&layerId, 50, 50, 400, 300, "FileManager")) return 0;

    Task_Message message;
    while(1) {
        App_Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Quit:
                App_Syscall_Exit(0);
                break;
            case Task_Message_CloseWindow:
                App_Syscall_Exit(0);
                break;
            default:
                break;
        }
    }

    return 0;
}
