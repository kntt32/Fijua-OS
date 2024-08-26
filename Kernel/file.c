#include <types.h>
#include <kernel.h>
#include <efi_simple_file_system_protocol.h>
#include <efi_file_protocol.h>
#include <efi_file_info.h>
#include "file.h"
#include "console.h"
#include "x64.h"
#include "functions.h"

#define IsValidPathChar(c) (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == '_') || (c == '.') || (c == '-'))

extern KernelInputStruct* KernelInput;

static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* Efi_SimpleFileSystemProtocol = NULL;

//初期化
void File_Init(void) {
    uintn status;

    Console_Print("File_Init: Initializing...\n");

    //Efi_Simple_File_System_Protocolのセット
    EFI_GUID Efi_SimpleFileSystemProtocol_Guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    
    status = Efi_Wrapper(
        KernelInput->LoadedImage->SystemTable->BootServices->HandleProtocol,
        KernelInput->LoadedImage->DeviceHandle,
        &Efi_SimpleFileSystemProtocol_Guid,
        &Efi_SimpleFileSystemProtocol
        );
    if(status) {
        Console_Print("File_Init: Initializing Failed\n");
        while(1) Hlt();
    }

    return;
}


//ディレクトリか判定 負数なら不明なエラー
static sintn File_IsDirectory(EFI_FILE_PROTOCOL* fileProtocol) {
    uintn status;
    EFI_GUID fileInfo_guid = EFI_FILE_INFO_ID;

    uintn buffSize = 0;
    Efi_Wrapper(
        fileProtocol->GetInfo,
        fileProtocol,
        &fileInfo_guid,
        &buffSize,
        NULL
        );
    uint8 buff[buffSize];
    status = Efi_Wrapper(
        fileProtocol->GetInfo,
        fileProtocol,
        &fileInfo_guid,
        &buffSize,
        buff);
    if(status) return -1;

    EFI_FILE_INFO* Efi_fileInfo = (EFI_FILE_INFO*)buff;
    if(Efi_fileInfo->Attribute & EFI_FILE_DIRECTORY) return 1;
    return 0;
}


//パス変換 pathDepthにパスの深さが返される　pathUtf16Le[]はpath[]以上の長さである必要がある
static uintn File_ConvertPath(const ascii path[], uint16 pathUtf16Le[], uintn* pathDepth) {
    sintn pathUtf16Le_index = 0;
    sintn index = -1;
    sintn startIndex = 0;

    *pathDepth = 0;

    while(1) {
        index ++;
        
        if(path[index] == '/' || path[index] == '\0') {
            if(index != startIndex) {
                (*pathDepth) ++;
            }

            if((index-startIndex == 1 && path[startIndex] == '.')
                || (index-startIndex == 2 && path[startIndex] == '.' && path[startIndex+1] == '.')) {
                return 1;//無効なパス
            }
            if(index != startIndex) {
                for(sintn i=0; i<index - startIndex + 1; i++) {
                    Functions_ASCII2UTF16LE((path[startIndex + i] == '/')?('\\'):(path[startIndex + i]), &pathUtf16Le[pathUtf16Le_index]);
                    pathUtf16Le_index ++;
                }
            }
            if(path[index] == '\0') {
                Functions_ASCII2UTF16LE('\0', &(pathUtf16Le[pathUtf16Le_index]));
                break;
            }
            startIndex = index + 1;
            continue;
        }

        if(!IsValidPathChar(path[index])) {
            return 2;//無効なパス
        }
    }
    return 0;
}


//ディレクトリエントリを作成
static uintn File_DirectoryEntry_SetByDirHandle(EFI_FILE_PROTOCOL* Efi_fileProtocol, uintn* buffCount, File_DirectoryEntry buff[]) {
    if(Efi_fileProtocol == NULL || buffCount == NULL || (*buffCount != 0 && buff == NULL)) {
        *buffCount = 0;
        return 1;//無効な入力
    }
    uintn status;

    uintn directoryEntryIndex = 0;
    while(1) {
        uintn infobuffSize = 0;
        Efi_Wrapper(
            Efi_fileProtocol->Read,
            Efi_fileProtocol,
            &infobuffSize,
            NULL);
        if(infobuffSize == 0) {
            if(*buffCount < directoryEntryIndex) {
                *buffCount = directoryEntryIndex;
                return 2;
            }
            *buffCount = directoryEntryIndex;
            return 0;
        }

        uint8 infobuff[infobuffSize];
        status = Efi_Wrapper(
            Efi_fileProtocol->Read,
            Efi_fileProtocol,
            &infobuffSize,
            infobuff);
        if(status) {
            *buffCount = 0;
            return 3;
        }

        if(!(*buffCount <= directoryEntryIndex)) {
            EFI_FILE_INFO* Efi_fileInfo = (EFI_FILE_INFO*)infobuff;
            buff[directoryEntryIndex].type = (Efi_fileInfo->Attribute & EFI_FILE_DIRECTORY)?(File_Directory):(File_File);
            for(uintn i=0; i<31; i++) {
                Functions_UTF16LE2ASCII(Efi_fileInfo->FileName[i], &(buff[directoryEntryIndex].name[i]));
                if(Efi_fileInfo->FileName[i] == 0) break;
            }
            buff[directoryEntryIndex].name[31] = '\0';
            buff[directoryEntryIndex].size = Efi_fileInfo->FileSize;
            if(Functions_StrCmp(".", buff[directoryEntryIndex].name) == 0 || Functions_StrCmp("..", buff[directoryEntryIndex].name) == 0) continue;
        }
        directoryEntryIndex ++;
    }
}


//ディレクトリエントリを作成
static uintn File_DirectoryEntry_SetByFileHandle(EFI_FILE_PROTOCOL* Efi_fileProtocol, File_DirectoryEntry* buff) {
    if(Efi_fileProtocol == NULL || buff == NULL) return 1;

    uintn status;
    EFI_GUID fileInfo_guid = EFI_FILE_INFO_ID;

    uintn buffSize = 0;
    Efi_Wrapper(
        Efi_fileProtocol->GetInfo,
        Efi_fileProtocol,
        &fileInfo_guid,
        &buffSize,
        NULL
        );
    uint8 infobuff_uint8[buffSize];
    status = Efi_Wrapper(
        Efi_fileProtocol->GetInfo,
        Efi_fileProtocol,
        &fileInfo_guid,
        &buffSize,
        infobuff_uint8);
    if(status) return 2;
    EFI_FILE_INFO* Efi_fileInfo = (EFI_FILE_INFO*)infobuff_uint8;
    buff->type = (Efi_fileInfo->Attribute & EFI_FILE_DIRECTORY)?(File_Directory):(File_File);
    for(uintn i=0; i<31; i++) {
        Functions_UTF16LE2ASCII(Efi_fileInfo->FileName[i], &(buff->name[i]));
        if(Efi_fileInfo->FileName[i] == 0) break;
    }
    buff->name[31] = '\0';
    buff->size = Efi_fileInfo->FileSize;

    return 0;
}


//ファイルハンドルを取得
static EFI_FILE_PROTOCOL* File_GetHandleByPath(const ascii path[], uint64 openMode) {
    uintn status;

    uintn pathLength = Functions_CountStr(path);
    uint16 pathUtf16Le[pathLength+2];
    uintn pathDepth;
    if(path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
        pathUtf16Le[0] = L'\0';
        pathUtf16Le[1] = L'\0';
        pathDepth = 0;
    }else {
        status = File_ConvertPath(path, pathUtf16Le, &pathDepth);
        if(status) {
            return NULL;//無効なパス
        }
    }

    EFI_FILE_PROTOCOL* Efi_fileProtocol;

    //ルートディレクトリ
    status = Efi_Wrapper(
        Efi_SimpleFileSystemProtocol->OpenVolume,
        Efi_SimpleFileSystemProtocol,
        &Efi_fileProtocol
        );

    if(pathDepth != 0) {
        if(openMode & EFI_FILE_MODE_CREATE) {
            //ディレクトリを開いていく
            uintn targetPathStrIndex = 0;
            for(uintn i=0; i<pathLength; i++) {
                if(pathUtf16Le[i] != L'\\') continue;
                pathUtf16Le[i] = L'\0';

                EFI_FILE_PROTOCOL* temp_Efi_fileProtocol;

                status = Efi_Wrapper(
                    Efi_fileProtocol->Open,
                    Efi_fileProtocol,
                    &temp_Efi_fileProtocol,
                    pathUtf16Le + targetPathStrIndex,
                    EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                    EFI_FILE_DIRECTORY
                    );
                Efi_Wrapper(
                    Efi_fileProtocol->Close,
                    Efi_fileProtocol);
                if(status) {
                    return NULL;//失敗
                }

                Efi_fileProtocol = temp_Efi_fileProtocol;
                targetPathStrIndex = i+1;
            }

            EFI_FILE_PROTOCOL* temp_Efi_fileProtocol;
            status = Efi_Wrapper(
                Efi_fileProtocol->Open,
                Efi_fileProtocol,
                &temp_Efi_fileProtocol,
                pathUtf16Le + targetPathStrIndex,
                EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE,
                0);
            Efi_Wrapper(
                Efi_fileProtocol->Close,
                Efi_fileProtocol);
            Efi_fileProtocol = temp_Efi_fileProtocol;
            if(status) return NULL;
        }else {
            EFI_FILE_PROTOCOL* temp_Efi_fileProtocol;
            status = Efi_Wrapper(
                Efi_fileProtocol->Open,
                Efi_fileProtocol,
                &temp_Efi_fileProtocol,
                pathUtf16Le,
                openMode,
                0
                );
            Efi_Wrapper(
                Efi_fileProtocol->Close,
                Efi_fileProtocol);
            if(status) {
                return NULL;
            }
            Efi_fileProtocol = temp_Efi_fileProtocol;
        }
    }

    return Efi_fileProtocol;
}


//ディレクトリリスト取得
uintn File_GetDirectory(const ascii path[], uintn* buffCount, File_DirectoryEntry buff[]) {
    if(path == NULL || buffCount == NULL || (*buffCount != 0 && buff == NULL)) {
        *buffCount = 0;
        return 1;
    }//無効な入力

    uintn status = 0;

    //ハンドル取得
    EFI_FILE_PROTOCOL* Efi_fileProtocol = File_GetHandleByPath(path, EFI_FILE_MODE_READ);
    if(Efi_fileProtocol == NULL) {
        *buffCount = 0;
        return 2;
    }

    //ディレクトリか判定
    if(File_IsDirectory(Efi_fileProtocol) <= 0) {
        Efi_Wrapper(
            Efi_fileProtocol->Close,
            Efi_fileProtocol);
        *buffCount = 0;
        return 5;//ディレクトリでない
    }

    //ディレクトリエントリを作成
    status = File_DirectoryEntry_SetByDirHandle(Efi_fileProtocol, buffCount, buff);
    if(status) {
        Efi_Wrapper(
            Efi_fileProtocol->Close,
            Efi_fileProtocol);
        return 6;
    }

    //ハンドルをクローズ
    Efi_Wrapper(
        Efi_fileProtocol->Close,
        Efi_fileProtocol);

    return 0;
}


//ファイルパスのディレクトリエントリを取得
uintn File_GetDirectoryEntryByPath(const ascii path[], File_DirectoryEntry* buff) {
    if(path == NULL || buff == NULL) return 1;

    uintn status;

    EFI_FILE_PROTOCOL* Efi_fileProtocol = File_GetHandleByPath(path, EFI_FILE_MODE_READ);
    if(Efi_fileProtocol == NULL) {
        return 2;
    }

    status = File_DirectoryEntry_SetByFileHandle(Efi_fileProtocol, buff);
    if(status) {
        Efi_Wrapper(
            Efi_fileProtocol->Close,
            Efi_fileProtocol);
        return 3;
    }

    Efi_Wrapper(
        Efi_fileProtocol->Close,
        Efi_fileProtocol);

    return 0;
}


//ファイルパスのファイルを開く
uintn File_OpenAndMMapFile(const ascii path[], uintn buffSize, void* buff) {
    if(path == NULL || buff == NULL) return 1;

    uintn status = 0;
    EFI_FILE_PROTOCOL* Efi_fileProtocol = File_GetHandleByPath(path, EFI_FILE_MODE_READ);
    if(Efi_fileProtocol == NULL) {
        return 2;
    }

    if(0 < File_IsDirectory(Efi_fileProtocol)) {
        Efi_Wrapper(
            Efi_fileProtocol->Close,
            Efi_fileProtocol);
        return 3;
    }

    status = Efi_Wrapper(
        Efi_fileProtocol->Read,
        Efi_fileProtocol,
        &buffSize,
        buff);
    if(status) {
        Efi_Wrapper(
            Efi_fileProtocol->Close,
            Efi_fileProtocol);
        return 4;
    }

    Efi_Wrapper(
        Efi_fileProtocol->Close,
        Efi_fileProtocol);

    return 0;
}


//ファイルの書き込み
uintn File_WriteFromMem(const ascii path[], uintn buffSize, void* buff) {
    if(path == NULL || (buffSize != 0 && buff == NULL)) return 1;

    uintn status = 0;
    EFI_FILE_PROTOCOL* Efi_fileProtocol = File_GetHandleByPath(path, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ);
    if(Efi_fileProtocol == NULL) {
        return 2;
    }

    status = Efi_Wrapper(
        Efi_fileProtocol->SetPosition,
        Efi_fileProtocol,
        0);
    if(status) return 3;

    status = Efi_Wrapper(
        Efi_fileProtocol->Write,
        Efi_fileProtocol,
        &buffSize,
        buff);
    if(status) return 4;

    Efi_Wrapper(
        Efi_fileProtocol->Close,
        Efi_fileProtocol);

    return 0;
}


//ファイルの削除
uintn File_Remove(const ascii path[]) {
    if(path == NULL) return 1;

    uintn status;

    EFI_FILE_PROTOCOL* Efi_fileProtocol = File_GetHandleByPath(path, EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE);
    if(Efi_fileProtocol == NULL) return 2;

    status = Efi_Wrapper(
        Efi_fileProtocol->Delete,
        Efi_fileProtocol);
    if(status) return 3;
    return 0;
}
