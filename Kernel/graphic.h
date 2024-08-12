#ifndef INCLUDED_GRAPHIC_H

#define mouseCursor_width (16)
#define mouseCursor_height (18)

#define Graphic_Color2BGR(color) ((((uint32)color.red) << 16) + (((uint32)color.green) << 8) + ((uint32)color.blue))

void Graphic_Init(void);

typedef struct {
    uint8 red;
    uint8 green;
    uint8 blue;
} Graphic_Color;

typedef struct {
    uint32* frameBuff;
    uintn width;
    uintn height;
    uintn scanlineWidth;
} Graphic_FrameBuff;

#ifndef FILE_GRAPHIC_C

extern void (*Graphic_DrawSquare)(sintn x, sintn y, uintn width, uintn height, Graphic_Color color);//draw square
extern void (*Graphic_DrawFrom)(sintn x, sintn y, uintn xfrom, uintn yfrom, uintn width, uintn height, Graphic_FrameBuff from);//draw memmapped bgr frame

#endif

void Graphic_DrawMouse(uintn x, uintn y);//draw mouseCursor

void Graphic_FrameBuff_DrawSquare(Graphic_FrameBuff framebuff, sintn x, sintn y, uintn width, uintn height, Graphic_Color color);

void Graphic_FrameBuff_DrawFrom(Graphic_FrameBuff framebuff, sintn x, sintn y, uintn xfrom, uintn yfrom, uintn width, uintn height, Graphic_FrameBuff from);

#endif
