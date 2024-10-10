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

const uintn width = 400;
const uintn height = 300;
uintn layerId;

App_Syscall_Scrollbar_Data scroll = {width-16, 64+16, height-64-16, 0, height-64-16};

Graphic_Color white = {0xff, 0xff, 0xff};
Graphic_Color black = {0x00, 0x00, 0x00};
Graphic_Color gray = {0x88, 0x88, 0x88};
Graphic_Color blue = {0x55, 0x55, 0xff};
Graphic_Color shudow = {0x4f, 0x4f, 0x4f};
Graphic_Color light = {0xd0, 0xd0, 0xd0};

Graphic_Color ui_color = {0xf0, 0xf0, 0xf0};

ascii pathBar_buff[DefaultBuffSize] = "";
App_Syscall_EditBox_Data pathBar = {32+1, 32+1, width-32*2-2, 32-2,
                                    NULL, DefaultBuffSize,
                                    0, 1, 0, 0,
                                    0, 1, 1};


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
static void sprintint(uintn number, uintn buffsize, ascii buff[]);



sintn main(ascii arg[32]) {
    //App_Syscall_ExitStdIo();
    pathBar.buff = pathBar_buff;

    pathBar.buff[0] = '/';
    pathBar.buff[1] = '\0';

    if(!(arg == NULL || arg[0] == '\0')) {
        for(uintn i=0; i<32; i++) {
            pathBar.buff[i] = arg[i];
            if(arg[i] == '\0') break;
            if(i == 31) {
                pathBar.buff[0] = '\0';
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


static void movePrecDir(void) {
    ascii absPath[DefaultBuffSize];
    if(getAbsPath(
        "..",
        pathBar.buff,
        absPath)) {
        return;
    }
    pathBar.buff[0] = '/';
    for(uintn k=0; k<DefaultBuffSize; k++) {
        pathBar.buff[k+1] = absPath[k];
        if(pathBar.buff[k+1] == '\0') break;
    }
    load();
    return;
}


static void moveToDir(ascii name[DefaultBuffSize]) {
    ascii absPath[DefaultBuffSize];
    if(getAbsPath(
        name,
        pathBar.buff,
        absPath)) {
        return;
    }
    pathBar.buff[0] = '/';
    for(uintn k=0; k<DefaultBuffSize; k++) {
        pathBar.buff[k+1] = absPath[k];
        if(pathBar.buff[k+1] == '\0') break;
    }
    load();
    return;
}


void respondKeyboard(Task_Message* message) {
    if(message->type != Task_Message_KeyPushed) return;

    if(message->data.KeyPushed.asciiCode != 0) {
        if(message->data.KeyPushed.asciiCode == 0x08) {
            movePrecDir();
        }
        if(message->data.KeyPushed.asciiCode == '\n') {
            if(selectedIndex != -1) {
                if(dirEntData.dirEntList[selectedIndex].type == File_Directory) {
                    moveToDir(dirEntData.dirEntList[selectedIndex].name);
                }
            }
        }
    }else {
        switch(message->data.KeyPushed.scanCode) {
            case EFI_SIMPLE_INPUT_SCANCODE_UPARROW:
                if(0 < selectedIndex) selectedIndex --;
                if(selectedIndex < ((sintn)(height)-64-16-32)/32) {
                    scroll.offset = 0;
                }else {
                    scroll.offset = (selectedIndex)*32 - (((sintn)(height)-64-16-32)/32)*32;
                }
                break;
            case EFI_SIMPLE_INPUT_SCANCODE_DOWNARROW:
                if(selectedIndex == -1) {
                    selectedIndex = 0;
                    break;
                }
                if(selectedIndex+1 < (sintn)dirEntData.entryCount) selectedIndex ++;
                if(selectedIndex < ((sintn)(height)-64-16-32)/32) {
                    scroll.offset = 0;
                }else {
                    scroll.offset = (selectedIndex)*32 - (((sintn)(height)-64-16-32)/32)*32;
                }
                break;
            default:
                break;
        }
    }

    flush();

    return;
}


static uintn deleteFile_R(ascii path[DefaultBuffSize], uintn depth) {
    if(depth == 0) {
        return 1;
    }
    File_DirectoryEntry dirent;
    if(App_Syscall_GetDirEntryByPath(path, DefaultBuffSize, &dirent)) {
        return 1;
    }

    if(dirent.type == File_Directory) {
        uintn buffCount = 0;
        App_Syscall_GetFileList(path, DefaultBuffSize, &buffCount, NULL);
        File_DirectoryEntry buff[buffCount];
        if(App_Syscall_GetFileList(path, DefaultBuffSize, &buffCount, buff)) {
            return 2;
        }

        for(uintn i=0; i<buffCount; i++) {
            ascii absPath[DefaultBuffSize];
            if(getAbsPath(buff[i].name, path, absPath)) {
                return 3;
            }
            if(deleteFile_R(absPath, depth-1)) {
                return 4;
            }
        }
    }
    if(App_Syscall_RemoveFile(path, DefaultBuffSize)) {
        return 5;
    }

    return 0;
}

static uintn movAndCopyFile_R(ascii path[DefaultBuffSize], ascii dest[DefaultBuffSize], uintn depth, uint8 deleteflag) {
    if(depth == 0) {
        return 1;
    }
    File_DirectoryEntry dirent;
    if(App_Syscall_GetDirEntryByPath(path, DefaultBuffSize, &dirent)) {
        return 2;
    }

    if(dirent.type == File_Directory) {
        App_Syscall_MkDir(dest, DefaultBuffSize);

        uintn buffCount = 0;
        App_Syscall_GetFileList(path, DefaultBuffSize, &buffCount, NULL);
        File_DirectoryEntry buff[buffCount];
        if(App_Syscall_GetFileList(path, DefaultBuffSize, &buffCount, buff)) {
            return 2;
        }

        for(uintn i=0; i<buffCount; i++) {
            ascii absPath[DefaultBuffSize];
            if(getAbsPath(buff[i].name, path, absPath)) {
                return 3;
            }
            ascii absPath_dest[DefaultBuffSize];
            if(getAbsPath(buff[i].name, dest, absPath_dest)) {
                return 4;
            }

            if(movAndCopyFile_R(absPath, absPath_dest, depth-1, deleteflag)) {
                return 5;
            }
        }
    }else {
        uint8* filebuff = NULL;
        App_Syscall_AllocPage((dirent.size + 0xfff) >> 12, (void**)&filebuff);
        if(filebuff == NULL) {
            return 6;
        }

        if(App_Syscall_MMapFile(path, DefaultBuffSize, dirent.size, filebuff)) {
            return 7;
        }
        if(App_Syscall_WriteFileFromMem(dest, DefaultBuffSize, dirent.size, filebuff)) {
            return 8;
        }

        App_Syscall_FreePages((dirent.size + 0xfff) >> 12, filebuff);
    }
    if(deleteflag) {
        App_Syscall_RemoveFile(path, DefaultBuffSize);
    }

    return 0;
}


void respondMouse(Task_Message* message) {
    if(message->type != Task_Message_MouseLayerEvent) return;

    if(message->data.MouseLayerEvent.leftButton) {
        //戻るボタン
        if(0 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 32
            && 32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 64) {
            App_Syscall_DrawButton_Pushed(layerId, 0+1, 32+1, 32-2, 32-2, "<-");

            movePrecDir();
            flush();
        }

        //リロード
        if((sintn)(width-32) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width
            && 32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 64) {
            App_Syscall_DrawButton_Pushed(layerId, width-32+0+1, 32+1, 32-2, 32-2, "");

            load();
            flush();
        }

        //パスバーApp_Syscall_DrawSquare(layerId, 32+1, 32+1, width-32*2-2, 32-2, ui_color);
        if(32 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)(width-32)
            && 32 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 64) {
            App_Syscall_EditBox_Response(layerId, message->data.MouseLayerEvent.x, message->data.MouseLayerEvent.y, &pathBar);

            load();
            flush();
        }

        //ディレクトリ作成
        if(1 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 96-1
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            App_Syscall_DrawButton_Pushed(layerId, 1, 0+1, 96-2, 32-2, "NewDirectory");
            ascii buff[DefaultBuffSize] = "";
            if(!App_Syscall_Prompt("Input Directory Name", sizeof("Input Directory Name"), buff, DefaultBuffSize)) {
                ascii dirAbsPath[DefaultBuffSize];
                getAbsPath(buff, pathBar.buff, dirAbsPath);
                if(App_Syscall_MkDir(dirAbsPath, DefaultBuffSize)) {
                    App_Syscall_Alert("MkDir Failed", sizeof("MkDir Failed"));
                }
                load();
            }

            flush();
        }

        //テキスト作成
        if(96+1 <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < 96+64-2
            && 0 <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < 32) {
            App_Syscall_DrawButton_Pushed(layerId, 96+1, 0+1, 64-2, 32-2, "NewText");
            ascii buff[DefaultBuffSize] = "";
            if(!App_Syscall_Prompt("Input File Name", sizeof("Input File Name"), buff, DefaultBuffSize)) {
                ascii txtAbsPath[DefaultBuffSize];
                getAbsPath(buff, pathBar.buff, txtAbsPath);
                ascii txtFile[] = "Hello, World!";
                if(App_Syscall_WriteFileFromMem(txtAbsPath, DefaultBuffSize, sizeof(txtFile), txtFile)) {
                    App_Syscall_Alert("MkTxt Failed", sizeof("MkTxt Failed"));
                }
                load();
            }

            flush();
        }

        //スクロールバー
        if((sintn)(width-16) <= message->data.MouseLayerEvent.x && message->data.MouseLayerEvent.x < (sintn)width
            && (sintn)(64+16) <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < (sintn)height) {
            App_Syscall_DrawShade(layerId, 0, 64+16, width-16, height-16-64, black);
            App_Syscall_DrawScrollBar_Response(layerId, &scroll, message->data.MouseLayerEvent.x, message->data.MouseLayerEvent.y);
            flush();
        }

        //ファイルリストに対しての操作
        if(64 <= message->data.MouseLayerEvent.y && (sintn)message->data.MouseLayerEvent.x < (sintn)(width-16)) {
            for(sintn i=0; i<(sintn)dirEntData.entryCount; i++) {
                if((sintn)(64+16+i*32-scroll.offset) <= message->data.MouseLayerEvent.y && message->data.MouseLayerEvent.y < (sintn)(64+16+32+i*32-scroll.offset)) {
                    selectedIndex = i;
                    
                    uintn mouseX = message->data.MouseLayerEvent.x;

                    //ファイル削除
                    if(width-32*2-16+1 <= mouseX && mouseX < width-32*2-16+1+30) {
                        App_Syscall_DrawButton_Pushed(layerId, width-32*2-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Del");

                        if(!App_Syscall_Confirm("Will you delete file?", sizeof("Will you delete file?"))) {
                            ascii buff[DefaultBuffSize];
                            getAbsPath(dirEntData.dirEntList[i].name, pathBar.buff, buff);
                            if(deleteFile_R(buff, 3)) {
                                App_Syscall_Alert("Delete Failed", sizeof("Delete Failed"));
                            }
                            load();
                        }
                    }

                    //ファイル移動
                    if(width-32*3-16+1 <= mouseX && mouseX < width-32*3-16+1+30) {
                        App_Syscall_DrawButton_Pushed(layerId, width-32*3-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Mov");
                        ascii absPath[DefaultBuffSize];
                        ascii dest_rel[DefaultBuffSize] = "";
                        ascii dest[DefaultBuffSize] = "";

                        if(getAbsPath(dirEntData.dirEntList[i].name, pathBar.buff, absPath)) {
                            App_Syscall_Alert("Error", sizeof("Error"));
                        }else if(App_Syscall_Prompt("Destination Path", sizeof("Destination Path"), dest_rel, DefaultBuffSize)) {
                            App_Syscall_Alert("Invalid Path", sizeof("Invalid Path"));
                        }else if(getAbsPath(dest_rel, pathBar.buff, dest)) {
                            App_Syscall_Alert("Invalid Path", sizeof("Invalid Path"));
                        }else if(movAndCopyFile_R(absPath, dest, 3, 1)) {
                            App_Syscall_Alert("Error", sizeof("Error"));
                        }
                        load();
                    }

                    //ファイルコピー
                    if(width-32*4-16+1 <= mouseX && mouseX < width-32*4-16+1+30) {
                        App_Syscall_DrawButton_Pushed(layerId, width-32*4-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Cpy");
                        ascii absPath[DefaultBuffSize];
                        ascii dest_rel[DefaultBuffSize] = "";
                        ascii dest[DefaultBuffSize] = "";

                        if(getAbsPath(dirEntData.dirEntList[i].name, pathBar.buff, absPath)) {
                            App_Syscall_Alert("Error", sizeof("Error"));
                        }else if(App_Syscall_Prompt("Destination Path", sizeof("Destination Path"), dest_rel, DefaultBuffSize)) {
                            App_Syscall_Alert("Invalid Path", sizeof("Invalid Path"));
                        }else if(getAbsPath(dest_rel, pathBar.buff, dest)) {
                            App_Syscall_Alert("Invalid Path", sizeof("Invalid Path"));
                        }else if(movAndCopyFile_R(absPath, dest, 3, 0)) {
                            App_Syscall_Alert("Error", sizeof("Error"));
                        }
                        load();
                    }
                    
                    if(dirEntData.dirEntList[i].type == File_Directory) {
                        //ディレクトリを開く
                        if((sintn)width - 32-16 <= message->data.MouseLayerEvent.x) {
                            App_Syscall_DrawButton_Pushed(layerId, width-32-16+1, 64+16+i*32-scroll.offset+1, 30, 30, ">");

                            moveToDir(dirEntData.dirEntList[i].name);
                        }
                    }else {
                        //ファイル操作

                        //ファイル開く
                        if(width-32-16+1 <= mouseX && mouseX < width-32-16+1+30) {
                            App_Syscall_DrawButton_Pushed(layerId, width-32-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Open");
                            ascii absPath[DefaultBuffSize];
                            if(getAbsPath(dirEntData.dirEntList[i].name, pathBar.buff, absPath)) {
                                App_Syscall_Alert("Error", sizeof("Error"));
                            }
                            if(App_Syscall_RunApp("app/notepad.elf", DefaultBuffSize, absPath)) {
                                App_Syscall_Alert("Execution Failed", sizeof("Execution Failed"));
                            }
                        }

                        //ファイル実行
                        if(width-32*5-16+1 <= mouseX && mouseX < width-32*5-16+1+30) {
                            App_Syscall_DrawButton_Pushed(layerId, width-32*5-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Run");
                            ascii absPath[DefaultBuffSize];
                            if(getAbsPath(dirEntData.dirEntList[i].name, pathBar.buff, absPath)) {
                                App_Syscall_Alert("Error", sizeof("Error"));
                            }
                            if(App_Syscall_RunApp(absPath, DefaultBuffSize, "")) {
                                App_Syscall_Alert("Execution Failed", sizeof("Execution Failed"));
                            }
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
    if(pathBar.buff[0] == '\0') {
        pathBar.buff[0] = '/';
        pathBar.buff[1] = '\0';
    }

    App_Syscall_FreePages(dirEntData.pages, dirEntData.dirEntList);
    dirEntData.isExist = 0;
    dirEntData.dirEntList = NULL;
    dirEntData.entryCount = 0;
    dirEntData.pages = 0;

    App_Syscall_GetFileList(pathBar.buff, DefaultBuffSize, &dirEntData.entryCount, NULL);
    dirEntData.pages = (dirEntData.entryCount*sizeof(File_DirectoryEntry) + 0xfff) >> 12;
    if(App_Syscall_AllocPage(dirEntData.pages, (void**)&dirEntData.dirEntList)) return;

    if(!App_Syscall_GetFileList(pathBar.buff, DefaultBuffSize, &dirEntData.entryCount, dirEntData.dirEntList)) {
        dirEntData.isExist = 1;
    }

    selectedIndex = -1;
    scroll.offset = 0;
    scroll.page_height = dirEntData.entryCount*32;
    if(height-64-16 < dirEntData.entryCount*32) {
        scroll.page_height += 16;
    }

    return;
}


void flush(void) {

    App_Syscall_DrawSquare(layerId, 0, 64+16, width, height, white);

    if(dirEntData.isExist) {
        for(sintn i=0; i<(sintn)dirEntData.entryCount; i++) {
            if(!((sintn)64+16+i*32-(sintn)scroll.offset+32 < (sintn)(64+16))) {
                if(i == selectedIndex && 64 <= 64+16+i*32-scroll.offset) {
                    App_Syscall_DrawSquare(layerId, 0, 64+16+i*32-scroll.offset, width, 32, blue);
                }

                for(uintn k=0; k<32; k++) {
                    if(dirEntData.dirEntList[i].name[k] == '\0') break;
                    App_Syscall_DrawFont(layerId, 4+k*8, 64+16+10+i*32-scroll.offset, dirEntData.dirEntList[i].name[k], black);
                }

                if(dirEntData.dirEntList[i].type == File_Directory) {
                    App_Syscall_DrawButton(layerId, width-32-16+1, 64+16+i*32-scroll.offset+1, 30, 30, ">");
                    App_Syscall_DrawButton(layerId, width-32*2-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Del");
                    App_Syscall_DrawButton(layerId, width-32*3-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Mov");
                    App_Syscall_DrawButton(layerId, width-32*4-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Cpy");
                }else {
                    App_Syscall_DrawButton(layerId, width-32-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Open");
                    App_Syscall_DrawButton(layerId, width-32*2-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Del");
                    App_Syscall_DrawButton(layerId, width-32*3-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Mov");
                    App_Syscall_DrawButton(layerId, width-32*4-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Cpy");
                    App_Syscall_DrawButton(layerId, width-32*5-16+1, 64+16+i*32-scroll.offset+1, 30, 30, "Run");
                }

                if(0 <= 64+16+32+i*32-scroll.offset-1) App_Syscall_DrawSquare(layerId, 0, 64+16+32+i*32-scroll.offset-1, width, 1, gray);
            }
        }
    }else {
        ascii str[] = "Path Not Found.";
        for(uintn i=0; i<sizeof(str); i++) {
            if(str[i] == '\0') break;
            App_Syscall_DrawFont(layerId, i*8, 64+16, str[i], black);
        }
    }
    App_Syscall_DrawSquare(layerId, 0, 0, width, 64+16, white);

    //パスバー
    App_Syscall_EditBox_Draw(layerId, &pathBar);

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

    //ディレクトリ作成
    App_Syscall_DrawButton(layerId, 1, 0+1, 96-2, 32-2, "NewDirectory");

    //テキストファイル作成
    App_Syscall_DrawButton(layerId, 96+1, 0+1, 64-2, 32-2, "NewText");

    //ディレクトリを表示
    App_Syscall_DrawSquare(layerId, 0, 64, width, 16, ui_color);
    App_Syscall_DrawSquare(layerId, 0, 64+16-1, width-16, 1, gray);
    App_Syscall_DrawFont(layerId, 2, 64, 'n', gray);
    App_Syscall_DrawFont(layerId, 2+8, 64, 'a', gray);
    App_Syscall_DrawFont(layerId, 2+8*2, 64, 'm', gray);
    App_Syscall_DrawFont(layerId, 2+8*3, 64, 'e', gray);

    App_Syscall_DrawFont(layerId, width-64-16, 64, 's', gray);
    App_Syscall_DrawFont(layerId, width-64-16+8, 64, 'i', gray);
    App_Syscall_DrawFont(layerId, width-64-16+8*2, 64, 'z', gray);
    App_Syscall_DrawFont(layerId, width-64-16+8*3, 64, 'e', gray);

    //スクロールバー
    App_Syscall_DrawScrollBar(layerId, &scroll);

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


