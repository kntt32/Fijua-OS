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

typedef struct {
    ascii path[DefaultBuffSize];
    uintn layerId;

    App_Syscall_TextBox_Data textBoxData;
} NotePad;

void NotePad_init(void);
void respondMouse(Task_Message* message);
void flush(void);
uintn NotePad_open(ascii path[DefaultBuffSize]);


NotePad notePad;

sintn main(ascii arg[32]) {
    if(arg[0] == '\0') {
        App_Syscall_Alert("No File Input", sizeof("No File Input"));
        return 1;
    }
    App_Syscall_ExitStdIo();

    NotePad_init();
    NotePad_open(arg);
    flush();

    Task_Message message;
    while(1) {
        App_Syscall_TextBox(notePad.layerId, 0, 32, &(notePad.textBoxData));
        App_Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_CloseWindow:
                App_Syscall_Exit(0);
            case Task_Message_Quit:
                App_Syscall_Exit(0);
            case Task_Message_MouseLayerEvent:
                respondMouse(&message);
            default:
                break;
        }
    }

    return 0;
}


void NotePad_init(void) {
    notePad.path[0] = '\0';
    if(App_Syscall_NewWindow(&(notePad.layerId), 100, 100, width, height, "NotePad")) {
        App_Syscall_Alert("Unknown Error", sizeof("Unknown Error"));
        App_Syscall_Exit(-1);
    }

    notePad.textBoxData.buffSize = 0;
    notePad.textBoxData.buff = NULL;
    notePad.textBoxData.width = width;
    notePad.textBoxData.height = height-32;
    notePad.textBoxData.scroll = 0;
    notePad.textBoxData.cursorX = 0;
    notePad.textBoxData.cursorY = 0;

    return;
}


uintn NotePad_open(ascii path[DefaultBuffSize]) {
    File_DirectoryEntry dirEnt;

    for(uintn i=0; i<DefaultBuffSize; i++) {
        notePad.path[i] = path[i];
        if(path[i] == '\0') break;
    }

    if(App_Syscall_GetDirEntryByPath(notePad.path, DefaultBuffSize, &dirEnt)) {
        App_Syscall_Alert("File Not Found", sizeof("File Not Found"));
        return 1;
    }

    App_Syscall_FreePages((notePad.textBoxData.buffSize + 0xfff)>>12, notePad.textBoxData.buff);
    notePad.textBoxData.buffSize = 0;
    notePad.textBoxData.buff = NULL;

    if(App_Syscall_AllocPage((dirEnt.size + 0xfff)>>12, (void**)&(notePad.textBoxData.buff))) {
        App_Syscall_Alert("Failed to Open", sizeof("Failed to Open"));
        App_Syscall_Exit(-1);
    }

    notePad.textBoxData.buffSize = ((dirEnt.size + 1 + 0xfff)>>12)<<12;

    if(App_Syscall_MMapFile(notePad.path, DefaultBuffSize, notePad.textBoxData.buffSize, notePad.textBoxData.buff)) {
        App_Syscall_Alert("File Not Found", sizeof("File Not Found"));
        return 2;
    }
    notePad.textBoxData.buff[dirEnt.size] = '\0';

    return 0;
}


void respondMouse(Task_Message* message) {
    //保存ボタン
    if(0 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32
        && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
        //save
        for(uintn i=0; i<notePad.textBoxData.buffSize; i++) {
            if(notePad.textBoxData.buff[i] == '\0') {
                App_Syscall_WriteFileFromMem(notePad.path, DefaultBuffSize, i+1, notePad.textBoxData.buff);
                return;
            }
        }
        App_Syscall_Alert("Failed", sizeof("Failed"));
    }

    return;
}


void flush(void) {
    App_Syscall_DrawSquare(notePad.layerId, 0, 0, width, 32, gray);

    App_Syscall_DrawSquare(notePad.layerId, 1, 1, 32-2, 32-2, ui_color);//save
    App_Syscall_DrawFont(notePad.layerId, 0, 10, 'S', black);
    App_Syscall_DrawFont(notePad.layerId, 8, 10, 'a', black);
    App_Syscall_DrawFont(notePad.layerId, 8*2, 10, 'v', black);
    App_Syscall_DrawFont(notePad.layerId, 8*3, 10, 'e', black);

    return;
}

