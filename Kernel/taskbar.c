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

sintn Taskbar_Main(void) {
    uintn layerId;

    App_Syscall_GetDisplaySize(&width, &height);

    if(App_Syscall_NewWindow(&(layerId), 0, -100, width, 132-33, "TaskBar")) {
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


//タスクバーへのマウス操作への応答
void Taskbar_RespondMouse(Task_Message* message, uintn layerId) {
    if(message->type != Task_Message_MouseLayerEvent) return;
    if(!message->data.MouseLayerEvent.leftButton) return;

    //shボタン
    if(0 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32
        && 132-33-32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 132-33) {
        //シェルを起動
        Functions_StartShell();
        return;
    }

    //flボタン
    if(32 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 64
        && 132-33-32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 132-33) {
        //ファイルマネージャーを起動
        App_Syscall_RunApp("app/filemanager.elf", sizeof("app/filemanager.elf"), "");
        return;
    }

    //pwrボタン
    if((sintn)(width-32) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width
        && (sintn)(132-33-32) <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < (sintn)(132-33)) {
        //シャットダウン
        Task_New(Taskbar_Shutdown, 0, "");
        return;
    }

    //他のウインドウへの操作
    if(layerPtr != NULL) {
        uintn xi = 0;
        uintn nameLenght = 0;
        for(sintn i=layerPtr->Window.count-1; 0<=i; i--) {
            if(layerPtr->Window.Data[i].layerId == layerId) continue;

            for(uintn k=0; k<16; k++) {
                if(layerPtr->Window.Data[i].name[k] == '\0') {
                    nameLenght = k;
                    break;
                }
            }

            if((sintn)(64+xi+2*8) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)(64+xi+2*8+nameLenght*8)) {
                if(xi == 0 && layerPtr->Window.Data[i].hiddenFlag == 0) {
                    Layer_Window_Hidden(layerPtr->Window.Data[i].layerId);
                }else {
                    Layer_Window_Focus(layerPtr->Window.Data[i].layerId);
                }
            }
            
            xi += (nameLenght+4)*8;
        }
    }

    return;
}


//タスクバーを再描画
void Taskbar_Flush(uintn layerId, const Layer* layer) {
    layerPtr = layer;

    Graphic_Color black = {0x10, 0x10, 0x10};
    App_Syscall_DrawSquare(layerId, 0, 0, width, 132-33, black);

    if(layer == NULL) return;

    Graphic_Color white = {0xff, 0xff, 0xff};
    Graphic_Color gray = {0xaa, 0xaa, 0xaa};

    uintn xi = 0;
    for(sintn i=layer->Window.count-1; 0<=i; i--) {
        if(layer->Window.Data[i].layerId == layerId) continue;
        
        uintn nameLenght = 0;
        if(layer->Window.Data[i].hiddenFlag) {
            for(uintn k=0; k<16; k++) {
                if(layer->Window.Data[i].name[k] == '\0') {
                    nameLenght = k;
                    break;
                }
                App_Syscall_DrawFont(layerId, 64+xi+(2+k)*8, 132-33-32+10, layer->Window.Data[i].name[k], gray);
            }
            App_Syscall_DrawSquare(layerId, 64+xi+2*8, 132-33-32, nameLenght*8, 3, gray);
        }else {
            for(uintn k=0; k<16; k++) {
                if(layer->Window.Data[i].name[k] == '\0') {
                    nameLenght = k;
                    break;
                }
                App_Syscall_DrawFont(layerId, 64+xi+(2+k)*8, 132-33-32+10, layer->Window.Data[i].name[k], white);
            }
            App_Syscall_DrawSquare(layerId, 64+xi+2*8, 132-33-32, nameLenght*8, 3, white);
        }
        xi += (nameLenght+4)*8;
    }

    App_Syscall_DrawSquare(layerId, 64, 132-33-32+16-5, 2, 10, gray);

    App_Syscall_DrawSquare(layerId, 32, 132-33-32+16-5, 2, 10, gray);

    App_Syscall_DrawFont(layerId, 10, 132-33-32+10, 'S', white);
    App_Syscall_DrawFont(layerId, 10+8, 132-33-32+10, 'h', white);
    App_Syscall_DrawFont(layerId, 42, 132-33-32+10, 'F', white);
    App_Syscall_DrawFont(layerId, 42+8, 132-33-32+10, 'l', white);

    App_Syscall_DrawSquare(layerId, width-34, 132-33-32+16-5, 2, 10, gray);
    App_Syscall_DrawFont(layerId, width-30, 132-33-32+10, 'P', white);
    App_Syscall_DrawFont(layerId, width-30+8, 132-33-32+10, 'w', white);
    App_Syscall_DrawFont(layerId, width-30+8*2, 132-33-32+10, 'r', white);

    return;
}