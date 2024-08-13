#include <types.h>
#include <kernel.h>
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

#define Syscall_SyscallAddr ((void**)0x100000)

sintn Syscall_AppEnter();

void Syscall_Init(void) {
    Console_Print("Syscall_Init: Initializing Syscall...\n");

    *Syscall_SyscallAddr = (void*)Syscall_AppEnter;

    return;
}


//新規ウインドウ作成してLayerIdを返す
sintn Syscall_NewWindow(out uintn* layerId, in uintn x, in uintn y, in uintn width, in uintn height, in ascii title[]) {
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


//stdioのTaskIdを取得
sintn Syscall_GetStdOutTaskId(out uint16* taskId) {
    if(taskId == NULL) return 1;

    *taskId = Task_GetStdOut(Task_GetRunningTaskId());
    if(taskId == 0) return 1;

    return 0;
}


//stdioに文字列出力 count文字分までの表示を保証する
sintn Syscall_StdOut(in const ascii str[], uintn count) {
    uint16 sendToTaskId = Task_GetStdOut(Task_GetRunningTaskId());
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
    return 0;
}


//stdinから文字取得
sintn Syscall_StdIn(out ascii str[], uintn strBuffSize) {
    if(str == NULL || strBuffSize == 0) return 1;

    uint16 sendToTaskId = Task_GetStdIn(Task_GetRunningTaskId());
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
    uint16 sendToTaskId = Task_GetStdOut(Task_GetRunningTaskId());
    if(sendToTaskId == 0) return -1;

    Syscall_SendIPCMessage(sendToTaskId, 3, NULL);

    return 0;
}


//ディレクトリの内容物を取得
sintn Syscall_GetFileList(const ascii path[], uintn pathLength, uintn* buffCount, File_DirectoryEntry buff[]) {
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

    for(uintn i=0; i<pathLength; i++) {
        if(path[i] == '\0') {
            if(File_WriteFromMem(path, buffSize, buff)) return -1;
            return 0;
        }
    }

    return 2;
}


//メモリ確保
