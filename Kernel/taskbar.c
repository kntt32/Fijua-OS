#include <types.h>
#include "graphic.h"
#include "queue.h"
#include "task.h"
#include "terminal.h"
#include "file.h"
#include "layer.h"
#include "x64.h"
#include "console.h"
#include "functions.h"
#include "syscall.h"
#include <app_x64.h>

void Taskbar_Flush(uintn layerId, const Layer* layer);
void Taskbar_RespondMouse(Task_Message* message, uintn layerId);

static uintn width;
static uintn height;
static const Layer* layerPtr = NULL;

static Graphic_Color gray = {0xaa, 0xaa, 0xaa};
static Graphic_Color black = {0x10, 0x10, 0x10};
static Graphic_Color white = {0xff, 0xff, 0xff};

sintn Taskbar_Main(void) {
    uintn layerId;

    App_Syscall_GetDisplaySize(&width, &height);

    if(App_Syscall_NewWindow(&(layerId), 0, -33, width, 32, "TaskBar")) {
        Console_Print("Taskbar_Main: Err");
        return 0;
    }
    Taskbar_Flush(layerId, NULL);

    Task_Message message;
    while(1) {
        App_Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Private_LayerEvent:
                Taskbar_Flush(layerId, message.data.Private_LayerEvent.layer);
                break;
            case Task_Message_MouseLayerEvent:
                Taskbar_RespondMouse(&message, layerId);
                break;
            default:
                break;
        }
    }
    return 0;
}


sintn Taskbar_Shutdown(void) {
    if(App_Syscall_Confirm("Will you Shutdown?", sizeof("Will you Shutdown?")) == 0) {
        App_Syscall_ShutDown();
    }
    return 0;
}


sintn Taskbar_GetLayerIndex(uintn layerId) {
    for(uintn i=0; i<layerPtr->Window.count; i++) {
        if(layerPtr->Window.Data[i].layerId == layerId) {
            return i;
        }
    }

    return -1;
}



//タスクバーへのマウス操作への応答
void Taskbar_RespondMouse(Task_Message* message, uintn layerId) {
    if(message->type != Task_Message_MouseLayerEvent) return;
    if(!message->data.MouseLayerEvent.leftButton) return;

    //shボタン
    if(0 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32
        && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
        //シェルを起動
        App_Syscall_DrawButton_Pushed(layerId, 2, 2, 28, 28, "");
        Functions_StartShell();
        return;
    }

    //flボタン
    if(32 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 64
        && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
        //ファイルマネージャーを起動
        App_Syscall_DrawButton_Pushed(layerId, 3+32, 2, 28, 28, "");
        App_Syscall_RunApp("app/filemanager.elf", sizeof("app/filemanager.elf"), "");
        return;
    }

    //pwrボタン
    if((sintn)(width-32) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width
        && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
        //シャットダウン
        App_Syscall_DrawButton_Pushed(layerId, width-32-1, 2, 28, 28, "");
        Task_New(Taskbar_Shutdown, 0, "");
        return;
    }

    //他のウインドウへの操作
    if(layerPtr != NULL) {
        uintn ix = 0;
        for(uintn i=0; i<layerPtr->Window.count; i++) {
            if(layerPtr->Window.idList[i] == layerId) continue;
            sintn index = Taskbar_GetLayerIndex(layerPtr->Window.idList[i]);
            if(index < 0) continue;
            if((sintn)((8*16+2)*ix+72) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)((8*16+2)*(ix+1)+72)) {
                App_Syscall_DrawButton_Pushed(layerId, (8*16+2)*ix+72, 2, 8*16, 28, layerPtr->Window.Data[index].name);
                Layer_Window_Focus(layerPtr->Window.idList[i]);
            }
            
            ix++;
        }
    }

    return;
}


//shellロゴ描画
static void Taskbar_draw_shellLogo(uintn layerId, uintn x, uintn y) {
    static uint8 logo[] = {
#include "shell_logo"
    };

    for(uintn i=0; i<28; i++) {
        for(uintn k=0; k<4; k++) {
            uint8 temp = logo[i*4+k];
            for(uintn j=0; j<8; j++) {
                if(temp & 0x80) {
                    App_Syscall_DrawSquare(layerId, x+k*8+j, y+i, 1, 1, gray);
                }
                temp <<= 1;
            }
        }
    }

    return;
}


//fileロゴ描画
static void Taskbar_draw_FileLogo(uintn layerId, uintn x, uintn y) {
    static uint8 logo[] = {
#include "file_logo"
    };

    for(uintn i=0; i<28; i++) {
        for(uintn k=0; k<4; k++) {
            uint8 temp = logo[i*4+k];
            for(uintn j=0; j<8; j++) {
                if(temp & 0x80) {
                    App_Syscall_DrawSquare(layerId, x+k*8+j, y+i, 1, 1, gray);
                }
                temp <<= 1;
            }
        }
    }

    return;
}


//pwrロゴ描画
//fileロゴ描画
static void Taskbar_draw_PwrLogo(uintn layerId, uintn x, uintn y) {
    static uint8 logo[] = {
#include "pwr_logo"
    };

    for(uintn i=0; i<28; i++) {
        for(uintn k=0; k<4; k++) {
            uint8 temp = logo[i*4+k];
            for(uintn j=0; j<8; j++) {
                if(temp & 0x80) {
                    App_Syscall_DrawSquare(layerId, x+k*8+j, y+i, 1, 1, gray);
                }
                temp <<= 1;
            }
        }
    }

    return;
}



//タスクバーを再描画
void Taskbar_Flush(uintn layerId, const Layer* layer) {
    layerPtr = layer;

    App_Syscall_DrawSquare(layerId, 0, 0, width, 32, black);

    if(layer == NULL) return;

    uintn ix = 0;
    for(uintn i=0; i<layerPtr->Window.count; i++) {
        if(layerPtr->Window.idList[i] == layerId) continue;
        sintn index = Taskbar_GetLayerIndex(layerPtr->Window.idList[i]);
        if(index < 0) continue;
        if(layerPtr->Window.Data[index].hiddenFlag) {
            App_Syscall_DrawButton_NotActive(layerId, (8*16+2)*ix+72, 2, 8*16, 28, layerPtr->Window.Data[index].name);
        }else {
            App_Syscall_DrawButton(layerId, (8*16+2)*ix+72, 2, 8*16, 28, layerPtr->Window.Data[index].name);
        }
        ix++;
    }

    App_Syscall_DrawSquare(layerId, 66, 16-5, 2, 10, gray);
    App_Syscall_DrawSquare(layerId, width-38, 16-5, 2, 10, gray);

    App_Syscall_DrawButton(layerId, 2, 2, 28, 28, "");
    Taskbar_draw_shellLogo(layerId, 2, 2);
    App_Syscall_DrawButton(layerId, 3+32, 2, 28, 28, "");
    Taskbar_draw_FileLogo(layerId, 3+32, 2);

    App_Syscall_DrawButton(layerId, width-32-1, 2, 28, 28, "");
    Taskbar_draw_PwrLogo(layerId, width-32+1, 2);

    return;
}