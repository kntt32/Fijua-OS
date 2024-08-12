#ifndef INCLUDED_LAYER_H
#define INCLUDED_LAYER_H

typedef struct {
    uint16 taskId;

    uint8 hiddenFlag;

    uintn layerId;//!=0

    ascii name[16];

    struct {
        sintn x;
        sintn y;
        uintn width;
        uintn height;

        sintn oldx;
        sintn oldy;
    } Draw;

    struct {
        uintn x;
        uintn y;
        uintn width;
        uintn height;
    } Change;

    struct {
        uintn pages;
        Graphic_FrameBuff Data;
    } FrameBuff;
} Layer_Window;


typedef struct {
    struct {
        sintn x;
        sintn y;
        uintn width;
        uintn height;

        sintn oldx;
        sintn oldy;
    } Draw;

    struct {
        sintn x;
        sintn y;
    } Drag;
    uintn leftButton;
    uintn oldLeftButton;
} Layer_Mouse;


typedef struct {
    uintn changedFlag;
    uintn isDrawingFlag;
    uintn drawBackgroundFlag;

    struct {
        uintn count;
        uintn pages;
        Layer_Window* Data;
    } Window;

    Layer_Mouse Mouse;
} Layer;


void Layer_Init(void);

void Layer_Update(void);

void Layer_Mouse_NotifyUpdate(uintn x, uintn y, uintn leftButton);

uintn Layer_Window_New(uint16 taskId, ascii name[], uintn x, uintn y, uintn width, uintn height);

uintn Layer_Window_Delete(uintn layerId);

void Layer_Window_DeleteAll(uint16 taskId);

uint16 Layer_Window_GettaskId(uintn layerId);

void Layer_Window_Focus(uintn layerId);

void Layer_Window_Flush(uintn layerId);

void Layer_Window_FlushIndex(uintn layerIndex);

uintn Layer_Window_GetFrameBuff(uintn layerId, Graphic_FrameBuff* framebuff);

uint16 Layer_Window_GetFocusedTaskId(void);

void Layer_Window_NotifyUpdate(uintn layerId, uintn x, uintn y, uintn width, uintn height);

#endif
