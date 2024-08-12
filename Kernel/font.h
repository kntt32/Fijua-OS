#ifndef INCLUDED_FONT_H
#define INCLUDED_FONT_H

void Font_Draw(Graphic_FrameBuff drawto, uintn x, uintn y, ascii asciicode, Graphic_Color color);

void Font_DrawStr(Graphic_FrameBuff drawto, uintn x, uintn y, ascii str[], Graphic_Color color, uintn length);

#endif
