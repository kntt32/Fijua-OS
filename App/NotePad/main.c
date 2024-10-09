#include <types.h>
#include "../../Kernel/graphic.h"
#include "../../Kernel/queue.h"
#include "../../Kernel/task.h"
#include "../../Kernel/file.h"
#include "../../Kernel/syscall.h"
#include <app_x64.h>

#define DefaultBuffSize (128)

const Graphic_Color white = {0xff, 0xff, 0xff};
const Graphic_Color black = {0x00, 0x00, 0x00};
const Graphic_Color gray = {0x88, 0x88, 0x88};
const Graphic_Color blue = {0x55, 0x55, 0xff};
const Graphic_Color ui_color = {0xf0, 0xf0, 0xf0};

const uintn width = 600;
const uintn height = 450;

uintn layerId;
ascii path[DefaultBuffSize] = "";

App_Syscall_EditBox_Data editboxData = {0, 32, width-16, height-32, NULL, 0, 0, 0, 0, 0, 0, 1, 1};


void flush() {
    App_Syscall_DrawButton(layerId, 1, 1, 48-2, 32-2, "Save");
    App_Syscall_DrawButton(layerId, 49, 1, 48-2, 32-2, "Help");

    App_Syscall_EditBox_Draw(layerId, &editboxData);

    return;
}


sintn main(ascii arg[32]) {
    if(arg[0] == '\0') {
        App_Syscall_Alert("notepad: No File Input", sizeof("notepad: No File Input"));
        return 1;
    }

    for(uintn i=0; i<DefaultBuffSize; i++) {
        path[i] = arg[i];
        if(arg[i] == '\0') break;
    }
    path[DefaultBuffSize-1] = '\0';

    File_DirectoryEntry dirEntry;
    if(App_Syscall_GetDirEntryByPath(path, DefaultBuffSize, &dirEntry)) {
        App_Syscall_Alert("notepad: Invalid Input", sizeof("notepad: Invalid Input"));
        return 2;
    }

    if(App_Syscall_AllocPage((dirEntry.size*2 + 1 + 0xfff)>>12, (void**)&(editboxData.buff))) {
        App_Syscall_Alert("notepad: Failed 1", sizeof("notepad: Failed 1"));
        return 3;
    }
    editboxData.buffSize = ((dirEntry.size*2 + 1 + 0xfff)>>12)<<12;

    if(App_Syscall_MMapFile(path, DefaultBuffSize, editboxData.buffSize, editboxData.buff)) {
        App_Syscall_Alert("notepad: Failed 2", sizeof("notepad: Failed 2"));
        return 4;
    }
    editboxData.buff[editboxData.buffSize-1] = '\0';

    if(App_Syscall_NewWindow(&layerId, 100, 100, width, height, path)) {
        return 5;
    }

    flush();

    while(1) {
        App_Syscall_EditBox_Response(layerId, editboxData.x, editboxData.y, &editboxData);

        Task_Message message;
        App_Syscall_ReadMessage(&message);

        switch(message.type) {
            case Task_Message_CloseWindow:
                return 0;
            case Task_Message_Quit:
                return 0;
            case Task_Message_MouseLayerEvent:
                if(message.data.MouseLayerEvent.leftButton) {
                    //Save
                    if(0 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 48
                        && 0 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 32) {
                        App_Syscall_DrawButton_Pushed(layerId, 1, 1, 48-2, 32-2, "Save");
                        for(uintn i=0; i<editboxData.buffSize; i++) {
                            if(editboxData.buff[i] == '\0') {
                                App_Syscall_WriteFileFromMem(path, DefaultBuffSize, i+1, editboxData.buff);
                            }
                        }
                        flush();
                    }
                    //Help
                    if(48 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 48+48
                        && 0 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 32) {
                        App_Syscall_DrawButton_Pushed(layerId, 49, 1, 48-2, 32-2, "Help");
                        //Do Something
                        flush();
                    }
                }
                break;
            default:
                break;
        }
    }

    return 0;
}



