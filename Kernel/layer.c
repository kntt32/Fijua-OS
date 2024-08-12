#include <types.h>
#include <kernel.h>
#include "graphic.h"
#include "layer.h"
#include "memory.h"
#include "mouse.h"
#include "functions.h"
#include "console.h"
#include "queue.h"
#include "task.h"
#include "font.h"
#include "message.h"

#define Layer_TaskId (2)

extern KernelInputStruct* KernelInput;

static Layer layer;

static const Graphic_Color background_color = {0x2d, 0x38, 0x81};
static const Graphic_Color window_titleBar_backgroundColor = {0xe0, 0xe0, 0xe0};
static const Graphic_Color window_shadow_color = {0x00, 0x00, 0x00};
static const Graphic_Color window_titleBar_closeButton_color = {0xf0, 0x56, 0x56};
static const Graphic_Color window_titleBar_hiddenButton_color = {0xc0, 0xc0, 0xc0};
static const Graphic_Color window_titleBar_titleText_color = {0x30, 0x50, 0x50};
static const sintn window_titleBar_height = 32;
static const sintn window_shadow_overThick = 1;
static const sintn window_shadow_underThick = 2;

static sintn Layer_Window_GetIndex(uintn layerId) {
    for(uintn i=0; i<layer.Window.count; i++) {
        if(layer.Window.Data[i].layerId == layerId) return i;
    }
    return -1;
}

//Layerを初期化
void Layer_Init(void) {
    Console_Print("Layer_Init: Initializing...\n");

    layer.changedFlag = 0;
    layer.isDrawingFlag = 0;

    layer.drawBackgroundFlag = 1;

    //init Layer.Window
    layer.Window.count = 0;
    layer.Window.pages = 0;
    layer.Window.Data = NULL;

    //init Layer.Mouse
    layer.Mouse.Draw.x = 0;
    layer.Mouse.Draw.y = 0;
    layer.Mouse.Draw.width = mouseCursor_width;
    layer.Mouse.Draw.height = mouseCursor_height;
    layer.Mouse.Draw.oldx = 0;
    layer.Mouse.Draw.oldy = 0;

    layer.Mouse.Drag.x = 0;
    layer.Mouse.Drag.y = 0;
    layer.Mouse.leftButton = 0;
    layer.Mouse.oldLeftButton = 0;

    return;
}


static void Layer_Update_WindowState(void) {
    Layer_Window* targetWindow;

    if(layer.Mouse.oldLeftButton == 0 && layer.Mouse.leftButton != 0) {
        targetWindow = layer.Window.Data + layer.Window.count - 1;

        for(sintn i=layer.Window.count-1; 0<=i; i--) {
            if(targetWindow->hiddenFlag) {
                targetWindow --;
                continue;
            }

            //ウインドウを最前面に移動
            if((targetWindow->Draw.x <= layer.Mouse.Draw.oldx && layer.Mouse.Draw.oldx < targetWindow->Draw.x + (sintn)(targetWindow->Draw.width))
                && (targetWindow->Draw.y <= layer.Mouse.Draw.oldy && layer.Mouse.Draw.oldy < targetWindow->Draw.y + (sintn)(targetWindow->Draw.height))) {
                Layer_Window_Focus(layer.Window.Data[i].layerId);
                break;
            }
            
            targetWindow--;
        }
    }

    if(0 < layer.Window.count) {
        targetWindow = layer.Window.Data + layer.Window.count - 1;

        if(layer.Mouse.oldLeftButton != 0 && layer.Mouse.leftButton == 0) {
            //ウインドウの非表示
            if((targetWindow->Draw.x+(sintn)window_shadow_overThick+1 <= layer.Mouse.Draw.oldx && layer.Mouse.Draw.oldx < targetWindow->Draw.x+(sintn)window_shadow_overThick+1+(sintn)window_titleBar_height-2)
                && (targetWindow->Draw.y+(sintn)window_shadow_overThick+1 <= layer.Mouse.Draw.oldy && layer.Mouse.Draw.oldy < targetWindow->Draw.y+(sintn)window_shadow_overThick+1+window_titleBar_height-2)) {
                targetWindow->hiddenFlag = 1;
                layer.drawBackgroundFlag = 1;
                if(1 < layer.Window.count) {
                    Layer_Window_Focus(layer.Window.Data[layer.Window.count-2].layerId);
                }
            }

            //ウィンドウの消去
            if((targetWindow->Draw.x+(sintn)targetWindow->Draw.width-(sintn)window_shadow_underThick-1-((sintn)window_titleBar_height-2) <= layer.Mouse.Draw.oldx && layer.Mouse.Draw.oldx < targetWindow->Draw.x+(sintn)targetWindow->Draw.width-(sintn)window_shadow_underThick-1)
                && (targetWindow->Draw.y+window_shadow_overThick+1 <= layer.Mouse.Draw.oldy && layer.Mouse.Draw.oldy < targetWindow->Draw.y+window_shadow_overThick+1+window_titleBar_height-2)) {
                Task_Message message;
                message.type = Task_Message_CloseWindow;
                message.data.CloseWindow.layerId = targetWindow->layerId;
                Message_EnQueue(targetWindow->taskId, &message);
            }
        }

        //ウインドウの移動
        if(!targetWindow->hiddenFlag
            && layer.Mouse.leftButton != 0 && !(layer.Mouse.Drag.x == 0 && layer.Mouse.Drag.y == 0)
            && (targetWindow->Draw.x + (sintn)window_shadow_overThick + (sintn)window_titleBar_height <= layer.Mouse.Draw.oldx && layer.Mouse.Draw.oldx < targetWindow->Draw.x + (sintn)targetWindow->Draw.width - (sintn)window_shadow_underThick - (sintn)window_titleBar_height)
            && (targetWindow->Draw.y + (sintn)window_shadow_overThick <= layer.Mouse.Draw.oldy && layer.Mouse.Draw.oldy < targetWindow->Draw.y + (sintn)window_shadow_overThick + (sintn)window_titleBar_height)) {
            
            targetWindow->Draw.x += layer.Mouse.Drag.x;
            targetWindow->Draw.y += layer.Mouse.Drag.y;
        }
    }

    layer.Mouse.Drag.x = 0;
    layer.Mouse.Drag.y = 0;
    layer.Mouse.oldLeftButton = layer.Mouse.leftButton;
    layer.Mouse.leftButton = 0;


    return;
}


static void Layer_Update_SetDrawArea_Background(uint8* layer_redrawFlag_Or, uintn blockXCount, uintn blockYCount) {
    uint8 background_redrawFlag[blockXCount*blockYCount];
    if(layer.drawBackgroundFlag) {
        for(uintn i=0; i<blockXCount*blockYCount; i++) {
            background_redrawFlag[i] = 1;
        }
        layer.drawBackgroundFlag = 0;
    }else {
        for(uintn i=0; i<blockXCount*blockYCount; i++) {
            background_redrawFlag[i] = 0;
        }
    }

    Layer_Window* targetWindow;
    uintn startX, startY, endX, endY;

    //上位レイヤ移動なら移動元ブロックに再描画フラグ
    targetWindow = layer.Window.Data - 1;
    for(uintn i=0; i<layer.Window.count; i++) {
        targetWindow++;
        if(targetWindow->hiddenFlag) continue;

        if(targetWindow->Draw.oldx == targetWindow->Draw.x && targetWindow->Draw.oldy == targetWindow->Draw.y) continue;

        startX = (targetWindow->Draw.oldx < 0)?(0):((uintn)targetWindow->Draw.oldx >> 6);
        startY = (targetWindow->Draw.oldy < 0)?(0):((uintn)targetWindow->Draw.oldy >> 6);
        endX = (targetWindow->Draw.oldx + (sintn)targetWindow->Draw.width + 63 < 0)?(0):((uintn)(targetWindow->Draw.oldx + (sintn)targetWindow->Draw.width + 63) >> 6);
        endY = (targetWindow->Draw.oldy + (sintn)targetWindow->Draw.height + 63 < 0)?(0):((uintn)(targetWindow->Draw.oldy + (sintn)targetWindow->Draw.height + 63) >> 6);
        for(uintn y=startY; y<endY && y<blockYCount; y++) {
            for(uintn x=startX; x<endX && x<blockXCount; x++) {
                background_redrawFlag[x+y*blockXCount] = 1;
            }
        }
    }

    //マウスレイヤ
    if(!(layer.Mouse.Draw.oldx == layer.Mouse.Draw.x && layer.Mouse.Draw.oldy == layer.Mouse.Draw.y)) {
        startX = (layer.Mouse.Draw.oldx < 0)?(0):((uintn)(layer.Mouse.Draw.oldx) >> 6);
        startY = (layer.Mouse.Draw.oldy < 0)?(0):((uintn)(layer.Mouse.Draw.oldy) >> 6);
        endX = (layer.Mouse.Draw.oldx + (sintn)layer.Mouse.Draw.width + 63 < 0)?(0):((uintn)(layer.Mouse.Draw.oldx + (sintn)layer.Mouse.Draw.width + 63) >> 6);
        endY = (layer.Mouse.Draw.oldy + (sintn)layer.Mouse.Draw.height + 63 < 0)?(0):((uintn)(layer.Mouse.Draw.oldy + (sintn)layer.Mouse.Draw.height + 63) >> 6);

        for(uintn y=startY; y<endY && y<blockYCount; y++) {
            for(uintn x=startX; x<endX && x<blockXCount; x++) {
               background_redrawFlag[x+y*blockXCount] = 1;
            }
        }
    }

    //上位レイヤの非表示ブロックの再描画フラグをマスク
    targetWindow = layer.Window.Data - 1;
    for(uintn i=0; i<layer.Window.count; i++) {
        targetWindow ++;
        if(targetWindow->hiddenFlag) continue;

        startX = (targetWindow->Draw.x + 63 < 0)?(0):((uintn)(targetWindow->Draw.x + 63) >> 6);
        startY = (targetWindow->Draw.y + 63 < 0)?(0):((uintn)(targetWindow->Draw.y + 63) >> 6);
        endX = (targetWindow->Draw.x + (sintn)targetWindow->Draw.width < 0)?(0):((uintn)(targetWindow->Draw.x + (sintn)targetWindow->Draw.width) >> 6);
        endY = (targetWindow->Draw.y + (sintn)targetWindow->Draw.height < 0)?(0):((uintn)(targetWindow->Draw.y + (sintn)targetWindow->Draw.height) >> 6);

        for(uintn y=startY; y<endY && y<blockYCount; y++) {
            for(uintn x=startX; x<endX && x<blockXCount; x++) {
                background_redrawFlag[x+y*blockXCount] = 0;
            }
        }
    }

    //layer_redrawFlag_Orに反映
    for(uintn i=0; i<blockXCount*blockYCount; i++) {
        if(background_redrawFlag[i]) layer_redrawFlag_Or[i] = 1;
    }
    return;
}


static void Layer_Update_SetDrawArea_Window(uint8* layer_redrawFlag_Or, uintn blockXCount, uintn blockYCount) {
    Layer_Window* targetWindow;

    uint8 window_redrawFlag[blockXCount*blockYCount];

    targetWindow = layer.Window.Data - 1;
    for(uintn i=0; i<layer.Window.count; i++) {//ウィンドウレイヤ
        targetWindow ++;

        if(targetWindow->hiddenFlag) continue;

        uintn startX, startY, endX, endY;

        //フラグのリセット
        for(uintn i=0; i<blockXCount*blockYCount; i++) {
            window_redrawFlag[i] = 0;
        }

        if(targetWindow->Draw.x == targetWindow->Draw.oldx && targetWindow->Draw.y == targetWindow->Draw.oldy) {
            //対象レイヤ移動なしなら対象レイヤの更新ブロックの再描画フラグを立てる
            startY = (targetWindow->Draw.y+(sintn)targetWindow->Change.y < 0)?(0):(((uintn)(targetWindow->Draw.y+(sintn)targetWindow->Change.y))>>6);
            endY = (targetWindow->Draw.y+(sintn)targetWindow->Change.y+(sintn)targetWindow->Change.height+63 < 0)?(0):(((uintn)(targetWindow->Draw.y+(sintn)targetWindow->Change.y+(sintn)targetWindow->Change.height+63))>>6);
            startX = (targetWindow->Draw.x+(sintn)targetWindow->Change.x < 0)?(0):(((uintn)(targetWindow->Draw.x+(sintn)targetWindow->Change.x))>>6);
            endX = (targetWindow->Draw.x+(sintn)targetWindow->Change.x+(sintn)targetWindow->Change.width+63 < 0)?(0):(((uintn)(targetWindow->Draw.x+(sintn)targetWindow->Change.x+(sintn)targetWindow->Change.width+63))>>6);
            
            for(uintn y=startY; y<endY && y<blockYCount; y++) {
                for(uintn x=startX; x<endX && x<blockXCount; x++) {
                    window_redrawFlag[x+y*blockXCount] = 1;
                }
            }
        }else {
            //対象レイヤ移動なら移動先ブロックに再描画フラグ
            startY = (targetWindow->Draw.y < 0)?(0):(((uintn)targetWindow->Draw.y)>>6);
            endY = (targetWindow->Draw.y+(sintn)targetWindow->Draw.height+63 < 0)?(0):(((uintn)(targetWindow->Draw.y+(sintn)targetWindow->Draw.height+63))>>6);
            startX = (targetWindow->Draw.x < 0)?(0):(((uintn)targetWindow->Draw.x)>>6);
            endX = (targetWindow->Draw.x+(sintn)targetWindow->Draw.width+63 < 0)?(0):(((uintn)(targetWindow->Draw.x+(sintn)targetWindow->Draw.width+63))>>6);

            for(uintn y=startY; y<endY && y<blockYCount; y++) {
                for(uintn x=startX; x<endX && x<blockXCount; x++) {
                    window_redrawFlag[x+y*blockXCount] = 1;
                }
            }
        }

        //上位レイヤ移動なら移動元ブロックに再描画フラグ
        for(uintn k=i+1; k<layer.Window.count; k++) {//ウィンドウレイヤ
            Layer_Window* targetUpperWindow = layer.Window.Data + k;
            if(!(targetUpperWindow->Draw.x == targetUpperWindow->Draw.oldx && targetUpperWindow->Draw.y == targetUpperWindow->Draw.oldy)) {

                startY = (targetUpperWindow->Draw.oldy < 0)?(0):((uintn)(targetUpperWindow->Draw.oldy)>>6);
                endY = (targetUpperWindow->Draw.oldy+(sintn)targetUpperWindow->Draw.height+63 < 0)?(0):(((uintn)(targetUpperWindow->Draw.oldy+(sintn)targetUpperWindow->Draw.height+63))>>6);
                startX = (targetUpperWindow->Draw.oldx < 0)?(0):((uintn)(targetUpperWindow->Draw.oldx)>>6);
                endX = (targetUpperWindow->Draw.oldx+(sintn)targetUpperWindow->Draw.width+63 < 0)?(0):(((uintn)(targetUpperWindow->Draw.oldx+(sintn)targetUpperWindow->Draw.width+63))>>6);

                for(uintn y=startY; y<endY && y<blockYCount; y++) {
                    for(uintn x=startX; x<endX && x<blockXCount; x++) {
                        window_redrawFlag[x+y*blockXCount] = 1;
                    }
                }
            }
        }
        {//マウスレイヤ
            if(!(layer.Mouse.Draw.x == layer.Mouse.Draw.oldx && layer.Mouse.Draw.y == layer.Mouse.Draw.oldy)) {
                startX = (layer.Mouse.Draw.oldx < 0)?(0):(((uintn)layer.Mouse.Draw.oldx) >> 6);
                endX = (layer.Mouse.Draw.oldx + (sintn)layer.Mouse.Draw.width < 0)?(0):(((uintn)(layer.Mouse.Draw.oldx + (sintn)layer.Mouse.Draw.width + 63)) >> 6);
                startY = (layer.Mouse.Draw.oldy < 0)?(0):(((uintn)layer.Mouse.Draw.oldy) >> 6);
                endY = (layer.Mouse.Draw.oldy + (sintn)layer.Mouse.Draw.height < 0)?(0):(((uintn)(layer.Mouse.Draw.oldy + (sintn)layer.Mouse.Draw.height + 63)) >> 6);

                for(uintn y=startY; y<endY && y<blockYCount; y++) {
                    for(uintn x=startX; x<endX && x<blockXCount; x++) {
                        window_redrawFlag[x+y*blockXCount] = 1;
                    }
                }
            }
        }

        //上位レイヤの非表示ブロックの再描画フラグをマスク
        for(uintn k=i+1; k<layer.Window.count; k++) {
            Layer_Window* targetUpperWindow = layer.Window.Data + k;
            
            startX = (targetUpperWindow->Draw.x + 63 < 0)?(0):((uintn)(targetUpperWindow->Draw.x + 63) >> 6);
            startY = (targetUpperWindow->Draw.y + 63 < 0)?(0):((uintn)(targetUpperWindow->Draw.y + 63) >> 6);
            endX = (targetUpperWindow->Draw.x + (sintn)targetUpperWindow->Draw.width < 0)?(0):((uintn)(targetUpperWindow->Draw.x + (sintn)targetUpperWindow->Draw.width) >> 6);
            endY = (targetUpperWindow->Draw.y + (sintn)targetUpperWindow->Draw.height < 0)?(0):((uintn)(targetUpperWindow->Draw.y + (sintn)targetUpperWindow->Draw.height) >> 6);

            for(uintn y=startY; y<endY && y<blockYCount; y++) {
                for(uintn x=startX; x<endX && x<blockXCount; x++) {
                    window_redrawFlag[x+y*blockXCount] = 0;
                }
            }
        }

        //対象レイヤのないブロックの再描画フラグをおろす
        {
            startX = (targetWindow->Draw.x < 0)?(0):((uintn)targetWindow->Draw.x >> 6);
            startY = (targetWindow->Draw.y < 0)?(0):((uintn)targetWindow->Draw.y >> 6);
            endX = (targetWindow->Draw.x + (sintn)targetWindow->Draw.width + 63 < 0)?(0):((uintn)(targetWindow->Draw.x + (sintn)targetWindow->Draw.width + 63) >> 6);
            endY = (targetWindow->Draw.y + (sintn)targetWindow->Draw.height + 63 < 0)?(0):((uintn)(targetWindow->Draw.y + (sintn)targetWindow->Draw.height + 63) >> 6);

            for(uintn y=0; y<startY && y<blockYCount; y++) {
                for(uintn x=0; x<blockXCount; x++) {
                    window_redrawFlag[x+y*blockXCount] = 0;
                }
            }
            for(uintn y=startY; y<endY && y<blockYCount; y++) {
                for(uintn x=0; x<startX && x<blockXCount; x++) {
                    window_redrawFlag[x+y*blockXCount] = 0;
                }
                for(uintn x=endX; x<blockXCount; x++) {
                    window_redrawFlag[x+y*blockXCount] = 0;
                }
            }
            for(uintn y=endY; y<blockYCount; y++) {
                for(uintn x=0; x<blockXCount; x++) {
                    window_redrawFlag[x+y*blockXCount] = 0;
                }
            }
        }

        //layer_redrawFlag_Orに反映
        for(uintn k=0; k<blockXCount*blockYCount; k++) {
            if(window_redrawFlag[k]) {
                layer_redrawFlag_Or[k] = 1;
            }
        }

        //Window.Draw及びWindow.Changeの更新
        targetWindow->Draw.oldx = targetWindow->Draw.x;
        targetWindow->Draw.oldy = targetWindow->Draw.y;
        targetWindow->Change.width = 0;
        targetWindow->Change.height = 0;
    }
    return;
}


static void Layer_Update_SetDrawArea(uint8* layer_redrawFlag_Or, uintn blockXCount, uintn blockYCount) {
    Layer_Update_SetDrawArea_Background(layer_redrawFlag_Or, blockXCount, blockYCount);

    Layer_Update_SetDrawArea_Window(layer_redrawFlag_Or, blockXCount, blockYCount);

    return;
}


static void Layer_Update_DrawWindow(uint8* layer_redrawFlag_Or, uintn blockXCount, uintn blockYCount) {
    static uint32 buff[64*64];
    Graphic_FrameBuff backBuff;
    backBuff.width = 64;
    backBuff.height = 64;
    backBuff.scanlineWidth = 64;
    backBuff.frameBuff = buff;

    sintn drawX;
    sintn drawY;
    sintn xfrom;
    sintn yfrom;
    sintn width;
    sintn height;
    for(uintn y=0; y<blockYCount; y++) {
        for(uintn x=0; x<blockXCount; x++) {
            if(!(layer_redrawFlag_Or[x+y*blockXCount])) continue;
            Graphic_FrameBuff_DrawSquare(
                    backBuff,
                    0, 0,
                    64, 64,
                    background_color);

            for(uintn i=0; i<layer.Window.count; i++) {
                if(layer.Window.Data[i].hiddenFlag) continue;
                xfrom = (x<<6) - layer.Window.Data[i].Draw.x;
                yfrom = (y<<6) - layer.Window.Data[i].Draw.y;
                drawX = 0;
                drawY = 0;
                width = 64;
                height = 64;

                if(xfrom < 0) {
                    drawX -= xfrom;
                    width += xfrom;
                    if(width < 0) continue;
                    xfrom = 0;
                 }
                if(yfrom < 0) {
                    drawY -= yfrom;
                    height += yfrom;
                    if(height < 0) continue;
                    yfrom = 0;
                }

                Graphic_FrameBuff_DrawFrom(
                    backBuff,
                    drawX, drawY,
                    xfrom, yfrom, width, height, layer.Window.Data[i].FrameBuff.Data);
            }
            Graphic_DrawFrom(x<<6, y<<6, 0, 0, 64, 64, backBuff);
        }
    }
    return;
}


//Layerを描画
void Layer_Update(void) {
    if(layer.changedFlag == 0) return;
    layer.isDrawingFlag = 1;
    layer.changedFlag = 0;

    Layer_Update_WindowState();

    //再描画フラグ計算 64x64//
    uintn blockXCount = (KernelInput->Graphic.width+63)>>6;
    uintn blockYCount = (KernelInput->Graphic.height+63)>>6;

    uint8 layer_redrawFlag_Or[blockXCount*blockYCount];

    //フラグの初期化
    for(uintn i=0; i<blockXCount*blockYCount; i++) {
        layer_redrawFlag_Or[i] = 0;
    }
    Layer_Update_SetDrawArea(layer_redrawFlag_Or, blockXCount, blockYCount);

    //有効なブロックを描画
    Layer_Update_DrawWindow(layer_redrawFlag_Or, blockXCount, blockYCount);

    //マウスカーソル描画
    Graphic_DrawMouse(layer.Mouse.Draw.x, layer.Mouse.Draw.y);
    layer.Mouse.Draw.oldx = layer.Mouse.Draw.x;
    layer.Mouse.Draw.oldy = layer.Mouse.Draw.y;

    layer.isDrawingFlag = 0;

    return;
}


//Layer.Windowのサイズ拡張
static uintn Layer_Window_Expand(void) {
    uintn newPages = layer.Window.pages*2 + 1;
    Layer_Window* newData = Memory_AllocPages(Layer_TaskId, newPages);
    if(newData == NULL) return 1;

    Functions_MemCpy(newData, layer.Window.Data, sizeof(Layer_Window)*layer.Window.count);

    Memory_FreePages(Layer_TaskId, layer.Window.pages, layer.Window.Data);
    layer.Window.pages = newPages;
    layer.Window.Data = newData;

    return 0;
}


//Layer.Windowを作成してlayerIdを返す
uintn Layer_Window_New(uint16 taskId, ascii name[], uintn x, uintn y, uintn width, uintn height) {
    if(taskId == 0 || taskId == 1) return 0;
    if(width == 0 || height == 0) return 0;
    if(width <= window_titleBar_height*3 || height <= window_titleBar_height) return 0;

    width += window_shadow_overThick + window_shadow_underThick;
    height += window_shadow_overThick + window_shadow_underThick + window_titleBar_height;

    //割り当てるlayerIdを取得
    uintn layerId = 1;
    for(sintn i=0; i<(sintn)(layer.Window.count); i++) {
        if(layer.Window.Data[i].layerId == layerId) {
            layerId++;
            i = -1;
            continue;
        }
    }

    //Layer.Windowに追加
    if((layer.Window.pages << 12) <= layer.Window.count*sizeof(Layer_Window)) {
        if(Layer_Window_Expand()) return 0;
    }

    Layer_Window* newWindow = layer.Window.Data + layer.Window.count;
    newWindow->taskId = taskId;
    newWindow->hiddenFlag = 0;
    newWindow->layerId = layerId;
    for(uintn i=0; i<15; i++) {
        newWindow->name[i] = name[i];
        if(name[i] == '\0') break;
    }
    newWindow->name[15] = '\0';

    newWindow->Draw.x = x;
    newWindow->Draw.y = y;
    newWindow->Draw.width = width;
    newWindow->Draw.height = height;
    newWindow->Draw.oldx = x;
    newWindow->Draw.oldy = y;

    newWindow->Change.x = 0;
    newWindow->Change.y = 0;
    newWindow->Change.width = width;
    newWindow->Change.height = height;

    newWindow->FrameBuff.pages = (width*height*sizeof(uint32)+0xfff)>>12;
    newWindow->FrameBuff.Data.width = width;
    newWindow->FrameBuff.Data.height = height;
    newWindow->FrameBuff.Data.scanlineWidth = newWindow->FrameBuff.Data.width;
    newWindow->FrameBuff.Data.frameBuff = Memory_AllocPages(Layer_TaskId, newWindow->FrameBuff.pages);
    if(newWindow->FrameBuff.Data.frameBuff == NULL) return 0;

    layer.Window.count++;

    layer.changedFlag = 1;

    //フレームバッファを白く塗りつぶす
    Graphic_Color defaultColor = {0xff, 0xff, 0xff};
    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        0, 0,
        width, height,
        defaultColor);

    //タイトルバー描画
    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        window_shadow_overThick, window_shadow_overThick,
        width - window_shadow_overThick - window_shadow_underThick, window_titleBar_height,
        window_titleBar_backgroundColor);

    for(uintn i=0; i<15; i++) {
        if(name[i] == '\0') break;
        Font_Draw(newWindow->FrameBuff.Data, window_shadow_overThick + window_titleBar_height +8 + 8*i, window_shadow_overThick + ((window_titleBar_height>>1)-8) + 2, name[i], window_titleBar_titleText_color);
    }

    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        width - window_shadow_underThick - 1 - (window_titleBar_height - 2), window_shadow_overThick + 1,
        window_titleBar_height - 2, window_titleBar_height - 2,
        window_titleBar_closeButton_color);

    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        window_shadow_overThick + 1, window_shadow_overThick + 1,
        window_titleBar_height - 2, window_titleBar_height - 2,
        window_titleBar_hiddenButton_color);

    for(uintn i=10; i<window_titleBar_height-12; i++) {
        ((uint32*)newWindow->FrameBuff.Data.frameBuff)[(width-window_shadow_underThick-1-(window_titleBar_height-2)+i)+(window_shadow_overThick+1+i)*width] = 0x00505050;
        ((uint32*)newWindow->FrameBuff.Data.frameBuff)[(width-window_shadow_underThick-2-i)+(window_shadow_overThick+1+i)*width] = 0x00505050;
    }
    for(uintn i=10; i<window_titleBar_height-12; i++) {
        ((uint32*)newWindow->FrameBuff.Data.frameBuff)[(window_shadow_overThick+1+i)+(((window_titleBar_height-2)>>1)+window_shadow_overThick+1)*width] = 0x00505050;
    }

    //影描画
    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        width - window_shadow_underThick, 0,
        window_shadow_underThick, height,
        window_shadow_color);
    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        0, 0,
        width, window_shadow_overThick,
        window_shadow_color);
    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        0, height - window_shadow_underThick,
        width, window_shadow_underThick,
        window_shadow_color);
    Graphic_FrameBuff_DrawSquare(
        newWindow->FrameBuff.Data,
        0, 0,
        window_shadow_overThick, height,
        window_shadow_color);

    return layerId;
}


//ウインドウ削除
uintn Layer_Window_Delete(uintn layerId) {
    sintn layerIndex = Layer_Window_GetIndex(layerId);
    if(layerIndex < 0) return 1;

    Layer_Window* targetWindow = layer.Window.Data + layerIndex;

    Memory_FreePages(
        Layer_TaskId,
        (targetWindow->FrameBuff.Data.scanlineWidth*targetWindow->FrameBuff.Data.height + 0xfff)>>12,
        targetWindow->FrameBuff.Data.frameBuff);

    Functions_MemCpy(layer.Window.Data+layerIndex, layer.Window.Data+layerIndex+1, sizeof(Layer_Window)*(layer.Window.count-layerIndex-1));

    layer.Window.count--;

    for(uintn i=0; i<layer.Window.count; i++) {
        Layer_Window_FlushIndex(i);
    }

    layer.drawBackgroundFlag = 1;

    return 0;
}


//指定タスクのウインドウ全削除
void Layer_Window_DeleteAll(uint16 taskId) {
    Layer_Window* targetWindow = layer.Window.Data - 1;
    for(uintn i=0; i<layer.Window.count; i++) {
        targetWindow ++;
        if(targetWindow->taskId == taskId) {
            Layer_Window_Delete(targetWindow->layerId);
        }
    }
    return;
}


//LayerIdのウインドウのタスクIDを返す
uint16 Layer_Window_GettaskId(uintn layerId) {
    sintn layerIndex = Layer_Window_GetIndex(layerId);
    if(layerIndex < 0) return 0;

    return layer.Window.Data[layerIndex].taskId;
}


//ウインドウを最前面に移動
void Layer_Window_Focus(uintn layerId) {
    sintn layerIndex = Layer_Window_GetIndex(layerId);
    if(layerIndex < 0) return;

    Layer_Window temp_layerwindow;
    Functions_MemCpy(&temp_layerwindow, layer.Window.Data+layerIndex, sizeof(Layer_Window));

    Functions_MemCpy(layer.Window.Data+layerIndex, layer.Window.Data+layerIndex+1, sizeof(Layer_Window)*(layer.Window.count - layerIndex - 1));

    Functions_MemCpy(layer.Window.Data+layer.Window.count-1, &temp_layerwindow, sizeof(Layer_Window));

    Layer_Window_FlushIndex(layer.Window.count-1);

    return;
}


//layerIdのウインドウを再描画
void Layer_Window_Flush(uintn layerId) {
    sintn layerIndex = Layer_Window_GetIndex(layerId);
    if(layerIndex < 0) return;

    Layer_Window* targetWindow = layer.Window.Data + layerIndex;
    targetWindow->Change.x = 0;
    targetWindow->Change.y = 0;
    targetWindow->Change.width = targetWindow->Draw.width;
    targetWindow->Change.height = targetWindow->Draw.height;

    return;
}


//layerIndexのウインドウを再描画
void Layer_Window_FlushIndex(uintn layerIndex) {
    if(layer.Window.count <= layerIndex) return;

    Layer_Window* targetWindow = layer.Window.Data + layerIndex;
    targetWindow->Change.x = 0;
    targetWindow->Change.y = 0;
    targetWindow->Change.width = targetWindow->Draw.width;
    targetWindow->Change.height = targetWindow->Draw.height;

    return;
}


//レイヤーのフレームバッファを取得する
uintn Layer_Window_GetFrameBuff(uintn layerId, Graphic_FrameBuff* framebuff) {
    if(framebuff == NULL) return 1;
    sintn layerIndex = Layer_Window_GetIndex(layerId);
    if(layerIndex < 0) return 2;

    Graphic_FrameBuff* targetWindow_FrameBuff = &(layer.Window.Data[layerIndex].FrameBuff.Data);
    framebuff->width = targetWindow_FrameBuff->width - window_shadow_overThick - window_shadow_underThick;
    framebuff->scanlineWidth = targetWindow_FrameBuff->width;
    framebuff->height = targetWindow_FrameBuff->height - window_shadow_overThick - window_shadow_underThick - window_titleBar_height;
    framebuff->frameBuff = targetWindow_FrameBuff->frameBuff + targetWindow_FrameBuff->scanlineWidth*(window_shadow_overThick+window_titleBar_height) + window_shadow_overThick;

    return 0;
}


//最上位のレイヤのTaskIdを取得
uint16 Layer_Window_GetFocusedTaskId(void) {
    if(layer.Window.count == 0) return 0;

    return layer.Window.Data[layer.Window.count-1].taskId;
}


//レイヤーのフレームバッファ更新を通知
void Layer_Window_NotifyUpdate(uintn layerId, uintn x, uintn y, uintn width, uintn height) {
    sintn layerIndex = Layer_Window_GetIndex(layerId);
    if(layerIndex < 0) return;

    Layer_Window* targetWindow = layer.Window.Data + layerIndex;

    x += window_shadow_overThick;
    y += window_shadow_overThick + window_titleBar_height;

    if(targetWindow->Change.width == 0 && targetWindow->Change.height == 0) {
        targetWindow->Change.x = x;
        targetWindow->Change.y = y;
        targetWindow->Change.width = width;
        targetWindow->Change.height = height;
    }else {
        const uintn startX = (targetWindow->Change.x < x)?(targetWindow->Change.x):(x);
        const uintn endX = (targetWindow->Change.x+targetWindow->Change.width < x+width)?(x+width):(targetWindow->Change.x+targetWindow->Change.width);
        const uintn startY = (targetWindow->Change.y < y)?(targetWindow->Change.y):(y);
        const uintn endY = (targetWindow->Change.y+targetWindow->Change.height < y+height)?(y+height):(targetWindow->Change.y+targetWindow->Change.height);
        targetWindow->Change.x = startX;
        targetWindow->Change.y = startY;
        targetWindow->Change.width = endX - startX;
        targetWindow->Change.height = endY - startY;
    }
    
    layer.changedFlag = 1;

    return;
}


//マウス更新をLayerモジュールに通知
void Layer_Mouse_NotifyUpdate(uintn x, uintn y, uintn leftButton) {
    if(layer.isDrawingFlag) return;
    if(leftButton) {
        layer.Mouse.Drag.x = (sintn)x - (sintn)layer.Mouse.Draw.oldx;
        layer.Mouse.Drag.y = (sintn)y - (sintn)layer.Mouse.Draw.oldy;
        layer.Mouse.leftButton = 1;
    }

    layer.Mouse.Draw.x = x;
    layer.Mouse.Draw.y = y;
    layer.changedFlag = 1;
    
    return;
}

