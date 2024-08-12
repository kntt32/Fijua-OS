#include <types.h>
#include <kernel.h>
#include "graphic.h"
#include "font.h"

static uint8 fontdata_monospace[] = {
#include "font_bitmap"
};


//draw font
void Font_Draw(Graphic_FrameBuff drawto, uintn x, uintn y, ascii asciicode, Graphic_Color color) {
    if(drawto.frameBuff == NULL || drawto.width < 8) return;

    uint32* fromFrameBuffTarg = (uint32*)drawto.frameBuff + x + y*drawto.scanlineWidth;
    uint32 bgrcolor = Graphic_Color2BGR(color);
    uint8 targfontline;

    uintn drawWidth = 8;
    uintn drawHeight = 16;
    if(drawto.width <= x) return;
    if(drawto.height <= y) return;
    if(drawto.width <= x+drawWidth) drawWidth = drawto.width-x;
    if(drawto.height <= y+drawHeight) drawHeight = drawto.height-y;

    for(uintn i=0; i<drawHeight; i++) {
        targfontline = fontdata_monospace[(asciicode<<4) + i];
        for(uintn k=0; k<drawWidth; k++) {
            if(targfontline & 0x80) *fromFrameBuffTarg = bgrcolor;
            fromFrameBuffTarg++;
            targfontline <<= 1;
        }
        fromFrameBuffTarg += drawto.scanlineWidth - 8;
    }

    return;
}


//draw string
void Font_DrawStr(Graphic_FrameBuff drawto, uintn x, uintn y, ascii str[], Graphic_Color color, uintn length) {
    if(str == NULL || length == 0) return;

    uintn nowx = x;
    uintn nowy = y;

    for(uintn i=0; i<length; i++) {
        if(str[i] == '\0') return;
        if(str[i] == '\r') {
            nowx = x;
            continue;
        }
        if(str[i] == '\n') {
            nowx = x;
            nowy += 16;
            continue;
        }
        Font_Draw(drawto, nowx, nowy, str[i], color);

        nowx += 8;
    }

    return;
}
