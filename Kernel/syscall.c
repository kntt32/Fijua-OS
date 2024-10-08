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

static uint16 uplogo[] = {
#include "uplogo_bitmap"
};

static uint16 downlogo[] = {
#include "downlogo_bitmap"
};

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

    if(path[0] == '\0') {
        return 2;
    }

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
            return 3;
        }
    }

    ascii buff[1];
    buff[0] = '\0';
    status = File_WriteFromMem(path_temp, 1, buff);
    if(status) return 4;

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

    Syscall_DrawButton(layerId, 300-52*2, 100-35, 50, 32, "Ok");
    Syscall_DrawButton(layerId, 300-52, 100-35, 50, 32, "Cancel");

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
                    Syscall_DrawButton_Pushed(layerId, 300-52*2, 100-35, 50, 32, "Ok");
                    Layer_Window_Delete(layerId);
                    return 0;
                }
                //No
                if(300-52 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 300-52+50
                    && 100-35 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 100-35+32) {
                    Syscall_DrawButton_Pushed(layerId, 300-52, 100-35, 50, 32, "Cancel");
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

    Syscall_DrawButton(layerId, 300-52, 100-35, 50, 32, "Ok");

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
                    Syscall_DrawButton_Pushed(layerId, 300-52, 100-35, 50, 32, "Ok");
                    Layer_Window_Delete(layerId);
                    return 0;
                }
            default:
                break;
        }
    }
}


//エディタ
sintn Syscall_EditBox_Draw(uintn layerId, in out App_Syscall_EditBox_Data* data);

static sintn Syscall_EditBox_Draw_(uintn layerId, in out App_Syscall_EditBox_Data* data, uint8 hideCursor);

static sintn Syscall_EditBox_GetIndex(uintn x, uintn y, in App_Syscall_EditBox_Data* data) {
    uintn seek_x = 0;
    uintn seek_y = 0;

    for(sintn index=0; index<(sintn)data->buffSize; index++) {
        if(x == seek_x && y == seek_y) {
            return index;
        }

        if(data->buff[index] == '\0') return -1;

        seek_x ++;
        if(data->width < seek_x*8+4+8 || data->buff[index] == '\n') {
            seek_x = 0;
            seek_y ++;
            if(y < seek_y) return -1;
        }
    }
    return -1;
}

static uintn Syscall_EditBox_TextLength(in App_Syscall_EditBox_Data* data) {
    for(uintn i=0; i<data->buffSize; i++) {
        if(data->buff[i] == '\0') {
            return i;
        }
    }
    return data->buffSize;
}

static sintn Syscall_EditBox_GetOffset(uintn index, optional out uintn* x, optional out uintn* y, in App_Syscall_EditBox_Data* data) {
    uintn seek_x = 0;
    uintn seek_y = 0;

    for(uintn index2=0; index2<data->buffSize; index2++) {
        if(index2 == index) {
            if(x != NULL) *x = seek_x;
            if(y != NULL) *y = seek_y;
            return 0;
        }
        if(data->buff[index2] == '\0') return 1;

        seek_x ++;
        if(data->width < seek_x*8+4+8 || data->buff[index2] == '\n') {
            seek_x = 0;
            seek_y ++;
        }
    }

    return 1;
}

static sintn Syscall_EditBox_MaxXWithY_Index(uintn y, App_Syscall_EditBox_Data* data) {
    sintn index = Syscall_EditBox_GetIndex(0, y, data);
    if(index < 0) {
        return -1;
    }

    for(uintn i=index; i<data->buffSize; i++) {
        if(data->buff[i] == '\n' || data->buff[i] == '\0') {
            return i;
        }
    }

    return -1;
}

static void Syscall_EditBox_GetXYByMouse(sintn mouseX, sintn mouseY, out uintn* offsetX, out uintn* offsetY, in App_Syscall_EditBox_Data* data) {
    sintn x = (mouseX-data->x-4)/8;
    sintn y = (mouseY-data->y-8)/20;

    if(x < 0) x = 0;
    if(y < 0) y = 0;

    sintn index = Syscall_EditBox_GetIndex(x, y, data);
    if(index < 0) {
        uintn textLength = Syscall_EditBox_TextLength(data);
        if(textLength == 0) {
            *offsetX = 0;
            *offsetY = 0;
        }else {
            Syscall_EditBox_GetOffset(textLength-1, offsetX, offsetY, data);
            (*offsetX) ++;
            if(data->width < (*offsetX)*8+4+8) {
                *offsetX = 0;
                (*offsetY) ++;
            }
        }
    }else {
        *offsetX = x;
        *offsetY = y;
    }

    return;
}


static void Syscall_EditBox_InsertStr(ascii buff[], in out App_Syscall_EditBox_Data* data) {
    uintn buffLength = 0;
    for(buffLength = 0; ; buffLength++) {
        if(buff[buffLength] == '\0') break;
    }

    sintn startIndex = Syscall_EditBox_GetIndex(data->cursor_startX, data->cursor_startY, data);
    sintn endIndex = Syscall_EditBox_GetIndex(data->cursor_endX, data->cursor_endY, data);
    if((sintn)buffLength < (endIndex - startIndex)) {
        for(uintn i=0; i<buffLength; i++) {
            data->buff[startIndex+i] = buff[i];
        }
        for(uintn i=endIndex; i<data->buffSize; i++) {
            data->buff[i-(endIndex - startIndex)+buffLength] = data->buff[i];
        }
    }else {
        if(data->buffSize <= endIndex + buffLength + 1) return;
        for(sintn i=data->buffSize-(buffLength-(endIndex-startIndex))-1; endIndex<=i; i--) {
            data->buff[i+(buffLength-(endIndex-startIndex))] = data->buff[i];
        }
        for(uintn i=0; i<buffLength; i++) {
            data->buff[startIndex+i] = buff[i];
        }
    }

    Syscall_EditBox_GetOffset(startIndex + buffLength, &data->cursor_startX, &data->cursor_startY, data);
    data->cursor_endX = data->cursor_startX;
    data->cursor_endY = data->cursor_startY;

    return;
}


sintn Syscall_EditBox_Response(uintn layerId, uintn mouseX, uintn mouseY, in out App_Syscall_EditBox_Data* data) {
    if(!(data->x <= mouseX && mouseX < data->x+data->width && data->y <= mouseY && mouseY < data->y+data->height)) return 0;

    Syscall_EditBox_GetXYByMouse(mouseX, mouseY, &data->cursor_startX, &data->cursor_startY, data);
    data->cursor_endX = data->cursor_startX;
    data->cursor_endY = data->cursor_startY;

    Task_Message message;
    while(1) {
        Syscall_EditBox_Draw_(layerId, data, 0);

        Syscall_ReadMessage(&message);

        switch(message.type) {
            case Task_Message_Quit:
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_CloseWindow:
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_MouseLayerEvent:
                if(message.data.MouseLayerEvent.layerId == layerId) {
                    if(message.data.MouseLayerEvent.leftButton == 1
                        && !((sintn)data->x <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < (sintn)(data->x+data->width)
                            && (sintn)data->y <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < (sintn)(data->y+data->height))) {
                        Syscall_EditBox_Draw(layerId, data);
                        Message_EnQueue(Task_GetRunningTaskId(), &message);
                        return 0;
                    }

                    if(message.data.MouseLayerEvent.leftButton) {
                        Syscall_EditBox_GetXYByMouse(message.data.MouseLayerEvent.x, message.data.MouseLayerEvent.y, &data->cursor_startX, &data->cursor_startY, data);
                        data->cursor_endX = data->cursor_startX;
                        data->cursor_endY = data->cursor_startY;

                        while(1) {
                            Syscall_EditBox_Draw_(layerId, data, 0);
                            uint8 flag = 0;
                            Task_Message message2;
                            Syscall_ReadMessage(&message2);

                            switch(message2.type) {
                                case Task_Message_Quit:
                                    Message_EnQueue(Task_GetRunningTaskId(), &message);
                                    return -1;
                                case Task_Message_CloseWindow:
                                    Message_EnQueue(Task_GetRunningTaskId(), &message);
                                    return -1;
                                case Task_Message_MouseLayerEvent:
                                    if(!message2.data.MouseLayerEvent.leftButton) {
                                        flag = 1;
                                        break;
                                    }

                                    uintn cursor_newX;
                                    uintn cursor_newY;
                                    Syscall_EditBox_GetXYByMouse(message2.data.MouseLayerEvent.x, message2.data.MouseLayerEvent.y, &cursor_newX, &cursor_newY, data);
                                    if(cursor_newY < data->cursor_startY || (cursor_newY == data->cursor_startY && cursor_newX <= data->cursor_startX)) {
                                        data->cursor_startX = cursor_newX;
                                        data->cursor_startY = cursor_newY;
                                    }else if(data->cursor_endY < cursor_newY || (cursor_newY == data->cursor_endY && data->cursor_endX <= cursor_newX)) {
                                        data->cursor_endX = cursor_newX;
                                        data->cursor_endY = cursor_newY;
                                    }else {
                                        data->cursor_endX = cursor_newX;
                                        data->cursor_endY = cursor_newY;
                                    }
                                    break;
                                default:
                                    flag = 1;
                                    break;
                            }

                            if(flag) break;
                        }
                    }
                }

                break;
            case Task_Message_KeyPushed:
                if(data->allowInput) {
                    if(message.data.KeyPushed.scanCode == 0) {
                        if((0x21 <= message.data.KeyPushed.asciiCode && message.data.KeyPushed.asciiCode <= 0x7e) || message.data.KeyPushed.asciiCode == '\n') {
                            ascii str[2];
                            str[0] = message.data.KeyPushed.asciiCode;
                            str[1] = '\0';
                            Syscall_EditBox_InsertStr(str, data);
                        }

                        if(message.data.KeyPushed.asciiCode == 0x08) {
                            sintn startIndex = Syscall_EditBox_GetIndex(data->cursor_startX, data->cursor_startY, data);
                            sintn endIndex = Syscall_EditBox_GetIndex(data->cursor_endX, data->cursor_endY, data);

                            if(startIndex != endIndex) {
                                Syscall_EditBox_InsertStr("", data);
                            }else if(0 < startIndex) {
                                Syscall_EditBox_GetOffset(startIndex-1, &data->cursor_startX, &data->cursor_startY, data);
                                Syscall_EditBox_InsertStr("", data);
                            }
                        }

                        if(message.data.KeyPushed.asciiCode == 3) {
                            //copy
                            sintn startIndex = Syscall_EditBox_GetIndex(data->cursor_startX, data->cursor_startY, data);
                            sintn endIndex = Syscall_EditBox_GetIndex(data->cursor_endX, data->cursor_endY, data);
                            if(startIndex < 0 || endIndex < 0) break;
                            Clip_Set(data->buff+startIndex, endIndex-startIndex+1);
                        }else if(message.data.KeyPushed.asciiCode == 22) {
                            //past
                            uintn buffSize = 0;
                            Clip_Get(NULL, &buffSize);
                            ascii buff[buffSize];
                            if(Clip_Get(buff, &buffSize)) {
                                break;
                            }
                            Syscall_EditBox_InsertStr(buff, data);
                        }else if(message.data.KeyPushed.asciiCode == 1) {
                            //select all
                            for(uintn i=0; i<data->buffSize; i++) {
                                if(data->buff[i] == '\0') {
                                    Syscall_EditBox_GetOffset(i, &data->cursor_endX, &data->cursor_endY, data);
                                }
                            }
                            data->cursor_startX = 0;
                            data->cursor_startY = 0;
                        }else if(message.data.KeyPushed.asciiCode == 24) {
                            //copy and delete
                            sintn startIndex = Syscall_EditBox_GetIndex(data->cursor_startX, data->cursor_startY, data);
                            sintn endIndex = Syscall_EditBox_GetIndex(data->cursor_endX, data->cursor_endY, data);
                            if(startIndex < 0 || endIndex < 0) break;
                            Clip_Set(data->buff+startIndex, endIndex-startIndex+1);
                            Syscall_EditBox_InsertStr("", data);
                        }

                    }else {
                        //up

                        //down

                        //right

                        //left

                    }
                }
                
                break;
            default:
                break;
        }
    }

    Syscall_EditBox_Draw(layerId, data);

    return 0;
}

static sintn Syscall_EditBox_Draw_(uintn layerId, in out App_Syscall_EditBox_Data* data, uint8 hideCursor) {
    Graphic_FrameBuff framebuff;
    if(Layer_Window_GetFrameBuff(layerId, &framebuff)) return -1;

    Syscall_DrawSquare(layerId, data->x, data->y, data->width, data->height, white);
    Syscall_DrawSquare(layerId, data->x, data->y, 1, data->height, shudow);
    Syscall_DrawSquare(layerId, data->x+data->width-1, data->y, 1, data->height, light);
    Syscall_DrawSquare(layerId, data->x, data->y, data->width, 1, shudow);
    Syscall_DrawSquare(layerId, data->x, data->y+data->height-1, data->width, 1, light);

    Graphic_FrameBuff editor_buff;
    editor_buff.frameBuff = framebuff.frameBuff+data->x+data->y*framebuff.scanlineWidth;
    editor_buff.width = MIN(data->width, framebuff.width);
    editor_buff.height = MIN(data->height, framebuff.height);
    editor_buff.scanlineWidth = framebuff.scanlineWidth;

    for(uintn y=0; y<data->buffSize/((data->width-4)/8); y++) {
        for(uintn x=0; x<(data->width-4)/8; x++) {
            sintn index = Syscall_EditBox_GetIndex(x, y, data);

            if(0 <= y*20+8-data->scroll) {
                if(!hideCursor) {
                    if((data->cursor_startY <= y && y <= data->cursor_endY)
                        && (y != data->cursor_startY || data->cursor_startX <= x)
                        && (y != data->cursor_endY || x <= data->cursor_endX)) {
                        Graphic_FrameBuff_DrawSquare(editor_buff, x*8+4, y*20+8-data->scroll+16, 8, 2, black);
                    }
                }

                if(data->buff[index] == '\0') continue;

                if(0 <= index) {
                    Font_Draw(editor_buff, x*8+4, y*20+8-data->scroll, data->buff[index], black);
                }
            }
        }
    }

    return 0;
}


sintn Syscall_EditBox_Draw(uintn layerId, in out App_Syscall_EditBox_Data* data) {
    uintn cursor_startX = 0;
    uintn cursor_startY = 0;

    data->cursor_startX = 0;
    data->cursor_startY = 0;
    data->cursor_endX = 0;
    data->cursor_endY = 0;

    return Syscall_EditBox_Draw_(layerId, data, 1);
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

    Syscall_DrawButton(layerId, 300-51, 150-33, 50, 32, "Ok");
    
    for(uintn i=0; i<strLength; i++) {
        if(str[i] == '\0') break;
        Syscall_DrawFont(layerId, 10+i*8, 10, str[i], black);
    }

    Syscall_DrawSquare(layerId, 9, 150-52-32-1, buffLength*8+2, 32+2, uicolor);
    Syscall_DrawSquare(layerId, 10, 150-52-32, buffLength*8, 32, editor_backcolor);

    //Syscall_EditBox(layerId, 10, 150-52-32, 32, buff, buffLength);

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
                if(!message.data.MouseLayerEvent.leftButton) break;
                if(10 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < (sintn)(100+buffLength*8)
                    && 150-52-32 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 150-52) {
                    //Syscall_EditBox(layerId, 10, 150-52-32, 32, buff, buffLength);
                }
                if(300-51 <= message.data.MouseLayerEvent.x && message.data.MouseLayerEvent.x < 300-1
                    && 150-33 <= message.data.MouseLayerEvent.y && message.data.MouseLayerEvent.y < 150-1) {
                    Syscall_DrawButton_Pushed(layerId, 300-51, 150-33, 50, 32, "Ok");
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
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return -1;
            case Task_Message_CloseWindow:
                Message_EnQueue(Task_GetRunningTaskId(), &message);
                return 0;
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

sintn Syscall_DrawButton_NotActive(uintn layerId, uintn x, uintn y, uintn width, uintn height, in const ascii* str) {
    Syscall_DrawButton_(layerId, x, y, width, height, str, Syscall_DrawButton_State_NotActive);
    return 0;
}

inline static void Syscall_DrawScrollBar_UpLogo(uintn layerId, uintn x, uintn y) {
    for(uintn iy=0; iy<16; iy++) {
        uint16 bitmap_line = uplogo[iy];
        for(uintn ix=0; ix<16; ix++) {
            if(bitmap_line & (0x8000 >> ix)) {
                Syscall_DrawSquare(layerId, x+ix, y+iy, 1, 1, black);
            }
        }
    }

    return;
}

inline static void Syscall_DrawScrollBar_DownLogo(uintn layerId, uintn x, uintn y) {
    for(uintn iy=0; iy<16; iy++) {
        uint16 bitmap_line = downlogo[iy];
        for(uintn ix=0; ix<16; ix++) {
            if(bitmap_line & (0x8000 >> ix)) {
                Syscall_DrawSquare(layerId, x+ix, y+iy, 1, 1, black);
            }
        }
    }

    return;
}

sintn Syscall_DrawScrollBar(uintn layerId, App_Syscall_Scrollbar_Data* data) {
    uintn x = data->x;
    uintn y = data->y;
    uintn height = data->height;
    uintn offset = data->offset;
    uintn page_height = data->page_height;

    Syscall_DrawSquare(layerId, x, y, 16, height, gray);
    Syscall_DrawSquare(layerId, x, y, 1, height, shudow);
    Syscall_DrawSquare(layerId, x+16-1, y, 1, height, light);
    Syscall_DrawSquare(layerId, x, y, 16, 1, shudow);
    Syscall_DrawSquare(layerId, x, y+height-1, 16, 1, light);

    uintn drawX = x+1;
    uintn drawY = y+16;
    uintn drawHeight = height - 32;
    if(height < page_height) {
        drawHeight = (height-32)*height/page_height;
        drawY = y+16+offset*(height-32-drawHeight)/(page_height - height);
    }
    Syscall_DrawSquare(layerId, drawX, drawY, 14, drawHeight, ui_color);
    Syscall_DrawSquare(layerId, drawX, drawY, 1, drawHeight, light);
    Syscall_DrawSquare(layerId, drawX+14-1, drawY, 1, drawHeight, shudow);
    Syscall_DrawSquare(layerId, drawX, drawY, 14, 1, light);
    Syscall_DrawSquare(layerId, drawX, drawY+drawHeight-1, 14, 1, shudow);

    Syscall_DrawButton(layerId, x+1, y+1, 14, 14, "");
    Syscall_DrawScrollBar_UpLogo(layerId, x, y);

    Syscall_DrawButton(layerId, x+1, y+height-15, 14, 14, "");
    Syscall_DrawScrollBar_DownLogo(layerId, x, y+height-16);
    
    return 0;
}

sintn Syscall_DrawScrollBar_Response(uintn layerId, App_Syscall_Scrollbar_Data* data, uintn mouseX, uintn mouseY) {
    uintn x = data->x;
    uintn y = data->y;
    uintn height = data->height;
    uintn offset = data->offset;
    uintn page_height = data->page_height;

    if(x <= mouseX && mouseX < x+16 && y <= mouseY && mouseY < y+height) {
        //up
        if(mouseY < y+16) {
            Syscall_DrawButton_Pushed(layerId, x+1, y+1, 14, 14, "");

            if(data->offset < 16) {
                data->offset = 0;
            }else {
                data->offset -= 16;
            }
        }
        //down
        if(y+height-16 <= mouseY) {
            Syscall_DrawButton_Pushed(layerId, x+1, y+height-15, 14, 14, "");

            if(height <= page_height) {
                data->offset += 16;
                if(page_height-height <= data->offset) {
                    data->offset = page_height-height;
                }
            }
        }
        //バーのドラッグ
        if(x <= mouseX && mouseX < x+16 && y+16 <= mouseY && mouseY < y+height-16) {
            Task_Message message;
            while(1) {
                Syscall_ReadMessage(&message);

                switch(message.type) {
                    case Task_Message_Quit:
                        Message_EnQueue(Task_GetRunningTaskId(), &message);
                        return -1;
                    case Task_Message_CloseWindow:
                        Message_EnQueue(Task_GetRunningTaskId(), &message);
                        return 0;
                    case Task_Message_MouseLayerEvent:;
                        uintn drawHeight = height - 32;
                        if(height < page_height) {
                            if(page_height == 0) return -1;
                            drawHeight = (height-32)*height/page_height;
                        }
                        if(height-32-drawHeight == 0) return -1;
                        sintn scroll = ((sintn)message.data.MouseLayerEvent.y - (sintn)mouseY)*(sintn)(page_height - height)/(sintn)(height-32-drawHeight);
                        if(scroll < 0) {
                            if(data->offset < (uintn)(-scroll)) {
                                data->offset = 0;
                            }else {
                                data->offset += scroll;
                            }
                        }else {
                            if(height <= page_height) {
                                data->offset += scroll;
                                if(page_height-height <= data->offset) {
                                    data->offset = page_height-height;
                                }
                            }
                        }
                        mouseX = message.data.MouseLayerEvent.x;
                        mouseY = message.data.MouseLayerEvent.y;
                        Syscall_DrawScrollBar(layerId, data);
                        if(!message.data.MouseLayerEvent.leftButton) {
                            return 0;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }else {
        return -1;
    }

    return 0;
}


sintn Syscall_DrawShade(uintn layerId, sintn x, sintn y, uintn width, uintn height, Graphic_Color color) {
    Graphic_FrameBuff framebuff;
    if(Layer_Window_GetFrameBuff(layerId, &framebuff)) return -1;

    Graphic_FrameBuff_DrawShade(framebuff, x, y, width, height, color);
    Layer_Window_NotifyUpdate(layerId, x, y, width, height);
    
    return 0;
}

