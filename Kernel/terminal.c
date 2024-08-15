#include <types.h>
#include "graphic.h"
#include "queue.h"
#include "task.h"
#include "terminal.h"
#include "file.h"
#include <app_syscall_x64.h>

#include "functions.h"

#define Terminal_StrWidth (50)
#define Terminal_StrHeight (20)


typedef struct {
    uintn layerId;

    uintn cursorX;
    uintn cursorY;

    ascii strBuff[Terminal_StrWidth*Terminal_StrHeight];
    uint8 updateFlag[Terminal_StrHeight];

    uint8 waitingKeyFlag;
    ascii keyStrBuff[1000];
    uintn keyStrBuffIndex;
    uintn keyStrBuffStartCursorX;
    uintn keyStrBuffStartCursorY;
} Terminal;


void Terminal_Init(Terminal* this);
void Terminal_Flush(Terminal* this);
void Terminal_GetKeyInput(Terminal* this);
void Terminal_Print(Terminal* this, ascii str[]);
void Terminal_Cls(Terminal* this);

static const Graphic_Color Terminal_BackgroundColor = {0x1b, 0x1d, 0x29};
static const Graphic_Color Terminal_FontColor = {0xd3, 0xd4, 0xde};


sintn Terminal_Main(void) {
    Terminal terminal;

    Terminal_Init(&terminal);
    if(App_Syscall_NewWindow(&(terminal.layerId), 500, 100, Terminal_StrWidth*8, Terminal_StrHeight*16, "Terminal")) return -1;
    App_Syscall_DrawSquare(terminal.layerId, 0, 0, Terminal_StrWidth*8, Terminal_StrHeight*16, Terminal_BackgroundColor);

    Task_Message message;

    while(1) {
        App_Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_KeyPushed:
                if(terminal.waitingKeyFlag) {
                    if(message.data.KeyPushed.asciiCode != 0) {
                        switch(message.data.KeyPushed.asciiCode) {
                            case '\n':
                                Terminal_Print(&terminal, "\n");
                                App_Syscall_StdOut(terminal.keyStrBuff, sizeof(terminal.keyStrBuff));
                                terminal.waitingKeyFlag = 0;
                                break;
                            case 0x08:
                                if(0 < terminal.keyStrBuffIndex) {
                                    terminal.keyStrBuffIndex--;
                                    terminal.keyStrBuff[terminal.keyStrBuffIndex] = '\0';
                                    terminal.cursorX = terminal.keyStrBuffStartCursorX;
                                    terminal.cursorY = terminal.keyStrBuffStartCursorY;
                                    Terminal_Print(&terminal, terminal.keyStrBuff);
                                }
                                break;
                            default:
                                if(!(sizeof(terminal.keyStrBuff) <= (terminal.keyStrBuffIndex+1)*sizeof(ascii))) {
                                    terminal.keyStrBuff[terminal.keyStrBuffIndex] = message.data.KeyPushed.asciiCode;
                                    terminal.keyStrBuff[terminal.keyStrBuffIndex+1] = '\0';
                                    terminal.keyStrBuffIndex++;
                                    terminal.cursorX = terminal.keyStrBuffStartCursorX;
                                    terminal.cursorY = terminal.keyStrBuffStartCursorY;
                                    Terminal_Print(&terminal, terminal.keyStrBuff);
                                    break;
                                }
                        }
                    }else {
                        switch(message.data.KeyPushed.scanCode) {
                            default:
                                break;
                        }
                    }
                }else {
                    App_Syscall_StdOut("", sizeof(""));
                }
                break;
            case Task_Message_IPCMessage:
                {
                    if(message.data.IPCMessage.u64 == 0 || message.data.IPCMessage.u64 == 2) {
                        ascii strBuff[33];
                        for(uintn i=0; i<32; i++) strBuff[i] = message.data.IPCMessage.str[i];
                        strBuff[32] = '\0';
                        Terminal_Print(&terminal, strBuff);
                    }else if(message.data.IPCMessage.u64 == 1) {
                        Terminal_GetKeyInput(&terminal);
                    }else if(message.data.IPCMessage.u64 == 3) {
                        Terminal_Cls(&terminal);
                    }
                }
                break;
            case Task_Message_CloseWindow:
                App_Syscall_Exit(0);
            case Task_Message_Nothing:
                Terminal_Print(&terminal, "Message Received: Nothing\n");
                break;
            case Task_Message_Quit:
                App_Syscall_Exit(0);
            default:
                {
                Terminal_Print(&terminal, "Message Received: UnKnown\n");
                }
                break;
        }
    }

    return 0;
}


//ターミナルの初期化
void Terminal_Init(Terminal* this) {
    if(this == NULL) return;

    this->layerId = 0;

    this->cursorX = 0;
    this->cursorY = 0;
    for(uintn i=0; i<Terminal_StrWidth*Terminal_StrHeight; i++) {
        this->strBuff[i] = ' ';
    }
    for(uintn i=0; i<Terminal_StrHeight; i++) this->updateFlag[i] = 1;

    this->waitingKeyFlag = 0;
    this->keyStrBuffIndex = 0;
    this->keyStrBuffStartCursorX = 0;
    this->keyStrBuffStartCursorY = 0;

    return;
}


//ターミナルのスクロール
void Terminal_Scroll(Terminal* this) {
    if(this == NULL) return;

    for(uintn i=0; i<Terminal_StrHeight; i++) {
        this->updateFlag[i] = 1;
    }
    for(uintn i=1; i<Terminal_StrHeight; i++) {
        for(uintn k=0; k<Terminal_StrWidth; k++) {
            this->strBuff[k+(i-1)*Terminal_StrWidth] = this->strBuff[k+i*Terminal_StrWidth];
        }
    }
    for(uintn i=0; i<Terminal_StrWidth; i++) {
        this->strBuff[i+(Terminal_StrHeight-1)*Terminal_StrWidth] = ' ';
    }
    this->cursorX = 0;
    this->cursorY--;

    if(this->waitingKeyFlag) {
        if(this->keyStrBuffStartCursorY == 0) {
            if(this->keyStrBuffIndex < Terminal_StrWidth) {
                this->keyStrBuff[0] = '\0';
                this->keyStrBuffIndex = 0;
            }else {
                for(uintn i=0; i<this->keyStrBuffIndex - Terminal_StrWidth; i++) {
                    this->keyStrBuff[i] = this->keyStrBuff[i + Terminal_StrWidth];
                }
                this->keyStrBuff[this->keyStrBuffIndex - Terminal_StrWidth] = '\0';
                this->keyStrBuffIndex -= Terminal_StrWidth;
            }
        }
        this->keyStrBuffStartCursorY--;
    }
    
    return;
}


//ターミナルの描画
void Terminal_Flush(Terminal* this) {
    if(this == NULL) return;

    for(uintn i=0; i<Terminal_StrHeight; i++) {
        if(this->updateFlag[i]) {
            this->updateFlag[i] = 0;
            for(uintn k=0; k<Terminal_StrWidth; k++) {
                App_Syscall_DrawSquare(this->layerId, k*8, i*16, 8, 16, Terminal_BackgroundColor);
                App_Syscall_DrawFont(this->layerId, k*8, i*16, this->strBuff[k+i*Terminal_StrWidth], Terminal_FontColor);
            }
        }
    }
    App_Syscall_DrawSquare(this->layerId, this->cursorX*8, this->cursorY*16+14, 8, 2, Terminal_FontColor);
    
    return;
}


//キー入力モードへ
void Terminal_GetKeyInput(Terminal* this) {
    this->waitingKeyFlag = 1;
    this->keyStrBuff[0] = '\0';
    this->keyStrBuffIndex = 0;
    this->keyStrBuffStartCursorX = this->cursorX;
    this->keyStrBuffStartCursorY = this->cursorY;
    return;
}


//strの文字表示
void Terminal_Print(Terminal* this, ascii str[]) {
    if(this == NULL || str == NULL) return;

    sintn index = -1;
    while(1) {
        index ++;
        if(str[index] == '\0') {
            for(uintn i=this->cursorX; i<Terminal_StrWidth; i++) this->strBuff[i + this->cursorY*Terminal_StrWidth] = ' ';
            this->updateFlag[this->cursorY] = 1;
            Terminal_Flush(this);
            return;
        }
        if(str[index] == '\n') {
            this->updateFlag[this->cursorY] = 1;
            this->cursorX = 0;
            this->cursorY ++;
            if(Terminal_StrHeight <= this->cursorY) Terminal_Scroll(this);
            continue;
        }

        this->strBuff[this->cursorX + this->cursorY*Terminal_StrWidth] = str[index];
        this->updateFlag[this->cursorY] = 1;
        this->cursorX ++;
        if(Terminal_StrWidth <= this->cursorX) {
            this->cursorX = 0;
            this->cursorY ++;
            if(Terminal_StrHeight <= this->cursorY) Terminal_Scroll(this);
        }
    }

    return;
}


//画面クリア
void Terminal_Cls(Terminal* this) {
    if(this == NULL) return;

    this->cursorX = 0;
    this->cursorY = 0;
    for(uintn i=0; i<Terminal_StrWidth*Terminal_StrHeight; i++) this->strBuff[i] = ' ';
    for(uintn i=0; i<Terminal_StrHeight; i++) this->updateFlag[i] = 1;

    if(this->waitingKeyFlag) {
        this->keyStrBuffStartCursorX = 0;
        this->keyStrBuffStartCursorY = 0;
        Terminal_Print(this, this->keyStrBuff);
    }

    Terminal_Flush(this);

    return;
}
