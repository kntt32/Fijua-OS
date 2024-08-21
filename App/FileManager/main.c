#include <types.h>
#include "../../Kernel/graphic.h"
#include "../../Kernel/queue.h"
#include "../../Kernel/task.h"
#include "../../Kernel/file.h"
#include <app_x64.h>

#define DefaultBuffSize (128)

uintn width = 400;
uintn height = 300;
uintn layerId;
ascii path[DefaultBuffSize] = "app";

void flush(void);

sintn main(ascii arg[32]) {
    App_Syscall_ExitStdIo();

    if(App_Syscall_NewWindow(&layerId, 50, 50, width, height, "FileManager")) return 0;

    flush();

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


void flush(void) {
    Graphic_Color black = {0x00, 0x00, 0x00};
    Graphic_Color gray = {0x88, 0x88, 0x88};
    //パスバー、戻るボタン、アクションバー描画
    App_Syscall_DrawSquare(layerId, 0, 0, width, 64, gray);
    //パスバー
    Graphic_Color pathBar_color = {0xf0, 0xf0, 0xf0};
    App_Syscall_DrawSquare(layerId, 32+1, 32+1, width-32*3-2, 32-2, pathBar_color);
    App_Syscall_DrawFont(layerId, 32+8, 32+10, 'p', black);
    App_Syscall_DrawFont(layerId, 32+8+8, 32+10, 'a', black);
    App_Syscall_DrawFont(layerId, 32+8+8*2, 32+10, 't', black);
    App_Syscall_DrawFont(layerId, 32+8+8*3, 32+10, 'h', black);
    App_Syscall_DrawFont(layerId, 32+8+8*4, 32+10, ':', black);

    for(uintn i=0; i<DefaultBuffSize; i++) {
        if(path[i] == '\0') break;
        App_Syscall_DrawFont(layerId, 32+8+8*(i+5), 32+10, path[i], black);
    }

    

    uintn buffCount = 0;
    App_Syscall_GetFileList(path, DefaultBuffSize, &buffCount, NULL);
    File_DirectoryEntry buff[buffCount];
    if(App_Syscall_GetFileList(path, DefaultBuffSize, &buffCount, buff)) {
        //Pathが存在しない
    }


    return;
}
