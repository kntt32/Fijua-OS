#include <types.h>
#include <efi.h>
#include <kernel.h>
#include "functions.h"
#include "x64.h"
#include "queue.h"
#include "task.h"
#include "console.h"

extern KernelInputStruct* KernelInput;

//HaltLoop
void HltLoop(void) {
    while(1) {
        Task_Yield();
        Hlt();
    }
}


//数字を16進数に変換する
void SPrintIntX(uintn number, uintn buffsize, ascii buff[]) {
    uintn n;
    for(sintn i=buffsize-2; 0<=i; i--) {
        n = number & 0xf;
        buff[i] = (n<10)?(n+'0'):(n+'a'-10);
        
        number >>= 4;
    }
    buff[buffsize-1] = '\0';
    
    return;
}


//log2
sintn Log2(uintn number) {
    for(sintn i=0; i<64; i++) {
        if(number>>i == 0) return i-1;
    }
    return -1;
}


//fromからtoへsizeバイトだけメモリコピー
void Functions_MemCpy(void* to, const void* from, uintn size) {
    uint64* targTo64 = (uint64*)to;
    uint64* targFrom64 = (uint64*)from;
    for(uintn i=0; i<(size >> 3); i++) {
        *targTo64 = *targFrom64;
        targTo64++;
        targFrom64++;
    }
    uint8* targTo8 = (uint8*)targTo64;
    uint8* targFrom8 = (uint8*)targFrom64;
    for(uintn i=0; i<(size&0x07); i++) {
        *targTo8 = *targFrom8;
        targTo8++;
        targFrom8++;
    }
    return;
}


//UTF-16の文字コードをasciiに変換して*outputに戻す x64のみ対応
uintn Functions_UTF16LE2ASCII(uint16 input, ascii* output) {
    uint8* in_uint8 = (uint8*)&input;
    if(in_uint8[1] != 0 || 0x80 <= in_uint8[0]) return 1;

    *output = in_uint8[0];
    if(*output == '\r') *output = '\n';

    return 0;
}


//countまでの範囲でNULL文字が出るまでutf16leのinput[]の文字列をasciiのoutput[]の文字列に変換
void Functions_UTF16LE2ASCII_Str(uintn count, const uint16 input[], ascii output[]) {
    if(count == 0 || input == NULL || output == NULL) return;

    for(uintn i=0; i<count; i++) {
        Functions_UTF16LE2ASCII(input[i], output+i);
        if(input[i] == 0) break;
    }

    return;
}


//asciiをUTF-16に変換　x64のみ
void Functions_ASCII2UTF16LE(ascii input, uint16* output) {
    uint8* out_uint8 = (uint8*)output;

    output[0] = input;
    output[1] = 0;

    return;
}


//countまでの範囲でNULL文字が出るまでasciiのinput[]の文字列をutf16leのoutput[]の文字列に変換
void Functions_ASCII2UTF16LE_Str(uintn count, const ascii input[], uint16 output[]) {
    if(count == 0 || input == NULL || output == NULL) return;

    for(uintn i=0; i<count; i++) {
        Functions_ASCII2UTF16LE(input[i], output+i);
        if(input[i] == '\0') break;
    }

    return;
}


//メモリダンプ
void Functions_MemDump(void* start, uintn size) {
    ascii strbuff[3];
    uint8* memptr = (uint8*)start;
    for(uintn i=0; i<(size+15)/16; i++) {
        for(uintn k=0; k<16; k++) {
            SPrintIntX(memptr[k+i*16], 3, strbuff);
            Console_Print(strbuff);
            Console_Print(" ");
        }
        Console_Print("\n");
    }
}


//文字列比較
sintn Functions_StrCmp(const ascii str1[], const ascii str2[]) {
    if(str1 == NULL || str2 == NULL) return 1;

    for(uintn i=0; 1; i++) {
        if(str1[i] != str2[i]) return ((sintn)str1[i] - (sintn)str2[i]);
        if(str1[i] == '\0') return 0;
    }

    return 0;
}


//文字数カウント NULL文字は含まない
uintn Functions_CountStr(const ascii str[]) {
    if(str == NULL) return 0;

    for(uintn i=0; 1; i++) {
        if(str[i] == '\0') return i;
    }
}
