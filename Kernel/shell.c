#include <types.h>
#include "graphic.h"
#include "queue.h"
#include "task.h"
#include "shell.h"
#include "file.h"
#include <app_syscall_x64.h>

#define Shell_DefaultBuffSize (128)
void SPrintIntX(uintn number, uintn buffsize, ascii buff[]);

typedef enum {
    Shell_CmdType_UnKnown,
    Shell_CmdType_Cls,
    Shell_CmdType_Echo,
    Shell_CmdType_Ls,
    Shell_CmdType_Cd,
    Shell_CmdType_Mv,
    Shell_CmdType_Cp,
    Shell_CmdType_Rm,
    Shell_CmdType_MkDir,
    Shell_CmdType_Cat,
    Shell_CmdType_Touch,
    Shell_CmdType_ShutDown,
    Shell_CmdType_Run
} Shell_CmdType;


Shell_CmdType Shell_GetCmd(const ascii shellInput[]);
const ascii* Shell_Cmd_GetInput(const ascii shellInput[]);
void Shell_Cmd_Echo(const ascii cmdInput[]);
void Shell_Cmd_Cls(void);
void Shell_Cmd_Ls(const ascii cmdInput[Shell_DefaultBuffSize], const ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_Cd(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_Mv(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_Cp(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_Rm(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_MkDir(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_Cat(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_Touch(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);
void Shell_Cmd_Run(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]);


sintn Shell_Main(void) {
    sintn status;

    ascii strBuff[Shell_DefaultBuffSize];
    ascii workingPath[Shell_DefaultBuffSize] = "";

    while(1) {
        if(workingPath[0] == '\0') App_Syscall_StdOut("/", sizeof("/"));
        else App_Syscall_StdOut(workingPath, Shell_DefaultBuffSize);
        App_Syscall_StdOut("> ", sizeof("> "));
        
        status = App_Syscall_StdIn(strBuff, sizeof(strBuff));
        if(status != 0) {
            App_Syscall_StdOut("Shell: too large input\n", sizeof("Shell: too large input\n"));
            continue;
        }

        if(strBuff[0] == '\0') {
            continue;
        }
        
        Shell_CmdType cmdType = Shell_GetCmd(strBuff);
        const ascii* cmdInput = Shell_Cmd_GetInput(strBuff);
        switch(cmdType) {
            case Shell_CmdType_UnKnown:
                App_Syscall_StdOut("Shell: UnSupported Input: ", sizeof("Shell: UnSupported Input: "));
                App_Syscall_StdOut(strBuff, Shell_DefaultBuffSize);
                App_Syscall_StdOut("\n", 2);
                break;
            case Shell_CmdType_Cls:
                Shell_Cmd_Cls();
                break;
            case Shell_CmdType_Echo:
                Shell_Cmd_Echo(cmdInput);
                break;
            case Shell_CmdType_Ls:
                Shell_Cmd_Ls(cmdInput, workingPath);
                break;
            case Shell_CmdType_Cd:
                Shell_Cmd_Cd(cmdInput, workingPath);
                break;
            case Shell_CmdType_Mv:
                Shell_Cmd_Mv(cmdInput, workingPath);
                break;
            case Shell_CmdType_Cp:
                Shell_Cmd_Cp(cmdInput, workingPath);
                break;
            case Shell_CmdType_Rm:
                Shell_Cmd_Rm(cmdInput, workingPath);
                break;
            case Shell_CmdType_MkDir:
                Shell_Cmd_MkDir(cmdInput, workingPath);
                break;
            case Shell_CmdType_Cat:
                Shell_Cmd_Cat(cmdInput, workingPath);
                break;
            case Shell_CmdType_Touch:
                Shell_Cmd_Touch(cmdInput, workingPath);
                break;
            case Shell_CmdType_ShutDown:
                App_Syscall_ShutDown();
                break;
            case Shell_CmdType_Run:
                Shell_Cmd_Run(cmdInput, workingPath);
                break;
            default:
                App_Syscall_StdOut("Shell: Unknown Err Occured\n", sizeof("Shell: Unknown Err Occured\n"));
        }
    }

    return 0;
}


//str1が終わるまででstr2がstr1と完全に等しいかどうか
uintn Shell_Cmd_IsEqualStr(const ascii str1[], const ascii str2[]) {
    if(str1 == NULL || str2 == NULL) return 0;

    for(uintn i=0; 1; i++) {
        if(str1[i] == '\0') return 1;
        if(str1[i] != str2[i]) return 0;
    }
    return 1;
}


//NULL文字を含まない文字列の文字数を取得
uintn Shell_CountStr(const ascii str[]) {
    if(str == NULL) return 0;

    for(uintn i=0; 1; i++) {
        if(str[i] == '\0') return i;
    }
    return 0;
}


//Shellへのユーザー入力とコマンド名が一致しているか判定
static uintn Shell_GetCmd_CmdCmp(const ascii shellInput[], const ascii cmd[]) {
    uintn cmdLength = Shell_CountStr(cmd);
    const ascii* shellInputStart = shellInput;
    
    while(1) {
        if(*shellInputStart != ' ') break;
        shellInputStart ++;
    }

    if(Shell_Cmd_IsEqualStr(cmd, shellInputStart) && (shellInputStart[cmdLength] == ' ' || shellInputStart[cmdLength] == '\0')) return 1;

    return 0;
}


//Shellへのユーザー入力からコマンドを取得
Shell_CmdType Shell_GetCmd(const ascii shellInput[]) {
    if(shellInput == NULL) return Shell_CmdType_UnKnown;

    //Cls
    if(Shell_GetCmd_CmdCmp(shellInput, "cls")) {
        return Shell_CmdType_Cls;
    }
    //Echo
    if(Shell_GetCmd_CmdCmp(shellInput, "echo")) {
        return Shell_CmdType_Echo;
    }
    //Ls
    if(Shell_GetCmd_CmdCmp(shellInput, "ls")) {
        return Shell_CmdType_Ls;
    }
    //Cd
    if(Shell_GetCmd_CmdCmp(shellInput, "cd")) {
        return Shell_CmdType_Cd;
    }
    //Mv
    if(Shell_GetCmd_CmdCmp(shellInput, "mv")) {
        return Shell_CmdType_Mv;
    }
    //Cp
    if(Shell_GetCmd_CmdCmp(shellInput, "cp")) {
        return Shell_CmdType_Cp;
    }
    //Rm
    if(Shell_GetCmd_CmdCmp(shellInput, "rm")) {
        return Shell_CmdType_Rm;
    }
    //MkDir
    if(Shell_GetCmd_CmdCmp(shellInput, "mkdir")) {
        return Shell_CmdType_MkDir;
    }
    //Cat
    if(Shell_GetCmd_CmdCmp(shellInput, "cat")) {
        return Shell_CmdType_Cat;
    }
    //Touch
    if(Shell_GetCmd_CmdCmp(shellInput, "touch")) {
        return Shell_CmdType_Touch;
    }
    //Shutdown
    if(Shell_GetCmd_CmdCmp(shellInput, "shutdown")) {
        return Shell_CmdType_ShutDown;
    }
    //Run
    if(Shell_GetCmd_CmdCmp(shellInput, "run")) {
        return Shell_CmdType_Run;
    }

    return Shell_CmdType_UnKnown;
}


//コマンドの引数を取得 ない場合NULLを返す
const ascii* Shell_Cmd_GetInput(const ascii shellInput[]) {
    if(shellInput == NULL) return NULL;

    while(1) {
        if(*shellInput != ' ' || *shellInput == '\0') break;
        shellInput ++;
    }
    while(1) {
        if(*shellInput == ' ' || *shellInput == '\0') break;
        shellInput ++;
    }
    while(1) {
        if(*shellInput != ' ' || *shellInput == '\0') break;
        shellInput ++;
    }

    if(*shellInput == '\0') return NULL;
    return shellInput;
}


//Clsコマンド
void Shell_Cmd_Cls(void) {
    App_Syscall_StdOut_Cls();
    return;
}


//Echoコマンド
void Shell_Cmd_Echo(const ascii cmdInput[]) {
    if(cmdInput == NULL) return;
    App_Syscall_StdOut(cmdInput, Shell_CountStr(cmdInput)+1);

    return;
}


//パスの文字列のディレクトリ名の長さを返す
static uintn Shell_Cmd_Ls_GetAbsPath_GetDirNameLength(const ascii* path) {
    if(path == NULL) return 0;

    for(uintn i=0; 1; i++) {
        if(path[i] == '/' || path[i] == '\0') return i;
    }
}


//絶対パスを取得
static uintn Shell_Cmd_GetAbsPath(const ascii relPath[Shell_DefaultBuffSize], const ascii workingPath[Shell_DefaultBuffSize], ascii absPath[Shell_DefaultBuffSize]) {
    if(workingPath == NULL || absPath == NULL) return 1;

    uintn absPath_lengthList[Shell_DefaultBuffSize];
    sintn absPath_lengthList_index = -1;
    sintn absPath_index = 0;

    uintn workingPath_index = 0;
    while(1) {
        uintn nameLength = Shell_Cmd_Ls_GetAbsPath_GetDirNameLength(workingPath + workingPath_index);
        if(nameLength != 0) {
            if(nameLength == 1 && workingPath[workingPath_index] == '.') {//./
                //do nothing
            }else if(nameLength == 2 && workingPath[workingPath_index] == '.' && workingPath[workingPath_index + 1] == '.') {//../
                if(absPath_lengthList_index < 0) return 2;
                absPath_index -= absPath_lengthList[absPath_lengthList_index];
                absPath_lengthList_index --;
            }else {
                for(uintn i=0; i<nameLength; i++) {
                    if(Shell_DefaultBuffSize <= workingPath_index + i || Shell_DefaultBuffSize <= absPath_index + i) return 3;
                    absPath[absPath_index + i] = workingPath[workingPath_index + i];
                }
                if(Shell_DefaultBuffSize <= absPath_index + nameLength) return 4;
                absPath[absPath_index + nameLength] = '/';
                absPath_index += nameLength + 1;
                absPath_lengthList_index ++;
                if(Shell_DefaultBuffSize <= absPath_lengthList_index) return 5;
                absPath_lengthList[absPath_lengthList_index] = nameLength + 1;
            }
        }
        if(Shell_DefaultBuffSize <= workingPath_index + nameLength) return 6;
        if(workingPath[workingPath_index + nameLength] == '\0') {
            if(Shell_DefaultBuffSize <= absPath_index) return 7;
            break;
        }

        workingPath_index += nameLength + 1;
    }

    uintn relPath_index = 0;
    while(1) {
        uintn nameLength = Shell_Cmd_Ls_GetAbsPath_GetDirNameLength(relPath + relPath_index);
        if(nameLength != 0) {
            if(nameLength == 1 && relPath[relPath_index] == '.') {//./
                //do nothing
            }else if(nameLength == 2 && relPath[relPath_index] == '.' && relPath[relPath_index + 1] == '.') {//../
                if(absPath_lengthList_index < 0) return 2;

                absPath_index -= absPath_lengthList[absPath_lengthList_index];
                absPath_lengthList_index --;
            }else {
                for(uintn i=0; i<nameLength; i++) {
                    if(Shell_DefaultBuffSize <= relPath_index + i || Shell_DefaultBuffSize <= absPath_index + i) return 3;
                    absPath[absPath_index + i] = relPath[relPath_index + i];
                }
                if(Shell_DefaultBuffSize <= absPath_index + nameLength) return 4;
                absPath[absPath_index + nameLength] = '/';
                absPath_index += nameLength + 1;
                absPath_lengthList_index ++;
                if(Shell_DefaultBuffSize <= absPath_lengthList_index) return 5;
                absPath_lengthList[absPath_lengthList_index] = nameLength + 1;
            }
        }

        if(Shell_DefaultBuffSize <= relPath_index + nameLength) return 6;
        if(relPath[relPath_index + nameLength] == '\0') {
            if(Shell_DefaultBuffSize <= absPath_index) return 7;
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


//Lsコマンド
void Shell_Cmd_Ls(const ascii cmdInput[Shell_DefaultBuffSize], const ascii workingPath[Shell_DefaultBuffSize]) {
    uintn status = 0;

    ascii absPath[Shell_DefaultBuffSize];

    //絶対パスの取得
    status = Shell_Cmd_GetAbsPath(cmdInput, workingPath, absPath);
    if(status) {
        App_Syscall_StdOut("ls: Invalid Path\n", sizeof("ls: Invalid Path\n"));
        return;
    }
    
    //ディレクトリエントリの取得
    uintn buffCount = 0;
    App_Syscall_GetFileList(absPath, Shell_DefaultBuffSize, &buffCount, NULL);
    File_DirectoryEntry buff[buffCount];
    status = App_Syscall_GetFileList(absPath, Shell_DefaultBuffSize, &buffCount, buff);
    if(status) {
        App_Syscall_StdOut("ls: Path not Found\n", sizeof("ls: Path not Found\n"));
        return;
    }

    //表示
    for(uintn i=0; i<buffCount; i++) {
        App_Syscall_StdOut(buff[i].name, 32);
        App_Syscall_StdOut(" ", 2);
    }
    App_Syscall_StdOut("\n", 2);

    return;
}


//Cdコマンド
void Shell_Cmd_Cd(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    if(cmdInput == NULL) {
        workingPath[0] = '\0';
        return;
    }

    uintn status = 0;

    //絶対パスの取得
    ascii absPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(cmdInput, workingPath, absPath);
    if(status) {
        App_Syscall_StdOut("cd: Invalid Path\n", sizeof("cd: Invalid Path\n"));
        return;
    }

    //移動先がディレクトリか判定
    File_DirectoryEntry dirEntBuff;
    status = App_Syscall_GetDirEntryByPath(absPath, Shell_DefaultBuffSize, &dirEntBuff);
    if(status || dirEntBuff.type != File_Directory) {
        App_Syscall_StdOut("cd: Invalid Path\n", sizeof("cd: Invalid Path\n"));
        return;
    }

    //移動
    for(uintn i=0; i<Shell_DefaultBuffSize; i++) {
        workingPath[i] = absPath[i];
        if(absPath[i] == '\0') break;
    }

    return;
}


//catコマンド
void Shell_Cmd_Cat(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    uintn status = 0;

    //絶対パスの取得
    ascii absPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(cmdInput, workingPath, absPath);
    if(status) {
        App_Syscall_StdOut("cat: Invalid Path\n", sizeof("cat: Invalid Path\n"));
        return;
    }

    File_DirectoryEntry dirEntBuff;
    status = App_Syscall_GetDirEntryByPath(absPath, Shell_DefaultBuffSize, &dirEntBuff);
    if(status) {
        App_Syscall_StdOut("cat: Invalid Path\n", sizeof("cat: Invalid Path\n"));
        return;
    }

    uint8 filebuff[dirEntBuff.size+1];
    status = App_Syscall_MMapFile(absPath, Shell_DefaultBuffSize, sizeof(filebuff), filebuff);
    if(status) {
        App_Syscall_StdOut("cat: Couldn't Open File\n", sizeof("cat: Couldn't Open File\n"));
        return;
    }

    filebuff[dirEntBuff.size] = '\0';
    App_Syscall_StdOut((ascii*)filebuff, sizeof(filebuff));
    App_Syscall_StdOut("\n", 2);

    return;
}


//新しいファイル作成
void Shell_Cmd_Touch(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    uintn status = 0;

    //絶対パスの取得
    ascii absPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(cmdInput, workingPath, absPath);
    if(status) {
        App_Syscall_StdOut("touch: Invalid Path\n", sizeof("touch: Invalid Path\n"));
        return;
    }

    ascii buff[] = "Hello, World!\nCreated by Touch command";
    status = App_Syscall_WriteFileFromMem(absPath, Shell_DefaultBuffSize, sizeof(buff), buff);
    if(status) {
        App_Syscall_StdOut("touch: Couldn't Create File\n", sizeof("touch: Couldn't Create File\n"));
        return;
    }

    return;
}


//Mvコマンド
void Shell_Cmd_Mv(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    uintn status;

    //絶対パスの取得
    uintn cmdInput_seekIndex = 0;

    ascii fromRelPath[Shell_DefaultBuffSize];
    for(uintn i=0; i<Shell_DefaultBuffSize; i++) {
        if(cmdInput[i] == ' ' || cmdInput[i] == '\0') {
            fromRelPath[i] = '\0';
            for(uintn k=i; k<Shell_DefaultBuffSize; k++) {
                if(cmdInput[k] != ' ' || cmdInput[k] == '\0') {
                    cmdInput_seekIndex = k;
                    break;
                }
            }
            if(cmdInput_seekIndex == i) {
                App_Syscall_StdOut("mv: Invalid Argument\n", sizeof("mv: Invalid Argument\n"));
                return;
            }
            break;
        }
        fromRelPath[i] = cmdInput[i];
    }
    ascii fromAbsPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(fromRelPath, workingPath, fromAbsPath);
    if(status) {
        App_Syscall_StdOut("mv: Invalid Path\n", sizeof("mv: Invalid Path\n"));
        return;
    }

    ascii toRelPath[Shell_DefaultBuffSize];
    for(uintn i=0; i<Shell_DefaultBuffSize-cmdInput_seekIndex; i++) {
        if(cmdInput[i+cmdInput_seekIndex] == ' ' || cmdInput[i+cmdInput_seekIndex] == '\0') {
            toRelPath[i] = '\0';
            break;
        }
        toRelPath[i] = cmdInput[i+cmdInput_seekIndex];
    }
    ascii toAbsPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(toRelPath, workingPath, toAbsPath);
    if(status) {
        App_Syscall_StdOut("mv: Invalid Path\n", sizeof("mv: Invalid Path\n"));
        return;
    }


    //ファイルサイズの取得
    File_DirectoryEntry dirEntBuff;
    status = App_Syscall_GetDirEntryByPath(fromAbsPath, Shell_DefaultBuffSize, &dirEntBuff);
    if(status) {
        App_Syscall_StdOut("mv: Invalid Path\n", sizeof("mv: Invalid Path\n"));
        return;
    }

    //ファイルバッファへ書き込み
    uint8 filebuff[dirEntBuff.size];
    status = App_Syscall_MMapFile(fromAbsPath, Shell_DefaultBuffSize, sizeof(filebuff), filebuff);
    if(status) {
        App_Syscall_StdOut("mv: Couldn't Open File\n", sizeof("mv: Couldn't Open File\n"));
        return;
    }

    //ファイルの保存
    status = App_Syscall_WriteFileFromMem(toAbsPath, Shell_DefaultBuffSize, dirEntBuff.size, filebuff);
    if(status) {
        App_Syscall_StdOut("mv: Failed\n", sizeof("mv: Failed"));
        return;
    }

    //元のファイルの消去
    status = App_Syscall_RemoveFile(fromAbsPath, Shell_DefaultBuffSize);
    if(status) {
        App_Syscall_StdOut("mv: Failed\n", sizeof("mv: Failed\n"));
        return;
    }

    return;
}


//Cpコマンド
void Shell_Cmd_Cp(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    uintn status;

    //絶対パスの取得
    uintn cmdInput_seekIndex = 0;

    ascii fromRelPath[Shell_DefaultBuffSize];
    for(uintn i=0; i<Shell_DefaultBuffSize; i++) {
        if(cmdInput[i] == ' ' || cmdInput[i] == '\0') {
            fromRelPath[i] = '\0';
            for(uintn k=i; k<Shell_DefaultBuffSize; k++) {
                if(cmdInput[k] != ' ' || cmdInput[k] == '\0') {
                    cmdInput_seekIndex = k;
                    break;
                }
            }
            if(cmdInput_seekIndex == i) {
                App_Syscall_StdOut("cp: Invalid Argument\n", sizeof("cp: Invalid Argument\n"));
                return;
            }
            break;
        }
        fromRelPath[i] = cmdInput[i];
    }
    ascii fromAbsPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(fromRelPath, workingPath, fromAbsPath);
    if(status) {
        App_Syscall_StdOut("cp: Invalid Path\n", sizeof("cp: Invalid Path\n"));
        return;
    }

    ascii toRelPath[Shell_DefaultBuffSize];
    for(uintn i=0; i<Shell_DefaultBuffSize-cmdInput_seekIndex; i++) {
        if(cmdInput[i+cmdInput_seekIndex] == ' ' || cmdInput[i+cmdInput_seekIndex] == '\0') {
            toRelPath[i] = '\0';
            break;
        }
        toRelPath[i] = cmdInput[i+cmdInput_seekIndex];
    }
    ascii toAbsPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(toRelPath, workingPath, toAbsPath);
    if(status) {
        App_Syscall_StdOut("cp: Invalid Path\n", sizeof("cp: Invalid Path\n"));
        return;
    }


    //ファイルサイズの取得
    File_DirectoryEntry dirEntBuff;
    status = App_Syscall_GetDirEntryByPath(fromAbsPath, Shell_DefaultBuffSize, &dirEntBuff);
    if(status) {
        App_Syscall_StdOut("cp: Invalid Path\n", sizeof("cp: Invalid Path\n"));
        return;
    }

    //ファイルバッファへ書き込み
    uint8 filebuff[dirEntBuff.size];
    status = App_Syscall_MMapFile(fromAbsPath, Shell_DefaultBuffSize, sizeof(filebuff), filebuff);
    if(status) {
        App_Syscall_StdOut("cp: Couldn't Open File\n", sizeof("cp: Couldn't Open File\n"));
        return;
    }

    //ファイルの保存
    status = App_Syscall_WriteFileFromMem(toAbsPath, Shell_DefaultBuffSize, dirEntBuff.size, filebuff);
    if(status) {
        App_Syscall_StdOut("cp: Failed\n", sizeof("cp: Failed"));
        return;
    }

    return;
}


//Rmコマンド
void Shell_Cmd_Rm(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    uintn status;

    ascii absPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(cmdInput, workingPath, absPath);
    if(status) {
        App_Syscall_StdOut("rm: Invalid Path\n", sizeof("rm: Invalid Path\n"));
        return;
    }

    status = App_Syscall_RemoveFile(absPath, Shell_DefaultBuffSize);
    if(status) {
        App_Syscall_StdOut("rm: Failed\n", sizeof("rm: Failed\n"));
        return;
    }

    return;
}


//MkDirコマンド
void Shell_Cmd_MkDir(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    uintn status;

    ascii absPath[Shell_DefaultBuffSize];
    status = Shell_Cmd_GetAbsPath(cmdInput, workingPath, absPath);
    if(status) {
        App_Syscall_StdOut("mkdir: Invalid Path\n", sizeof("mkdir: Invalid Path\n"));
        return;
    }

    App_Syscall_MkDir(absPath, Shell_DefaultBuffSize);
    
    return;
}


//runコマンド　ELF形式の実行可能ファイルを実行する
void Shell_Cmd_Run(const ascii cmdInput[Shell_DefaultBuffSize], ascii workingPath[Shell_DefaultBuffSize]) {
    App_Syscall_StdOut("Coming soon\n", sizeof("Coming soon\n"));
    return;
}

