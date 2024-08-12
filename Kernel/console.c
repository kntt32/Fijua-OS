#include <types.h>
#include <kernel.h>
#include <efi.h>
#include "graphic.h"
#include "console.h"
#include "font.h"
#include "layer.h"

#define buffwidth (80)
#define buffheight (30)

static ascii buff[buffwidth * buffheight];//80x30
static uintn cursorX = 0;
static uintn cursorY = 0;
static uintn lineChangedFlag[buffheight];

static uint32 backgroundColor = 0x002d3881;
static Graphic_Color fontColor;

static uint32* console_framebuff[buffwidth*buffheight*16*8];
static Graphic_FrameBuff console_framebuffData;

static void Console_FlushLine(uintn line);
static void Console_Scroll(void);


void Console_Init(void) {
    console_framebuffData.frameBuff = console_framebuff;
    console_framebuffData.width = buffwidth*8;
    console_framebuffData.height = buffheight*16;
    console_framebuffData.scanlineWidth = console_framebuffData.width;

    for(uintn i=0; i<buffwidth*buffheight; i++) {
        buff[i] = ' ';
    }
    for(uintn i=0; i<buffwidth*buffheight*16*8>>1; i++) {
        *(((uint32*)console_framebuff)+i) = backgroundColor;
    }
    for(uintn i=0; i<buffheight; i++) {
        lineChangedFlag[i] = 0;
    }

    fontColor.red = 0xff;
    fontColor.green = 0xff;
    fontColor.blue = 0xff;

    cursorX = 0;
    cursorY = 0;

    for(uintn i=0; i<buffheight; i++) {
        Console_FlushLine(i);
    }
    Graphic_DrawFrom(0, 0, 0, 0, buffwidth*8, buffheight*16, console_framebuffData);


    return;
}


void Console_Print(ascii str[]) {
    lineChangedFlag[cursorY] = 1;
    for(uintn i=0; 1; i++) {
        switch(str[i]) {
            case '\0':
                for(uintn i=0; i<buffheight; i++) {
                    if(lineChangedFlag[i]) Console_FlushLine(i);
                }
                for(uintn i=0; i<buffheight; i++) {
                    if(lineChangedFlag[i]) {
                        lineChangedFlag[i] = 0;
                        Graphic_DrawFrom(0, i*16, 0, i*16, buffwidth*8, 16, console_framebuffData);
                    }
                }
                return;
            case '\n':
                cursorY++;
                if(buffheight <= cursorY) Console_Scroll();
                cursorX = 0;
                break;
            case '\r':
                cursorX = 0;
                break;
            default:
                if(cursorX == buffwidth) {
                    cursorX = 0;
                    cursorY++;
                }
                if(buffheight <= cursorY) Console_Scroll();
                lineChangedFlag[cursorY] = 1;
                buff[cursorX + cursorY*buffwidth] = str[i];
                cursorX++;

                break;
        }
    }

    return;
}


static void Console_Scroll(void) {
    for(uintn i=0; i<buffheight-1; i++) {
        for(uintn k=0; k<buffwidth; k++) {
            buff[i*buffwidth+k] = buff[(i+1)*buffwidth+k];
        }
    }

    for(uintn i=0; i<buffwidth; i++) {
        buff[(buffheight-1)*buffwidth+i] = ' ';
    }

    for(uintn i=0; i<buffheight; i++) {
        lineChangedFlag[i] = 1;
    }

    cursorY--;
    return;
}


static void Console_FlushLine(uintn line) {
    if(buffheight <= line) return;

    for(uintn i=0; i<16; i++) {
        for(uintn k=0; k<4*buffwidth; k++) {
            ((uint64*)console_framebuff)[k+(line*16 + i)*4*buffwidth] = (((uint64)backgroundColor)<<32) + ((uint64)backgroundColor);
        }
    }

    for(uintn i=0; i<buffwidth; i++) {
        Font_Draw(console_framebuffData, i*8, line*16, buff[buffwidth*line+i], fontColor);
    }

    return ;
}



