#include <types.h>
#include <kernel.h>
#include <efi.h>
#include <efi_runtime_services.h>
#include <efi_simple_text_input_protocol.h>
#include "syscall.h"
#include "queue.h"
#include "task.h"
#include "x64.h"
#include "console.h"
#include "graphic.h"
#include "layer.h"
#include "font.h"
#include "message.h"
#include "functions.h"
#include "file.h"
#include "memory.h"
#include "clipboard.h"

#include "terminal.h"

#include <elfloader.h>

#define Syscall_SyscallAddr ((void**)0x100000)

extern KernelInputStruct* KernelInput;

static Graphic_Color white = {0xff, 0xff, 0xff};
static Graphic_Color black = {0x00, 0x00, 0x00};
static Graphic_Color gray = {0x88, 0x88, 0x88};
static Graphic_Color blue = {0x55, 0x55, 0xff};
static Graphic_Color shudow = {0x4f, 0x4f, 0x4f};
static Graphic_Color light = {0xd0, 0xd0, 0xd0};
static Graphic_Color ui_color = {0xf0, 0xf0, 0xf0};

sintn Syscall_AppEnter();

void Syscall_Init(void) {
    Console_Print("Syscall_Init: Initializing Syscall...\n");

    *Syscall_SyscallAddr = (void*)Syscall_AppEnter;

    return;
}


//新規ウインドウ作成してLayerIdを返す
sintn Syscall_NewWindow(out uintn* layerId, in sintn x, in sintn y, in uintn width, in uintn height, in ascii title[]) {
    if(layerId == NULL) return 1;

    uint16 taskId = Task_GetRunningTaskId();

    *layerId = Layer_Window_New(taskId, title, x, y, width, height);
    if(*layerId == 0) return -1;

    return 0;
}


//layerIdのウインドウを閉じる
sintn Syscall_CloseWindow(in uintn layerId) {
    uint16 runningTaskId = Task_GetRunningTaskId();
    uint16 layerTaskID = Layer_Window_GettaskId(layerId);
    if(layerTaskID < 0 || runningTaskId != layerTaskID) return 1;
    
    Layer_Window_Delete(layerId);
    
    return 0;
}


//CPUを解放
sintn Syscall_YieldCpu(void) {
    Task_Yield();

    return 0;
}


//ウインドウに四角形を描画
sintn Syscall_DrawSquare(in uintn layerId, in uintn x, in uintn y, in uintn width, in uintn height, in Graphic_Color color) {
    Graphic_FrameBuff framebuff;
    if(Layer_Window_GetFrameBuff(layerId, &framebuff)) return -1;

    Graphic_FrameBuff_DrawSquare(framebuff, x, y, width, height, color);

    Layer_Window_NotifyUpdate(layerId, x, y, width, height);
    
    return 0;
}


//ウインドウに文字描画
sintn Syscall_DrawFont(in uintn layerId, in uintn x, in uintn y, in ascii asciicode, in Graphic_Color color) {
    Graphic_FrameBuff framebuff;
    if(Layer_Window_GetFrameBuff(layerId, &framebuff)) return -1;

    Font_Draw(framebuff, x, y, asciicode, color);

    Layer_Window_NotifyUpdate(layerId, x, y, 8, 16);
    
    return 0;
}


//メッセージを取得する　なければ処理停止
sintn Syscall_ReadMessage(out Task_Message* message) {
    Task_Yield();

    if(message == NULL) return 1;
    
    while(1) {
        if(Task_Messages_DeQueue(Task_GetRunningTaskId(), message)) {
            message->type = Task_Message_Nothing;
        }
        if(message->type != Task_Message_Nothing) break;
        Task_Halt();
    }

    return 0;
}


//メッセージを取得する　なければTask_Message_Nothingを返す
sintn Syscall_CheckMessage(out Task_Message* message) {
    Task_Yield();

    if(message == NULL) return 1;

    Task_Messages_Check(Task_GetRunningTaskId(), message);

    return 0;
}


//タスク間通信 32バイト送る
sintn Syscall_SendIPCMessage(in uint16 taskId, in uint64 u64, in const ascii str[32]) {
    if(taskId == 0) return 1;

    Task_Message taskMessage;
    taskMessage.type = Task_Message_IPCMessage;
    taskMessage.data.IPCMessage.u64 = u64;
    if(str != NULL) Functions_MemCpy(taskMessage.data.IPCMessage.str, str, sizeof(ascii)*32);

    Message_EnQueue(taskId, &taskMessage);
    Task_Yield();

    return 0;
}


//タスクの終了
sintn Syscall_Exit(in sintn retcode) {
    Task_Delete(Task_GetRunningTaskId());

    while(1) {
        Task_Halt();
    }

    return -1;
}


//stdioのTaskIoを取得
sintn Syscall_GetStdIoTaskId(out uint16* taskId) {
    if(taskId == NULL) return 1;

    *taskId = Task_GetStdIo(Task_GetRunningTaskId());
    if(taskId == 0) return 1;

    Task_Yield();

    return 0;
}


//stdioに文字列出力 count文字分までの表示を保証する
sintn Syscall_StdOut(in const ascii str[], uintn count) {
    uint16 sendToTaskId = Task_GetStdIo(Task_GetRunningTaskId());
    if(sendToTaskId == 0) return -1;
    if(count == 0) {
        Syscall_SendIPCMessage(sendToTaskId, 2, "\0");
        return 0;
    }

    for(uintn i=0; i<(count+31)/32; i++) {
        uintn breakFlag = 0;
        for(uintn k=0; k<32; k++) {
            if(str[k+i*32] == '\0') {
                breakFlag = 1;
                break;
            }
        }
        if(breakFlag) {
            Syscall_SendIPCMessage(sendToTaskId, 2, str+i*32);
            break;
        }else {
            Syscall_SendIPCMessage(sendToTaskId, 0, str+i*32);
        }
    }

    Task_Yield();

    return 0;
}


//stdinから文字取得
sintn Syscall_StdIn(out ascii str[], uintn strBuffSize) {
    if(str == NULL || strBuffSize == 0) return 1;

    uint16 sendToTaskId = Task_GetStdIo(Task_GetRunningTaskId());
    if(sendToTaskId == 0) return -1;

    Syscall_SendIPCMessage(sendToTaskId, 1, NULL);

    uintn buffindex = 0;

    Task_Message message;
    while(1) {
        Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_IPCMessage:
                if(strBuffSize <= buffindex + 32) return 2;
                if(message.data.IPCMessage.u64 == 2) {
                    uintn i;
                    for(i=0; i<32; i++) {
                        str[i+buffindex] = message.data.IPCMessage.str[i];
                        if(str[i+buffindex] == '\0') break;
                    }
                    return 0;
                }else {
                    uintn i;
                    for(i=0; i<32; i++) {
                        str[i+buffindex] = message.data.IPCMessage.str[i];
                        if(str[i+buffindex] == '\0') break;
                    }
                    if(str[i+buffindex] == '\0') return 0;
                }
                buffindex += 32;
                break;
            case Task_Message_CloseWindow:
                Syscall_Exit(1);
            case Task_Message_Quit:
                Syscall_Exit(1);
            default:
                break;
        }
    }

    return 0;
}


//stdoutの画面をクリア
sintn Syscall_StdOut_Cls(void) {
    uint16 sendToTaskId = Task_GetStdIo(Task_GetRunningTaskId());
    if(sendToTaskId == 0) return -1;

    Syscall_SendIPCMessage(sendToTaskId, 3, NULL);

    return 0;
}


//ディレクトリの内容物を取得
sintn Syscall_GetFileList(const ascii path[], uintn pathLength, uintn* buffCount, File_DirectoryEntry buff[]) {
    Task_Yield();

    for(uintn i=0; i<pathLength; i++) {
        if(path[i] == '\0') {
            if(File_GetDirectory(path, buffCount, buff)) return 1;
            return 0;
        }
    }

    return 2;
}


//パスからディレクトリエントリ取得
sintn Syscall_GetDirEntryByPath(const ascii path[], uintn pathLength, File_DirectoryEntry* buff) {
    if(path == NULL || buff == NULL) return 1;

    Task_Yield();

    for(uintn i=0; i<pathLength; i++) {
        if(path[i] == '\0') {
            uintn status = File_GetDirectoryEntryByPath(path, buff);
            if(status) return 2;
            return 0;
        }
    }

    return 3;
}


//パスのファイルをbuffからbuffSize分まで読み込む
sintn Syscall_MMapFile(const ascii path[], uintn pathLength, uintn buffSize, void* buff) {
    if(path == NULL || buffSize == 0 || buff == NULL) return 1;

    Task_Yield();

    for(uintn i=0; i<pathLength; i++) {
        if(path[i] == '\0') {
            if(File_OpenAndMMapFile(path, buffSize, buff)) return -1;
            return 0;
        }
    }
    
    return 2;
}


//パスのファイルに書き込み ないなら新規作成する
sintn Syscall_WriteFileFromMem(const ascii path[], uintn pathLength, uintn buffSize, void* buff) {
    if(path == NULL || (buffSize != 0 && buff == NULL)) return 1;

    Task_Yield();

    for(uintn i=0; i<pathLength; i++) {
        if(path[i] == '\0') {
            if(File_WriteFromMem(path, buffSize, buff)) return -1;
            return 0;
        }
    }

    return 2;
}


//ファイル消去
sintn Syscall_RemoveFile(const ascii path[], uintn pathLength) {
    if(path == NULL || pathLength == 0) return 1;

    Task_Yield();

    for(uintn i=0; i<pathLength; i++) {
        if(path[i] == '\0') {
            if(File_Remove(path)) return -1;
            return 0;
        }
    }

    return 0;
}


//ディレクトリ作成
sintn Syscall_MkDir(const ascii path[], uintn pathLength) {
    if(path == NULL || pathLength == 0) return 1;

    Task_Yield();

    uintn status;

    ascii path_temp[pathLength+3];
    for(uintn i=0; i<pathLength; i++) {
        path_temp[i] = path[i];
        if(path[i] == '\0') {
            path_temp[i] = '/';
            path_temp[i+1] = 'd';
            path_temp[i+2] = '\0';
            break;
        }
        if(i == pathLength-1 && path[i] != '\0') {
            return 2;
        }
    }

    ascii buff[1];
    buff[0] = '\0';
    status = File_WriteFromMem(path_temp, 1, buff);
    if(status) return 3;

    File_Remove(path_temp);

    return 0;
}


//シャットダウン
sintn Syscall_ShutDown(void) {
    Efi_Wrapper(
        KernelInput->LoadedImage->SystemTable->RuntimeServices->ResetSystem,
        EfiResetShutdown,
        0,
        0,
        NULL);
    return -1;
}


//ページごと(4KiB)のメモリ確保 *pageAddrにメモリアドレスが返される
sintn Syscall_AllocPage(uintn pages, void** pageAddr) {
    Task_Yield();

    if(pages == 0) {
        *pageAddr = NULL;
        return 0;
    }

    uint16 taskId = Task_GetRunningTaskId();
    if(taskId == 0) return -1;

    *pageAddr = Memory_AllocPages(taskId, pages);
    if(pageAddr == NULL) return -2;

    return 0;
}


//メモリ解放 pageAddrからpages分メモリ解放する
sintn Syscall_FreePages(uintn pages, void* pageAddr) {
    if(pageAddr == NULL) return 1;
    if(pages == 0) return 2;

    Task_Yield();

    uint16 taskId = Task_GetRunningTaskId();
    if(taskId == 0) return -1;

    Memory_FreePages(taskId, pages, pageAddr);

    return 0;
}


//リロケータブルELF形式実行可能ファイルを実行
sintn Syscall_RunApp(const ascii path[], uintn pathLength, optional ascii arg[32]) {
    Task_Yield();

    for(uintn i=0; i<pathLength; i++) {
        if(path[i] == '\0') break;
        if(i == pathLength-1) return 1;
    }

    //ファイルを開く
    File_DirectoryEntry dirEntBuff;
    if(File_GetDirectoryEntryByPath(path, &dirEntBuff)) return -1;

    uint16 runningTaskId = Task_GetRunningTaskId();
    if(runningTaskId == 0) return -2;

    uint8* elfbuff = Memory_AllocPages(runningTaskId, (dirEntBuff.size + 0xfff)>>12);
    if(elfbuff == NULL) return -3;

    if(File_OpenAndMMapFile(path, dirEntBuff.size, elfbuff)) {
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        return -4;
    }

    if(!ElfLoader_CheckElf(elfbuff)) {
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        return 1;
    }

    //ファイル展開
    uintn expandSize = 0;
    uintn loadAddr = 0;

    if(!ElfLoader_CheckDyn(elfbuff)) {
        return 2;
    }
    if(ElfLoader_GetLoadArea(elfbuff, &loadAddr, &expandSize)) {
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        return -5;
    }

    uint8* elfExpandBuff = Memory_AllocPages(runningTaskId, (expandSize + 0xfff)>>12);
    if(elfExpandBuff == NULL) {
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        return -6;
    }

    if(ElfLoader_Load(elfbuff, (uintn)elfExpandBuff)) {
        Memory_FreePages(runningTaskId, (expandSize + 0xfff)>>12, elfExpandBuff);
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        return -7;
    }

    void* entryPoint = NULL;
    if(ElfLoader_GetProperty(elfbuff, elfExpandBuff, &entryPoint, NULL)) {
        Memory_FreePages(runningTaskId, (expandSize + 0xfff)>>12, elfExpandBuff);
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        return -8;
    }

    uint16 terminal = Task_New(Terminal_Main, 0, path);
    if(terminal == 0) {
        Memory_FreePages(runningTaskId, (expandSize + 0xfff)>>12, elfExpandBuff);
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        return -9;
    }
    uint16 newTaskId = Task_New(entryPoint, terminal, arg);
    if(newTaskId == 0) {
        Memory_FreePages(runningTaskId, (expandSize + 0xfff)>>12, elfExpandBuff);
        Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);
        Task_Delete(terminal);
        return -10;
    }
    Task_ChangeStdIo(terminal, newTaskId);

    Memory_Move(runningTaskId, newTaskId, (expandSize + 0xfff)>>12, elfExpandBuff);

    Memory_FreePages(runningTaskId, (dirEntBuff.size + 0xfff)>>12, elfbuff);

    return 0;
}


//stdioを使用しない
sintn Syscall_ExitStdIo(void) {
    uint16 runningTaskId = Task_GetRunningTaskId();
    if(runningTaskId == 0) return -1;

    Task_ChangeStdIo(runningTaskId, 0);

    return 0;
}


//二択の質問　Okなら0,Noなら1を返す
sintn Syscall_Confirm(const ascii* question, uintn strlength) {
    if(question == NULL) return -1;

    uint16 runningTaskId = Task_GetRunningTaskId();
    uintn layerId = Layer_Window_New(runningTaskId, "Confirm", 10, 42, 300, 100);
    if(layerId == 0) return -1;

    Graphic_Color gray = {0xb0, 0xb0, 0xb0};
    Graphic_Color red = {0xee, 0xaa, 0xaa};
    Graphic_Color black = {0x00, 0x00, 0x00};

    Syscall_DrawSquare(layerId, 300-52*2, 100-35, 50, 32, gray);
    Syscall_DrawSquare(layerId, 300-52, 100-35, 50, 32, red);

    Syscall_DrawFont(layerId, 300-52*2+17, 100-35+10, 'O', black);
    Syscall_DrawFont(layerId, 300-52*2+25, 100-35+10, 'k', black);

    Syscall_DrawFont(layerId, 300-52+1, 100-35+10, 'C', black);
    Syscall_DrawFont(layerId, 300-52+1+8, 100-35+10, 'a', black);//Cancel
    Syscall_DrawFont(layerId, 300-52+1+8*2, 100-35+10, 'n', black);
    Syscall_DrawFont(layerId, 300-52+1+8*3, 100-35+10, 'c', black);
    Syscall_DrawFont(layerId, 300-52+1+8*4, 100-35+10, 'e', black);
    Syscall_DrawFont(layerId, 300-52+1+8*5, 100-35+10, 'l', black);

    for(uintn i=0; i<strlength; i++) {
        if(question[i] == '\0') break;
        Syscall_DrawFont(layerId, 10+i*8, 10, question[i], black);
    }

    Task_Message message;
    while(1) {
        Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Quit:
                Layer_Window_Delete(layerId);
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_CloseWindow:
                Layer_Window_Delete(layerId);
                return -1;
            case Task_Message_KeyPushed:
                if(message.data.KeyPushed.asciiCode == '\n') {
                    Layer_Window_Delete(layerId);
                    return 0;
                }
            case Task_Message_MouseLayerEvent:
                if(!message.data.MouseLayerEvent.leftButton) break;
                if(message.data.MouseLayerEvent.layerId != layerId) break;
                //Ok
                if(300-52*2 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 300-52*2+50
                    && 100-35 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 100-35+32) {
                    Layer_Window_Delete(layerId);
                    return 0;
                }
                //No
                if(300-52 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 300-52+50
                    && 100-35 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 100-35+32) {
                    Layer_Window_Delete(layerId);
                    return 1;
                }
            default:
                break;
        }
    }
}


//呼び出し元タスクのtaskIdを取得
sintn Syscall_GetThisTaskId(out uint16* taskId) {
    if(taskId == NULL) return 1;

    *taskId = Task_GetRunningTaskId();
    if(*taskId == 0) return -1;

    return 0;
}


//ディスプレイサイズ取得
sintn Syscall_GetDisplaySize(optional out uintn* width, optional out uintn* height) {
    if(width != NULL) *width = KernelInput->Graphic.width;
    if(height != NULL) *height = KernelInput->Graphic.height;
    return 0;
}


//通知
sintn Syscall_Alert(const ascii* str, uintn strlength) {
    if(str == NULL) return -1;

    uint16 runningTaskId = Task_GetRunningTaskId();
    uintn layerId = Layer_Window_New(runningTaskId, "Alert", 10, 42, 300, 100);
    if(layerId == 0) return -1;

    Graphic_Color gray = {0xb0, 0xb0, 0xb0};
    Graphic_Color black = {0x00, 0x00, 0x00};

    Syscall_DrawSquare(layerId, 300-52, 100-35, 50, 32, gray);

    Syscall_DrawFont(layerId, 300-52+17, 100-35+10, 'O', black);
    Syscall_DrawFont(layerId, 300-52+25, 100-35+10, 'k', black);

    for(uintn i=0; i<strlength; i++) {
        if(str[i] == '\0') break;
        Syscall_DrawFont(layerId, 10+i*8, 10, str[i], black);
    }

    Task_Message message;
    while(1) {
        Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Quit:
                Layer_Window_Delete(layerId);
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_CloseWindow:
                Layer_Window_Delete(layerId);
                return -2;
            case Task_Message_MouseLayerEvent:
                if(!message.data.MouseLayerEvent.leftButton) break;
                if(message.data.MouseLayerEvent.layerId != layerId) break;
                //Ok
                if(300-52 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 300-52+50
                    && 100-35 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 100-35+32) {
                    Layer_Window_Delete(layerId);
                    return 0;
                }
            default:
                break;
        }
    }
}


//エディタ
sintn Syscall_EditBox(uintn layerId, uintn x, uintn y, uintn height, out ascii buff[], uintn buffSize) {
    if(buff == NULL) return 1;

    sintn cursorX = 0;

    uintn width = buffSize*8;
    for(uintn i=0; i<buffSize; i++) {
        if(buff[i] == '\0') {
            cursorX = i;
            break;
        }
        if(i == buffSize-1) buff[0] = '\0';
    }

    Graphic_Color black = {0x00, 0x00, 0x00};
    Graphic_Color backcolor = {0xff, 0xff, 0xff};

    Graphic_Color shudow = {0x2f, 0x2f, 0x2f};
    Graphic_Color light = {0xd0, 0xd0, 0xd0};

    Task_Message message;
    while(1) {
        Syscall_DrawSquare(layerId, x, y, width, height, backcolor);
        Syscall_DrawSquare(layerId, x, y, width, 1, shudow);
        Syscall_DrawSquare(layerId, x, y+height-1, width, 1, light);
        Syscall_DrawSquare(layerId, x, y, 1, height, shudow);
        Syscall_DrawSquare(layerId, x+width-1, y, 1, height, shudow);
        
        for(uintn i=0; i<buffSize; i++) {
            if(buff[i] == '\0') break;
            Syscall_DrawFont(layerId, x+8*(i+1), y+height/2-6, buff[i], black);
        }
        Syscall_DrawSquare(layerId, x+8*(cursorX+1), y+height/2+8, 8, 2, black);

        Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Quit:
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_CloseWindow:
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -2;
            case Task_Message_MouseLayerEvent:
                if(message.data.MouseLayerEvent.leftButton) {
                    if(message.data.MouseLayerEvent.layerId != layerId) return 0;
                    if(!((sintn)x <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < (sintn)(x+width)
                        && (sintn)y <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < (sintn)(y+height))) {
                        Message_EnQueue(Task_GetRunningTaskId(), &message);
                        return 0;
                    }else {
                        sintn cursorX_tmp = (message.data.MouseLayerEvent.x-x)/8;
                        for(sintn i=0; i<cursorX_tmp-1; i++) {
                            if(buff[i] == '\0') {
                                cursorX_tmp = i;
                                break;
                            }
                        }
                        cursorX = cursorX_tmp;
                    }
                }
                break;
            case Task_Message_KeyPushed:
                if(message.data.KeyPushed.asciiCode != 0) {
                    if(message.data.KeyPushed.asciiCode == '\n') {
                        Message_EnQueue(Task_GetRunningTaskId(), &message);
                        return 0;
                    }
                    if(message.data.KeyPushed.asciiCode == 0x08) {
                        if(cursorX == 0) break;
                        cursorX --;
                        for(uintn i=cursorX; i<buffSize-1; i++) {
                            buff[i] = buff[i+1];
                        }
                        break;
                    }
                    if(cursorX == (sintn)buffSize) break;
                    for(sintn i=buffSize-2; cursorX<=i; i--) {
                        buff[i+1] = buff[i];
                    }
                    buff[cursorX] = message.data.KeyPushed.asciiCode;
                    cursorX++;
                }else {
                    switch(message.data.KeyPushed.scanCode) {
                        case EFI_SIMPLE_INPUT_SCANCODE_RIGHTARROW:
                            if(buff[cursorX] != '\0') cursorX++;
                            break;
                        case EFI_SIMPLE_INPUT_SCANCODE_LEFTARROW:
                            if(cursorX != 0) cursorX--;
                            break;
                        default:
                            break;
                    }
                }
            default:
                break;
        }
    }


    return 0;
}


//プロンプト
sintn Syscall_Prompt(const ascii str[], uintn strLength, out ascii buff[], uintn buffLength) {
    if(str == NULL || buff == NULL) return 1;

    uint16 runningTaskId = Task_GetRunningTaskId();
    uintn layerId = Layer_Window_New(runningTaskId, "Prompt", 10, 42, 300, 150);
    if(layerId == 0) return -1;

    Graphic_Color gray = {0xb0, 0xb0, 0xb0};
    Graphic_Color black = {0x00, 0x00, 0x00};
    Graphic_Color editor_backcolor = {0xff, 0xff, 0xff};
    Graphic_Color uicolor = {0xf0, 0xf0, 0xf0};
    Syscall_DrawSquare(layerId, 300-51, 150-33, 50, 32, gray);
    Syscall_DrawFont(layerId, 300-51+25-8, 150-33+10, 'O', black);
    Syscall_DrawFont(layerId, 300-51+25, 150-33+10, 'k', black);
    
    for(uintn i=0; i<strLength; i++) {
        if(str[i] == '\0') break;
        Syscall_DrawFont(layerId, 10+i*8, 10, str[i], black);
    }

    Syscall_DrawSquare(layerId, 9, 150-52-32-1, buffLength*8+2, 32+2, uicolor);
    Syscall_DrawSquare(layerId, 10, 150-52-32, buffLength*8, 32, editor_backcolor);

    Syscall_EditBox(layerId, 10, 150-52-32, 32, buff, buffLength);

    Task_Message message;
    while(1) {
        Syscall_ReadMessage(&message);

        switch(message.type) {
            case Task_Message_Quit:
                Layer_Window_Delete(layerId);
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_CloseWindow:
                Layer_Window_Delete(layerId);
                return -2;
            case Task_Message_MouseLayerEvent:
                if(message.data.MouseLayerEvent.layerId != layerId) break;
                if(10 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < (sintn)(100+buffLength*8)
                    && 150-52-32 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 150-52) {
                    Syscall_EditBox(layerId, 10, 150-52-32, 32, buff, buffLength);
                }
                if(300-51 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 300-1
                    && 150-33 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 150-1) {
                    Layer_Window_Delete(layerId);
                    return 0;
                }
                break;
            case Task_Message_KeyPushed:
                if(message.data.KeyPushed.asciiCode == '\n') {
                    Layer_Window_Delete(layerId);
                    return 0;
                }
                break;
            default:
                break;
        }
    }

    return 0;
}


static sintn Syscall_TextBox_GetStrIndexByLine(const ascii* str, uintn index, uintn max) {
    uintn seekindex = 0;

    for(uintn i=0; i<index; i++) {
        for(uintn k=0; k<max; k++) {
            if(str[seekindex] == '\n') {
                seekindex ++;
                break;
            }
            if(str[seekindex] == '\0') {
                return -1;
            }
            seekindex ++;
        }
    }
    return seekindex;
}

sintn Syscall_TextBox(uintn layerId, uintn x, uintn y, App_Syscall_TextBox_Data* data) {
    if(data == NULL || data->buff == NULL || data->buffSize == 0) return 1;
//width/8;
    data->buff[data->buffSize-1] = '\0';

    Graphic_Color black = {0x00, 0x00, 0x00};
    Graphic_Color white = {0xff, 0xff, 0xff};

    Task_Message message;

    if(Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8) < 0) {
        data->cursorX = 0;
        data->cursorY = 0;
    }else {
        ascii* str = data->buff + Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
        for(uintn i=0; i<data->cursorX-1; i++) {
            if(str[i] == '\0') {
                data->cursorX = 0;
                data->cursorY = 0;
                break;
            }
        }
    }

    uintn changedFlag = 1;

    while(1) {
        if((sintn)data->cursorY-(sintn)data->scroll < 0) {
            data->scroll = data->cursorY;
        }
        if((sintn)data->height/16 <= (sintn)data->cursorY-(sintn)data->scroll) {
            data->scroll = data->cursorY - data->height/16 + 1;
        }

        if(changedFlag) {
            changedFlag = 0;
            for(uintn i=0; i<data->height/16; i++) {
                sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, i+data->scroll, data->width/8);

                Syscall_DrawSquare(layerId, x, y+i*16, data->width, 16, white);

                if(index < 0) break;
                for(uintn k=0; 1; k++) {
                    if(data->buff[index+k] == '\n' || data->buff[index+k] == '\0') break;

                    Syscall_DrawFont(layerId, x+8*k, y+i*16, data->buff[index+k], black);
                }
            }
            if(0 <= y+16*((sintn)data->cursorY - data->scroll)+14) Syscall_DrawSquare(layerId, x+8*data->cursorX, y+16*((sintn)data->cursorY - data->scroll)+14, 8, 2, black);
        }

        Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Quit:
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_CloseWindow:
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -2;
            case Task_Message_KeyPushed:
                changedFlag = 1;
                if(message.data.KeyPushed.asciiCode != 0) {

                    for(uintn i=0; 1; i++) {
                        if(data->buff[i] == '\0') {

                            if(message.data.KeyPushed.asciiCode == 0x08) {
                                if(data->cursorX == 0 && data->cursorY == 0) break;

                                if(data->cursorX == 0) {
                                    sintn index2 = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY-1, data->width/8);
                                    if(index2 < 0) return -3;
                                    for(uintn k=index2; 1; k++) {
                                        if(data->buff[k] == '\n' || data->buff[k] == '\0') {
                                            data->cursorX = k-index2;
                                            break;
                                        }
                                    }
                                    data->cursorY --;
                                }else {
                                    data->cursorX --;
                                }

                                sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
                                for(uintn i=index+data->cursorX; i<data->buffSize; i++) {
                                    data->buff[i] = data->buff[i+1];
                                    if(data->buff[i] == '\0') break;
                                }
                            }else {
                                sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
                                if(i+2 == data->buffSize) {
                                    break;
                                }
                                for(sintn k=i; (sintn)(index+data->cursorX)<=k; k--) {
                                    data->buff[k+1] = data->buff[k];
                                }
                                data->buff[index+data->cursorX] = message.data.KeyPushed.asciiCode;
                                data->cursorX ++;
                                if(message.data.KeyPushed.asciiCode == '\n' || data->cursorX == data->width/8) {
                                    data->cursorX = 0;
                                    data->cursorY ++;
                                }
                            }
                            break;
                        }
                    }

                }else {
                    switch(message.data.KeyPushed.scanCode) {
                        case EFI_SIMPLE_INPUT_SCANCODE_RIGHTARROW:;
                            sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
                            if(data->buff[index+data->cursorX] == '\n') {
                                data->cursorX = 0;
                                data->cursorY ++;
                            }else if(data->buff[index+data->cursorX] == '\0') {
                                //do nothing
                            }else {
                                data->cursorX++;
                            }
                            break;
                        case EFI_SIMPLE_INPUT_SCANCODE_LEFTARROW:
                            if(data->cursorX == 0) {
                                if(data->cursorY != 0) {
                                    data->cursorY --;
                                    sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
                                    for(uintn i=index; 1; i++) {
                                        if(data->buff[i] == '\n' || data->buff[i] == '\0') {
                                            data->cursorX = i-index;
                                            break;
                                        }
                                    }
                                }
                            }else {
                                data->cursorX --;
                            }
                            break;
                        case EFI_SIMPLE_INPUT_SCANCODE_UPARROW:
                            if(data->cursorY == 0) {
                                data->cursorX = 0;
                            }else {
                                data->cursorY --;
                                sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
                                for(uintn i=index; i<index+data->cursorX; i++) {
                                    if(data->buff[i] == '\0' || data->buff[i] == '\n') {
                                        data->cursorX = i-index;
                                        break;
                                    }
                                }
                            }
                            break;
                        case EFI_SIMPLE_INPUT_SCANCODE_DOWNARROW:
                            if(Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY+1, data->width/8) < 0) {
                                sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
                                for(uintn i=index; 1; i++) {
                                    if(data->buff[i] == '\n' || data->buff[i] == '\0') {
                                        data->cursorX = i-index;
                                        break;
                                    }
                                }
                            }else {
                                data->cursorY ++;
                                sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->cursorY, data->width/8);
                                for(uintn i=index; i<index+data->cursorX; i++) {
                                    if(data->buff[i] == '\n' || data->buff[i] == '\0') {
                                        data->cursorX = i-index;
                                        break;
                                    }
                                }
                            }
                            break;
                        default:
                            break;
                    }
                }

                break;
            case Task_Message_MouseLayerEvent:
                if(message.data.MouseLayerEvent.layerId == layerId && message.data.MouseLayerEvent.leftButton == 1) {
                    if((sintn)x <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < (sintn)(x+data->width)
                        && (sintn)y <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < (sintn)(y+data->height)) {
                        sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, data->scroll + (message.data.MouseLayerEvent.y - y)/16, data->width/8);
                        if(index < 0) {
                            for(uintn i=0; 1; i++) {
                                if(Syscall_TextBox_GetStrIndexByLine(data->buff, i, data->width/8) < 0) {
                                    if(i == 0) {
                                        data->cursorX = 0;
                                        data->cursorY = 0;
                                    }else {
                                        sintn index = Syscall_TextBox_GetStrIndexByLine(data->buff, i-1, data->width/8);
                                        data->cursorY = i-1;
                                        for(uintn k=0; 1; k++) {
                                            if(data->buff[index + k] == '\0' || data->buff[index + k] == '\n') {
                                                data->cursorX = k;
                                                break;
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }else {
                            for(uintn i=0; i<(message.data.MouseLayerEvent.x - x)/8; i++) {
                                if(data->buff[index + i] == '\0' || data->buff[index + i] == '\n') {
                                    data->cursorX = i;
                                    data->cursorY = data->scroll + (message.data.MouseLayerEvent.y - y)/16;
                                    break;
                                }
                                if(i == ((message.data.MouseLayerEvent.x - x)/8)-1) {
                                    data->cursorX = (message.data.MouseLayerEvent.x - x)/8;
                                    data->cursorY = data->scroll + (message.data.MouseLayerEvent.y - y)/16;
                                }
                            }
                        }
                        changedFlag = 1;
                    }else {
                        Message_EnQueue(Task_GetRunningTaskId(), &message);
                        return 0;
                    }
                }
                break;
            default:
                break;
        }
    }

    return 0;
}


sintn Syscall_SetClipBoard(const ascii* str, uintn length) {
    if(Clip_Set(str, length)) {
        return 1;
    }

    return 0;
}


sintn Syscall_GetClipBoard(out ascii* buff, in out uintn* buffsize) {
    if(Clip_Get(buff, buffsize)) {
        return 1;
    }

    return 0;
}


typedef enum {
    Syscall_DrawButton_State_Pushed,
    Syscall_DrawButton_State_Normal,
    Syscall_DrawButton_State_NotActive
} Syscall_DrawButton_State;

static void Syscall_DrawButton_(uintn layerId, uintn x, uintn y, uintn width, uintn height, in const ascii* str, Syscall_DrawButton_State state) {
    Syscall_DrawSquare(layerId, x, y, width, height, ui_color);

    uintn str_length = 0;
    for(str_length = 0; 1; str_length++) {
        if(str[str_length] == '\0') break;
    }

    switch(state) {
        case Syscall_DrawButton_State_Pushed:
            Syscall_DrawSquare(layerId, x, y, width, 1, shudow);
            Syscall_DrawSquare(layerId, x, y+height-1, width, 1, light);
            Syscall_DrawSquare(layerId, x, y, 1, height, shudow);
            Syscall_DrawSquare(layerId, x+width-1, y, 1, height, light);
            for(uintn i=0; i<str_length; i++) {
                Syscall_DrawFont(layerId, x+width/2-str_length*8/2+8*i+1, y+height/2-6+1, str[i], black);
            }
            break;
        case Syscall_DrawButton_State_Normal:
            Syscall_DrawSquare(layerId, x, y, width, 1, light);
            Syscall_DrawSquare(layerId, x, y+height-1, width, 1, shudow);
            Syscall_DrawSquare(layerId, x, y, 1, height, light);
            Syscall_DrawSquare(layerId, x+width-1, y, 1, height, shudow);
            for(uintn i=0; i<str_length; i++) {
                Syscall_DrawFont(layerId, x+width/2-str_length*8/2+8*i, y+height/2-6, str[i], black);
            }
            break;
        case Syscall_DrawButton_State_NotActive:
            Syscall_DrawSquare(layerId, x, y, width, 1, gray);
            Syscall_DrawSquare(layerId, x, y+height-1, width, 1, gray);
            Syscall_DrawSquare(layerId, x, y, 1, height, gray);
            Syscall_DrawSquare(layerId, x+width-1, y, 1, height, gray);
            for(uintn i=0; i<str_length; i++) {
                Syscall_DrawFont(layerId, x+width/2-str_length*8/2+8*i, y+height/2-6, str[i], gray);
            }
            break;
    }

    return;    
}

sintn Syscall_DrawButton(uintn layerId, uintn x, uintn y, uintn width, uintn height, in const ascii* str) {
    Syscall_DrawButton_(layerId, x, y, width, height, str, Syscall_DrawButton_State_Normal);

    return 0;
}

sintn Syscall_DrawButton_Pushed(uintn layerId, uintn x, uintn y, uintn width, uintn height, in const ascii* str) {
    Syscall_DrawButton_(layerId, x, y, width, height, str, Syscall_DrawButton_State_Pushed);

    Task_Message message;
    while(1) {
        Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Quit:
                Layer_Window_Delete(layerId);
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_MouseLayerEvent:
                if(message.data.MouseLayerEvent.leftButton == 0) {
                    Syscall_DrawButton_(layerId, x, y, width, height, str, Syscall_DrawButton_State_Normal);
                    return 0;
                }
            default:
                break;
        }
    }

    return -2;
}

sintn Syscall_DrawSquare_NotActive(uintn layerId, uintn x, uintn y, uintn width, uintn height, in const ascii* str) {
    Syscall_DrawButton_(layerId, x, y, width, height, str, Syscall_DrawButton_State_NotActive);
    return 0;
}

