#include <types.h>
#include "../../Kernel/graphic.h"
#include "../../Kernel/queue.h"
#include "../../Kernel/task.h"
#include "../../Kernel/file.h"
#include <app_x64.h>

#define DefaultBuffSize (128)

uintn width = 400;
uintn height = 300;
uintn layerId;
ascii path[DefaultBuffSize] = "";

sintn scroll = 0;
struct {
    uintn pages;
    uintn entryCount;
    File_DirectoryEntry* dirEntList;
} dirEntData = {0, 0, NULL};
sintn selectedIndex = -1;

void respondMouse(Task_Message* message);
void load(void);
void flush(void);



sintn main(ascii arg[32]) {
    App_Syscall_StdOut("ABCDE", 6);
    //App_Syscall_ExitStdIo();


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




void respondMouse(Task_Message* message) {
    if(message->type != Task_Message_MouseLayerEvent) return;
    if(message->data.MouseLayerEvent.relativeScroll != 0) {
        if(64 <= message->data.MouseLayerEvent.y) {
            scroll += message->data.MouseLayerEvent.relativeScroll;
            flush();
        }
    }

    if(message->data.MouseLayerEvent.leftButton) {
        //戻るボタン
        if(0 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32
            && 32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 64) {
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
            load();
            flush();
        }

        //実行ボタンApp_Syscall_DrawSquare(layerId, width-32+1, 1, 32-2, 32-2, ui_color);
        if((sintn)(width-32) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            if(selectedIndex == -1) return;
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

        //ファイルリストに対しての操作
        if(64 <= message->data.MouseLayerEvent.y) {
            for(sintn i=0; i<(sintn)dirEntData.entryCount; i++) {
                if(64+16+i*32-scroll <= (sintn)message->data.MouseLayerEvent.y && (sintn)message->data.MouseLayerEvent.y < 64+16+32+i*32-scroll) {
                    selectedIndex = i;
                    if(dirEntData.dirEntList[i].type == File_Directory) {
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
    dirEntData.dirEntList = NULL;
    dirEntData.entryCount = 0;
    dirEntData.pages = 0;
    App_Syscall_GetFileList(path, DefaultBuffSize, &dirEntData.entryCount, NULL);
    dirEntData.pages = (dirEntData.entryCount + 0xfff) >> 12;
    if(App_Syscall_AllocPage(dirEntData.pages, (void**)&dirEntData.dirEntList)) return;

    App_Syscall_GetFileList(path, DefaultBuffSize, &dirEntData.entryCount, dirEntData.dirEntList);

    selectedIndex = -1;

    return;
}


void flush(void) {
    Graphic_Color white = {0xff, 0xff, 0xff};
    Graphic_Color black = {0x00, 0x00, 0x00};
    Graphic_Color gray = {0x88, 0x88, 0x88};
    Graphic_Color blue = {0x55, 0x55, 0xff};

    Graphic_Color ui_color = {0xf0, 0xf0, 0xf0};

    App_Syscall_DrawSquare(layerId, 0, 0, width, 64, gray);
    App_Syscall_DrawSquare(layerId, 0, 64, width, height-64, white);

    //パスバー
    App_Syscall_DrawSquare(layerId, 32+1, 32+1, width-32*2-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, 32+8, 32+10, 'p', black);
    App_Syscall_DrawFont(layerId, 32+8+8, 32+10, 'a', black);
    App_Syscall_DrawFont(layerId, 32+8+8*2, 32+10, 't', black);
    App_Syscall_DrawFont(layerId, 32+8+8*3, 32+10, 'h', black);
    App_Syscall_DrawFont(layerId, 32+8+8*4, 32+10, ':', black);

    if(path[0] == '\0') {
        App_Syscall_DrawFont(layerId, 32+8*6, 32+10, '/', black);
    }else {
        for(uintn i=0; i<DefaultBuffSize; i++) {
            if(path[i] == '\0') break;
            App_Syscall_DrawFont(layerId, 32+8+8*(i+5), 32+10, path[i], black);
        }
    }

    //戻るボタン
    App_Syscall_DrawSquare(layerId, 0+1, 32+1, 32-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, 8, 32+10, '<', black);
    App_Syscall_DrawFont(layerId, 8*2, 32+10, '-', black);

    //リロードボタン
    App_Syscall_DrawSquare(layerId, width-32+0+1, 32+1, 32-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, width-32+8, 32+2, 'R', black);
    App_Syscall_DrawFont(layerId, width-32+8+8, 32+2, 'e', black);
    App_Syscall_DrawFont(layerId, width-32+0, 32+18, 'l', black);
    App_Syscall_DrawFont(layerId, width-32+8, 32+18, 'o', black);
    App_Syscall_DrawFont(layerId, width-32+8*2, 32+18, 'a', black);
    App_Syscall_DrawFont(layerId, width-32+8*3, 32+18, 'd', black);

    //OpenWith...
    App_Syscall_DrawFont(layerId, width-32*3, 0, 'O', black);
    App_Syscall_DrawFont(layerId, width-32*3+8, 0, 'p', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*2, 0, 'e', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*3, 0, 'n', black);
    App_Syscall_DrawFont(layerId, width-32*3, 16, 'W', black);
    App_Syscall_DrawFont(layerId, width-32*3+8, 16, 'i', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*2, 16, 't', black);
    App_Syscall_DrawFont(layerId, width-32*3+8*3, 16, 'h', black);


    //実行ボタン
    App_Syscall_DrawSquare(layerId, width-32+1, 1, 32-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, width-32+4, 10, 'R', black);
    App_Syscall_DrawFont(layerId, width-32+4+8, 10, 'u', black);
    App_Syscall_DrawFont(layerId, width-32+4+8*2, 10, 'n', black);

    //メモ帳で開くボタン
    App_Syscall_DrawSquare(layerId, width-32*2+1, 1, 32-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, width-32*2, 0, 'N', black);
    App_Syscall_DrawFont(layerId, width-32*2+8, 0, 'o', black);
    App_Syscall_DrawFont(layerId, width-32*2+8*2, 0, 't', black);
    App_Syscall_DrawFont(layerId, width-32*2+8*3, 0, 'e', black);
    App_Syscall_DrawFont(layerId, width-32*2+4, 16, 'p', black);
    App_Syscall_DrawFont(layerId, width-32*2+4+8, 16, 'a', black);
    App_Syscall_DrawFont(layerId, width-32*2+4+8*2, 16, 'd', black);

    //移動ボタン
    App_Syscall_DrawSquare(layerId, 1, 1, 32-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, 4, 10, 'M', black);
    App_Syscall_DrawFont(layerId, 4+8, 10, 'o', black);
    App_Syscall_DrawFont(layerId, 4+8*2, 10, 'v', black);

    //コピーボタン
    App_Syscall_DrawSquare(layerId, 32+1, 1, 32-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, 32+4, 10, 'C', black);
    App_Syscall_DrawFont(layerId, 32+4+8, 10, 'p', black);
    App_Syscall_DrawFont(layerId, 32+4+8*2, 10, 'y', black);

    //削除ボタン
    App_Syscall_DrawSquare(layerId, 32*2+1, 1, 32-2, 32-2, ui_color);
    App_Syscall_DrawFont(layerId, 32*2+4, 10, 'D', black);
    App_Syscall_DrawFont(layerId, 32*2+4+8, 10, 'e', black);
    App_Syscall_DrawFont(layerId, 32*2+4+8*2, 10, 'l', black);


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

    for(sintn i=0; i<(sintn)dirEntData.entryCount; i++) {
        if(i == selectedIndex) {
            App_Syscall_DrawSquare(layerId, 0, 64+16+i*32-scroll, width, 32, blue);
        }

        for(uintn k=0; k<32; k++) {
            if(dirEntData.dirEntList[i].name[k] == '\0') break;
            App_Syscall_DrawFont(layerId, 4+k*8, 64+16+10+i*32-scroll, dirEntData.dirEntList[i].name[k], black);
        }

        if(dirEntData.dirEntList[i].size == 0) {
            App_Syscall_DrawFont(layerId, width-32-8, 64+16+10+i*32-scroll, '0', black);
        }else {
            uintn log10uintn_size = log10uintn(dirEntData.dirEntList[i].size);
            ascii strbuff[log10uintn_size+2];
            sprintint(dirEntData.dirEntList[i].size, log10uintn_size+2, strbuff);
            for(uintn k=0; k<log10uintn_size+1; k++) {
                App_Syscall_DrawFont(layerId, width-32-8*(log10uintn_size-k), 64+16+10+i*32-scroll, strbuff[k], black);
            }
        }

        if(dirEntData.dirEntList[i].type == File_Directory) {
            App_Syscall_DrawFont(layerId, width-12, 64+16+10+i*32-scroll, '>', black);
        }


        App_Syscall_DrawSquare(layerId, 0, 64+16+32+i*32-scroll-1, width, 1, gray);
    }

/*
    //スクロールを表示
    if(height-64 <= )
*/
    return;
}
