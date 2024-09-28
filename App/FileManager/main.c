#include <types.h>
#include "../../Kernel/graphic.h"
#include "../../Kernel/queue.h"
#include "../../Kernel/task.h"
#include "../../Kernel/file.h"
#include "../../Kernel/syscall.h"
#include <app_x64.h>
#include <efi.h>
#include <efi_simple_text_input_protocol.h>

#define DefaultBuffSize (128)

uintn width = 400;
uintn height = 300;
uintn layerId;
ascii path[DefaultBuffSize] = "";

sintn scroll = 0;
struct {
    uintn isExist;
    uintn pages;
    uintn entryCount;
    File_DirectoryEntry* dirEntList;
} dirEntData = {0, 0, 0, NULL};
sintn selectedIndex = -1;

void respondMouse(Task_Message* message);
void respondKeyboard(Task_Message* message);
void load(void);
void flush(void);
uintn mv(const ascii fromRelPath[DefaultBuffSize], const ascii toRelPath[DefaultBuffSize], ascii workingPath[DefaultBuffSize]);
uintn cp(const ascii fromRelPath[DefaultBuffSize], const ascii toRelPath[DefaultBuffSize], ascii workingPath[DefaultBuffSize]);


sintn main(ascii arg[32]) {
    App_Syscall_ExitStdIo();

    if(!(arg == NULL || arg[0] == '\0')) {
        for(uintn i=0; i<32; i++) {
            path[i] = arg[i];
            if(arg[i] == '\0') break;
            if(i == 31) {
                path[0] = '\0';
                break;
            }
        }
    }

    if(App_Syscall_NewWindow(&layerId, 50, 50, width, height, "FileManager")) return 0;

    load();
    flush();

    Task_Message message;
    while(1) {
        App_Syscall_ReadMessage(&message);
        switch(message.type) {
            case Task_Message_Quit:
                App_Syscall_Exit(0);
                break;
            case Task_Message_CloseWindow:
                App_Syscall_Exit(0);
                break;
            case Task_Message_KeyPushed:
                respondKeyboard(&message);
                break;
            case Task_Message_MouseLayerEvent:
                respondMouse(&message);
                break;
            default:
                break;
        }
    }

    return 0;
}


static void sprintint(uintn number, uintn buffsize, ascii buff[]) {
    uintn n;
    for(sintn i=buffsize-2; 0<=i; i--) {
        n = number%10;
        buff[i] = (n+'0');
        
        number /= 10;
    }
    buff[buffsize-1] = '\0';
    
    return;
}

static uintn log10uintn(uintn number) {
    for(uintn i=0; ; i++) {
        number /= 10;
        if(number == 0) return i;
    }
}

//パスの文字列のディレクトリ名の長さを返す
static uintn getAbsPath_getDirNameLength(const ascii* path) {
    if(path == NULL) return 0;

    for(uintn i=0; 1; i++) {
        if(path[i] == '/' || path[i] == '\0') return i;
    }
}


//絶対パスを取得
static uintn getAbsPath(const ascii relPath[DefaultBuffSize], const ascii workingPath[DefaultBuffSize], ascii absPath[DefaultBuffSize]) {
    if(workingPath == NULL || absPath == NULL) return 1;

    uintn absPath_lengthList[DefaultBuffSize];
    sintn absPath_lengthList_index = -1;
    sintn absPath_index = 0;

    uintn workingPath_index = 0;
    while(1) {
        uintn nameLength = getAbsPath_getDirNameLength(workingPath + workingPath_index);
        if(nameLength != 0) {
            if(nameLength == 1 && workingPath[workingPath_index] == '.') {//./
                //do nothing
            }else if(nameLength == 2 && workingPath[workingPath_index] == '.' && workingPath[workingPath_index + 1] == '.') {//../
                if(absPath_lengthList_index < 0) return 2;
                absPath_index -= absPath_lengthList[absPath_lengthList_index];
                absPath_lengthList_index --;
            }else {
                for(uintn i=0; i<nameLength; i++) {
                    if(DefaultBuffSize <= workingPath_index + i || DefaultBuffSize <= absPath_index + i) return 3;
                    absPath[absPath_index + i] = workingPath[workingPath_index + i];
                }
                if(DefaultBuffSize <= absPath_index + nameLength) return 4;
                absPath[absPath_index + nameLength] = '/';
                absPath_index += nameLength + 1;
                absPath_lengthList_index ++;
                if(DefaultBuffSize <= absPath_lengthList_index) return 5;
                absPath_lengthList[absPath_lengthList_index] = nameLength + 1;
            }
        }
        if(DefaultBuffSize <= workingPath_index + nameLength) return 6;
        if(workingPath[workingPath_index + nameLength] == '\0') {
            if(DefaultBuffSize <= absPath_index) return 7;
            break;
        }

        workingPath_index += nameLength + 1;
    }

    uintn relPath_index = 0;
    while(1) {
        uintn nameLength = getAbsPath_getDirNameLength(relPath + relPath_index);
        if(nameLength != 0) {
            if(nameLength == 1 && relPath[relPath_index] == '.') {//./
                //do nothing
            }else if(nameLength == 2 && relPath[relPath_index] == '.' && relPath[relPath_index + 1] == '.') {//../
                if(absPath_lengthList_index < 0) return 2;

                absPath_index -= absPath_lengthList[absPath_lengthList_index];
                absPath_lengthList_index --;
            }else {
                for(uintn i=0; i<nameLength; i++) {
                    if(DefaultBuffSize <= relPath_index + i || DefaultBuffSize <= absPath_index + i) return 3;
                    absPath[absPath_index + i] = relPath[relPath_index + i];
                }
                if(DefaultBuffSize <= absPath_index + nameLength) return 4;
                absPath[absPath_index + nameLength] = '/';
                absPath_index += nameLength + 1;
                absPath_lengthList_index ++;
                if(DefaultBuffSize <= absPath_lengthList_index) return 5;
                absPath_lengthList[absPath_lengthList_index] = nameLength + 1;
            }
        }

        if(DefaultBuffSize <= relPath_index + nameLength) return 6;
        if(relPath[relPath_index + nameLength] == '\0') {
            if(DefaultBuffSize <= absPath_index) return 7;
            if(absPath_index == 0) {
                absPath[0] = '\0';
            }else {
                absPath[absPath_index-1] = '\0';
            }
            break;
        }

        relPath_index += nameLength + 1;
    }

    return 0;
}


void respondKeyboard(Task_Message* message) {
    if(message->type != Task_Message_KeyPushed) return;

    if(message->data.KeyPushed.asciiCode != 0) {
        if(message->data.KeyPushed.asciiCode == 0x08) {
            ascii absPath[DefaultBuffSize];
            if(getAbsPath(
                "..",
                path,
                absPath)) {
                return;
            }
            for(uintn k=0; k<DefaultBuffSize; k++) {
                path[k] = absPath[k];
                if(path[k] == '\0') break;
            }
            load();
        }
        if(message->data.KeyPushed.asciiCode == '\n') {
            if(selectedIndex != -1) {
                if(dirEntData.dirEntList[selectedIndex].type == File_Directory) {
                    ascii absPath[DefaultBuffSize];
                    if(getAbsPath(
                        dirEntData.dirEntList[selectedIndex].name,
                        path,
                        absPath)) {
                        return;
                    }
                    for(uintn k=0; k<DefaultBuffSize; k++) {
                        path[k] = absPath[k];
                        if(path[k] == '\0') break;
                    }
                    load();
                }
            }
        }
    }else {
        switch(message->data.KeyPushed.scanCode) {
            case EFI_SIMPLE_INPUT_SCANCODE_UPARROW:
                if(0 < selectedIndex) selectedIndex --;
                if(selectedIndex < ((sintn)(height)-64-16-32)/32) {
                    scroll = 0;
                }else {
                    scroll = (selectedIndex)*32 - (((sintn)(height)-64-16-32)/32)*32;
                }
                break;
            case EFI_SIMPLE_INPUT_SCANCODE_DOWNARROW:
                if(selectedIndex == -1) {
                    selectedIndex = 0;
                    break;
                }
                if(selectedIndex+1 < (sintn)dirEntData.entryCount) selectedIndex ++;
                if(selectedIndex < ((sintn)(height)-64-16-32)/32) {
                    scroll = 0;
                }else {
                    scroll = (selectedIndex)*32 - (((sintn)(height)-64-16-32)/32)*32;
                }
                break;
            default:
                break;
        }
    }

    flush();

    return;
}


void respondMouse(Task_Message* message) {
    if(message->type != Task_Message_MouseLayerEvent) return;

    if(message->data.MouseLayerEvent.leftButton) {
        //戻るボタン
        if(0 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32
            && 32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 64) {
            App_Syscall_DrawButton_Pushed(layerId, 0+1, 32+1, 32-2, 32-2, "<-");

            ascii absPath[DefaultBuffSize];
            if(getAbsPath(
                "..",
                path,
                absPath)) {
                return;
            }
            for(uintn k=0; k<DefaultBuffSize; k++) {
                path[k] = absPath[k];
                if(path[k] == '\0') break;
            }

            load();
            flush();
        }

        //リロード
        if((sintn)(width-32) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width
            && 32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 64) {
            App_Syscall_DrawButton_Pushed(layerId, width-32+0+1, 32+1, 32-2, 32-2, "");
            load();
            flush();
        }

        //メモ帳で開く
        if((sintn)width-32*2+1 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width-32*2+1+32
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y <= 32) {
            App_Syscall_DrawButton_Pushed(layerId, width-32*2+1, 0+1, 32-2, 32-2, "");
            if(selectedIndex != -1) {
                ascii absPath[DefaultBuffSize];
                if(getAbsPath(dirEntData.dirEntList[selectedIndex].name, path, absPath)) {
                    App_Syscall_Alert("Error", sizeof("Error"));
                }
                if(App_Syscall_RunApp("app/notepad.elf", DefaultBuffSize, absPath)) {
                    App_Syscall_Alert("Execution Failed", sizeof("Execution Failed"));
                }
            }
            flush();
        }

        //実行ボタンApp_Syscall_DrawSquare(layerId, width-32+1, 1, 32-2, 32-2, ui_color);
        if((sintn)(width-32) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            if(selectedIndex == -1) return;
            App_Syscall_DrawButton_Pushed(layerId, width-32+1, 0+1, 32-2, 32-2, "Run");
            ascii absPath[DefaultBuffSize];
            if(getAbsPath(
                dirEntData.dirEntList[selectedIndex].name,
                path,
                absPath)) {
                App_Syscall_Alert("Execution Failed", sizeof("Execution Failed"));
                return;
            }
            if(App_Syscall_RunApp(absPath, DefaultBuffSize, "")) {
                App_Syscall_Alert("Execution Failed", sizeof("Execution Failed"));
                return;
            }
        }

        //パスバーApp_Syscall_DrawSquare(layerId, 32+1, 32+1, width-32*2-2, 32-2, ui_color);
        if(32 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)(width-32)
            && 32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 64) {
            App_Syscall_EditBox(layerId, 32+1, 32+1, 32-2, path, DefaultBuffSize);
            load();
            flush();
        }

        //移動ボタン
        if(0 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            App_Syscall_DrawButton_Pushed(layerId, 0+1, 0+1, 32-2, 32-2, "Mov");
            if(selectedIndex != -1) {
                ascii buff[DefaultBuffSize] = "";
                if(!App_Syscall_Prompt("Input Destination Path", sizeof("Input Destination Path"), buff, DefaultBuffSize)) {
                    if(mv(dirEntData.dirEntList[selectedIndex].name, buff, path)) {
                        App_Syscall_Alert("Move Failed", sizeof("Move Failed"));
                    }
                    load();
                    flush();
                }
            }
        }

        //コピーボタン
        if(32 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 64
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            if(selectedIndex != -1) {
                App_Syscall_DrawButton_Pushed(layerId, 32+1, 0+1, 32-2, 32-2, "Cpy");
                ascii buff[DefaultBuffSize] = "";
                if(!App_Syscall_Prompt("Input Destination Path", sizeof("Input Destination Path"), buff, DefaultBuffSize)) {
                    if(cp(dirEntData.dirEntList[selectedIndex].name, buff, path)) {
                        App_Syscall_Alert("Copy Failed", sizeof("Copy Failed"));
                    }
                    load();
                }
                flush();
            }
        }

        //削除ボタン
        if(64 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 96
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            if(selectedIndex != -1) {
                App_Syscall_DrawButton_Pushed(layerId, 32*2+1, 0+1, 32-2, 32-2, "Del");;
                if(!App_Syscall_Confirm("Will you delete file?", sizeof("Will you delete file?"))) {
                    ascii buff[DefaultBuffSize];
                    getAbsPath(dirEntData.dirEntList[selectedIndex].name, path, buff);
                    if(App_Syscall_RemoveFile(buff, DefaultBuffSize)) {
                        App_Syscall_Alert("Delete Failed", sizeof("Delete Failed"));
                    }
                    load();
                }
            }
        }


        //ディレクトリ作成
        if(96 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 128
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            App_Syscall_DrawButton_Pushed(layerId, 32*3+1, 0+1, 32-2, 32-2, "");
            ascii buff[DefaultBuffSize] = "";
            if(!App_Syscall_Prompt("Input Directory Name", sizeof("Input Directory Name"), buff, DefaultBuffSize)) {
                ascii dirAbsPath[DefaultBuffSize];
                getAbsPath(buff, path, dirAbsPath);
                if(App_Syscall_MkDir(dirAbsPath, DefaultBuffSize)) {
                    App_Syscall_Alert("MkDir Failed", sizeof("MkDir Failed"));
                }
                load();
            }

            flush();
        }


        //テキスト作成
        if(128 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 160
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            App_Syscall_DrawButton_Pushed(layerId, 32*4+1, 0+1, 32-2, 32-2, "");
            ascii buff[DefaultBuffSize] = "";
            if(!App_Syscall_Prompt("Input File Name", sizeof("Input File Name"), buff, DefaultBuffSize)) {
                ascii txtAbsPath[DefaultBuffSize];
                getAbsPath(buff, path, txtAbsPath);
                ascii txtFile[] = "Hello, World!";
                if(App_Syscall_WriteFileFromMem(txtAbsPath, DefaultBuffSize, sizeof(txtFile), txtFile)) {
                    App_Syscall_Alert("MkTxt Failed", sizeof("MkTxt Failed"));
                }
                load();
            }

            flush();
        }


        //upボタン
        if(32*5+16 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32*5+16+32
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 16) {
            App_Syscall_DrawButton_Pushed(layerId, 32*5+16+1, 0+1, 32-2, 16-2, "up");
            if(0 <= selectedIndex) {
                selectedIndex --;
            }
            flush();
        }

        //downボタン
        if(32*5+16 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32*5+16+32
            && 16 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            App_Syscall_DrawButton_Pushed(layerId, 32*5+16+1, 16+1, 32-2, 16-2, "down");;
            if(selectedIndex+1 < (sintn)dirEntData.entryCount) {
                selectedIndex ++;
            }
            flush();
        }

        //ファイルリストに対しての操作
        if(64 <= message->data.MouseLayerEvent.y) {
            for(sintn i=0; i<(sintn)dirEntData.entryCount; i++) {
                if(64+16+i*32-scroll <= (sintn)message->data.MouseLayerEvent.y && (sintn)message->data.MouseLayerEvent.y < 64+16+32+i*32-scroll) {
                    selectedIndex = i;
                    if(dirEntData.dirEntList[i].type == File_Directory && (sintn)width - 100 <= message->data.MouseLayerEvent.x) {
                        ascii absPath[DefaultBuffSize];
                        if(getAbsPath(
                            dirEntData.dirEntList[i].name,
                            path,
                            absPath)) {
                            return;
                        }
                        for(uintn k=0; k<DefaultBuffSize; k++) {
                            path[k] = absPath[k];
                            if(path[k] == '\0') break;
                        }
                        load();
                    }
                    flush();
                    break;
                }
            }
        }
    }

    return;
}


void load(void) {
    App_Syscall_FreePages(dirEntData.pages, dirEntData.dirEntList);
    dirEntData.isExist = 0;
    dirEntData.dirEntList = NULL;
    dirEntData.entryCount = 0;
    dirEntData.pages = 0;
    App_Syscall_GetFileList(path, DefaultBuffSize, &dirEntData.entryCount, NULL);
    dirEntData.pages = (dirEntData.entryCount + 0xfff) >> 12;
    if(App_Syscall_AllocPage(dirEntData.pages, (void**)&dirEntData.dirEntList)) return;

    if(!App_Syscall_GetFileList(path, DefaultBuffSize, &dirEntData.entryCount, dirEntData.dirEntList)) {
        dirEntData.isExist = 1;
    }

    selectedIndex = -1;
    scroll = 0;

    return;
}


void flush(void) {
    Graphic_Color white = {0xff, 0xff, 0xff};
    Graphic_Color black = {0x00, 0x00, 0x00};
    Graphic_Color gray = {0x88, 0x88, 0x88};
    Graphic_Color blue = {0x55, 0x55, 0xff};
    Graphic_Color shudow = {0x4f, 0x4f, 0x4f};
    Graphic_Color light = {0xd0, 0xd0, 0xd0};

    Graphic_Color ui_color = {0xf0, 0xf0, 0xf0};

    App_Syscall_DrawSquare(layerId, 0, 64, width, height-64, white);

    if(dirEntData.isExist) {
        for(sintn i=0; i<(sintn)dirEntData.entryCount; i++) {
            if(i == selectedIndex && 64 <= 64+16+i*32-scroll) {
                App_Syscall_DrawSquare(layerId, 0, 64+16+i*32-scroll, width, 32, blue);
            }

            for(uintn k=0; k<32; k++) {
                if(dirEntData.dirEntList[i].name[k] == '\0') break;
                App_Syscall_DrawFont(layerId, 4+k*8, 64+16+10+i*32-scroll, dirEntData.dirEntList[i].name[k], black);
            }

            if(dirEntData.dirEntList[i].type != File_Directory) {
                if(dirEntData.dirEntList[i].size != 0) {
                    uintn log10uintn_size = log10uintn(dirEntData.dirEntList[i].size);
                    ascii strbuff[log10uintn_size+2];
                    sprintint(dirEntData.dirEntList[i].size, log10uintn_size+2, strbuff);
                    for(uintn k=0; k<log10uintn_size+1; k++) {
                        App_Syscall_DrawFont(layerId, width-32-8*(log10uintn_size-k), 64+16+10+i*32-scroll, strbuff[k], black);
                    }
                }
            }

            if(dirEntData.dirEntList[i].type == File_Directory) {
                App_Syscall_DrawFont(layerId, width-12, 64+16+10+i*32-scroll, '>', black);
            }

            if(0 <= 64+16+32+i*32-scroll-1) App_Syscall_DrawSquare(layerId, 0, 64+16+32+i*32-scroll-1, width, 1, gray);
        }
    }else {
        ascii str[] = "Path Not Found.";
        for(uintn i=0; i<sizeof(str); i++) {
            if(str[i] == '\0') break;
            App_Syscall_DrawFont(layerId, i*8, 64+16, str[i], black);
        }
    }

    App_Syscall_DrawSquare(layerId, 0, 0, width, 64, gray);

    //パスバー
    App_Syscall_DrawSquare(layerId, 32+1, 32+1, width-32*2-2, 32-2, white);
    App_Syscall_DrawSquare(layerId, 32+1, 32+1, width-32*2-2, 1, shudow);
    App_Syscall_DrawSquare(layerId, 32+1, 32+32-2, width-32*2-2, 1, light);
    App_Syscall_DrawSquare(layerId, 32+1, 32+1, 1, 32-2, shudow);
    App_Syscall_DrawSquare(layerId, width-32-2, 32+1, 1, 32-2, light);

    if(path[0] == '\0') {
        App_Syscall_DrawFont(layerId, 32+8, 32+10, '/', black);
    }else {
        for(uintn i=0; i<DefaultBuffSize; i++) {
            if(path[i] == '\0') break;
            App_Syscall_DrawFont(layerId, 32+8*(i+1), 32+10, path[i], black);
        }
    }

    //戻るボタン
    App_Syscall_DrawButton(layerId, 0+1, 32+1, 32-2, 32-2, "<-");

    //リロードボタン
    App_Syscall_DrawButton(layerId, width-32+0+1, 32+1, 32-2, 32-2, "");
    App_Syscall_DrawFont(layerId, width-32+8, 32+2, 'R', black);
    App_Syscall_DrawFont(layerId, width-32+8+8, 32+2, 'e', black);
    App_Syscall_DrawFont(layerId, width-32+0, 32+18, 'l', black);
    App_Syscall_DrawFont(layerId, width-32+8, 32+18, 'o', black);
    App_Syscall_DrawFont(layerId, width-32+8*2, 32+18, 'a', black);
    App_Syscall_DrawFont(layerId, width-32+8*3, 32+18, 'd', black);
    if(selectedIndex != -1) {
        //移動ボタン
        App_Syscall_DrawButton(layerId, 0+1, 0+1, 32-2, 32-2, "Mov");

        //コピーボタン
        App_Syscall_DrawButton(layerId, 32+1, 0+1, 32-2, 32-2, "Cpy");

        //削除ボタン
        App_Syscall_DrawButton(layerId, 32*2+1, 0+1, 32-2, 32-2, "Del");
    }else {
        //移動ボタン
        App_Syscall_DrawSquare_NotActive(layerId, 0+1, 0+1, 32-2, 32-2, "Mov");

        //コピーボタン
        App_Syscall_DrawSquare_NotActive(layerId, 32+1, 0+1, 32-2, 32-2, "Cpy");

        //削除ボタン
        App_Syscall_DrawSquare_NotActive(layerId, 32*2+1, 0+1, 32-2, 32-2, "Del");
    }

    //ディレクトリ作成
    App_Syscall_DrawButton(layerId, 32*3+1, 0+1, 32-2, 32-2, "");
    App_Syscall_DrawFont(layerId, 32*3+8, 0, 'M', black);
    App_Syscall_DrawFont(layerId, 32*3+16, 0, 'k', black);
    App_Syscall_DrawFont(layerId, 32*3+4, 16, 'D', black);
    App_Syscall_DrawFont(layerId, 32*3+12, 16, 'i', black);
    App_Syscall_DrawFont(layerId, 32*3+20, 16, 'r', black);

    //テキストファイル作成
    App_Syscall_DrawButton(layerId, 32*4+1, 0+1, 32-2, 32-2, "");
    App_Syscall_DrawFont(layerId, 32*4+8, 0, 'M', black);
    App_Syscall_DrawFont(layerId, 32*4+16, 0, 'k', black);
    App_Syscall_DrawFont(layerId, 32*4+4, 16, 'T', black);
    App_Syscall_DrawFont(layerId, 32*4+12, 16, 'x', black);
    App_Syscall_DrawFont(layerId, 32*4+20, 16, 't', black);

    //上スクロールボタン
    App_Syscall_DrawButton(layerId, 32*5+16+1, 0+1, 32-2, 16-2, "up");

    //下スクロールボタン
    App_Syscall_DrawButton(layerId, 32*5+16+1, 16+1, 32-2, 16-2, "down");

    //OpenWith...
    App_Syscall_DrawFont(layerId, width-32*3, 0, 'O', black);
    App_Syscall_DrawFont(layerId, width-32*3+8, 0, 'p', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*2, 0, 'e', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*3, 0, 'n', black);
    App_Syscall_DrawFont(layerId, width-32*3, 16, 'W', black);
    App_Syscall_DrawFont(layerId, width-32*3+8, 16, 'i', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*2, 16, 't', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*3, 16, 'h', black);

    if(selectedIndex != -1) {
        //実行ボタン
        App_Syscall_DrawButton(layerId, width-32+1, 0+1, 32-2, 32-2, "Run");

        //メモ帳で開くボタン
        App_Syscall_DrawButton(layerId, width-32*2+1, 0+1, 32-2, 32-2, "");
        App_Syscall_DrawFont(layerId, width-32*2, 0, 'N', black);
        App_Syscall_DrawFont(layerId, width-32*2+8, 0, 'o', black);
        App_Syscall_DrawFont(layerId, width-32*2+8*2, 0, 't', black);
        App_Syscall_DrawFont(layerId, width-32*2+8*3, 0, 'e', black);
        App_Syscall_DrawFont(layerId, width-32*2+4, 16, 'p', black);
        App_Syscall_DrawFont(layerId, width-32*2+4+8, 16, 'a', black);
        App_Syscall_DrawFont(layerId, width-32*2+4+8*2, 16, 'd', black);
    }else {
        //実行ボタン
        App_Syscall_DrawSquare_NotActive(layerId, width-32+1, 0+1, 32-2, 32-2, "Run");

        //メモ帳で開くボタン
        App_Syscall_DrawSquare_NotActive(layerId, width-32*2+1, 0+1, 32-2, 32-2, "");
        App_Syscall_DrawFont(layerId, width-32*2, 0, 'N', gray);
        App_Syscall_DrawFont(layerId, width-32*2+8, 0, 'o', gray);
        App_Syscall_DrawFont(layerId, width-32*2+8*2, 0, 't', gray);
        App_Syscall_DrawFont(layerId, width-32*2+8*3, 0, 'e', gray);
        App_Syscall_DrawFont(layerId, width-32*2+4, 16, 'p', gray);
        App_Syscall_DrawFont(layerId, width-32*2+4+8, 16, 'a', gray);
        App_Syscall_DrawFont(layerId, width-32*2+4+8*2, 16, 'd', gray);
    }


    //ディレクトリを表示
    App_Syscall_DrawSquare(layerId, 0, 64, width, 16, ui_color);
    App_Syscall_DrawSquare(layerId, 0, 64+16-1, width, 1, gray);
    App_Syscall_DrawFont(layerId, 2, 64, 'n', gray);
    App_Syscall_DrawFont(layerId, 2+8, 64, 'a', gray);
    App_Syscall_DrawFont(layerId, 2+8*2, 64, 'm', gray);
    App_Syscall_DrawFont(layerId, 2+8*3, 64, 'e', gray);

    App_Syscall_DrawFont(layerId, width-64, 64, 's', gray);
    App_Syscall_DrawFont(layerId, width-64+8, 64, 'i', gray);
    App_Syscall_DrawFont(layerId, width-64+8*2, 64, 'z', gray);
    App_Syscall_DrawFont(layerId, width-64+8*3, 64, 'e', gray);

    App_Syscall_DrawFont(layerId, width-24, 64, 'd', gray);
    App_Syscall_DrawFont(layerId, width-24+8, 64, 'i', gray);
    App_Syscall_DrawFont(layerId, width-24+8*2, 64, 'r', gray);

    return;
}


uintn mv(const ascii fromRelPath[DefaultBuffSize], const ascii toRelPath[DefaultBuffSize], ascii workingPath[DefaultBuffSize]) {
    uintn status;

    //絶対パスの取得
    ascii fromAbsPath[DefaultBuffSize];
    status = getAbsPath(fromRelPath, workingPath, fromAbsPath);
    if(status) {
        return 1;
    }

    ascii toAbsPath[DefaultBuffSize];
    status = getAbsPath(toRelPath, workingPath, toAbsPath);
    if(status) {
        return 2;
    }

    //ファイルサイズの取得
    File_DirectoryEntry dirEntBuff;
    status = App_Syscall_GetDirEntryByPath(fromAbsPath, DefaultBuffSize, &dirEntBuff);
    if(status) {
        return 3;
    }

    //ファイルバッファへ書き込み
    uint8 filebuff[dirEntBuff.size];
    status = App_Syscall_MMapFile(fromAbsPath, DefaultBuffSize, sizeof(filebuff), filebuff);
    if(status) {
        return 4;
    }

    //ファイルの保存
    status = App_Syscall_WriteFileFromMem(toAbsPath, DefaultBuffSize, dirEntBuff.size, filebuff);
    if(status) {
        return 5;
    }

    //元のファイルの消去
    status = App_Syscall_RemoveFile(fromAbsPath, DefaultBuffSize);
    if(status) {
        return 6;
    }

    return 0;
}


uintn cp(const ascii fromRelPath[DefaultBuffSize], const ascii toRelPath[DefaultBuffSize], ascii workingPath[DefaultBuffSize]) {
    uintn status;

    //絶対パスの取得
    ascii fromAbsPath[DefaultBuffSize];
    status = getAbsPath(fromRelPath, workingPath, fromAbsPath);
    if(status) {
        return 1;
    }

    ascii toAbsPath[DefaultBuffSize];
    status = getAbsPath(toRelPath, workingPath, toAbsPath);
    if(status) {
        return 2;
    }

    //ファイルサイズの取得
    File_DirectoryEntry dirEntBuff;
    status = App_Syscall_GetDirEntryByPath(fromAbsPath, DefaultBuffSize, &dirEntBuff);
    if(status) {
        return 3;
    }

    //ファイルバッファへ書き込み
    uint8 filebuff[dirEntBuff.size];
    status = App_Syscall_MMapFile(fromAbsPath, DefaultBuffSize, sizeof(filebuff), filebuff);
    if(status) {
        return 4;
    }

    //ファイルの保存
    status = App_Syscall_WriteFileFromMem(toAbsPath, DefaultBuffSize, dirEntBuff.size, filebuff);
    if(status) {
        return 5;
    }

    return 0;
}


