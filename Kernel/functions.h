#ifndef INCLUDED_FUNCTIONS_H
#define INCLUDED_FUNCTIONS_H

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (b) : (a))

void HltLoop(void);

void SPrintIntX(uintn number, uintn buffsize, ascii buff[]);

sintn Log2(uintn number);

void Functions_MemCpy(void* to, const void* from, uintn size);

uintn Functions_UTF16LE2ASCII(uint16 input, ascii* output);

void Functions_ASCII2UTF16LE(ascii input, uint16* output);

void Functions_MemDump(void* start, uintn size);

sintn Functions_StrCmp(const ascii str1[], const ascii str2[]);

uintn Functions_CountStr(const ascii str[]);

uintn Functions_StartShell(void);

#endif
