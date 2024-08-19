.global App_Syscall_NewWindow
.global App_Syscall_CloseWindow
.global App_Syscall_YieldCpu
.global App_Syscall_DrawSquare
.global App_Syscall_DrawFont
.global App_Syscall_ReadMessage
.global App_Syscall_CheckMessage
.global App_Syscall_SendIPCMessage
.global App_Syscall_Exit
.global App_Syscall_GetStdOutTaskId
.global App_Syscall_StdOut
.global App_Syscall_StdIn
.global App_Syscall_StdOut_Cls
.global App_Syscall_GetFileList
.global App_Syscall_GetDirEntryByPath
.global App_Syscall_MMapFile
.global App_Syscall_WriteFileFromMem
.global App_Syscall_RemoveFile
.global App_Syscall_MkDir
.global App_Syscall_ShutDown
.global App_Syscall_AllocPage
.global App_Syscall_FreePages
.global App_Syscall_RunApp

.text

App_Syscall_NewWindow:
    mov $0, %rax
    callq *(0x100000)
    ret

App_Syscall_CloseWindow:
    mov $1, %rax
    callq *(0x100000)
    ret

App_Syscall_YieldCpu:
    mov $2, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawSquare:
    mov $3, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawFont:
    mov $4, %rax
    callq *(0x100000)
    ret

App_Syscall_ReadMessage:
    mov $5, %rax
    callq *(0x100000)
    ret

App_Syscall_CheckMessage:
    mov $6, %rax
    callq *(0x100000)
    ret

App_Syscall_SendIPCMessage:
    mov $7, %rax
    callq *(0x100000)
    ret

App_Syscall_Exit:
    mov $8, %rax
    callq *(0x100000)
    ret

App_Syscall_GetStdOutTaskId:
    mov $9, %rax
    callq *(0x100000)
    ret

App_Syscall_StdOut:
    mov $10, %rax
    callq *(0x100000)
    ret

App_Syscall_StdIn:
    mov $11, %rax
    callq *(0x100000)
    ret

App_Syscall_StdOut_Cls:
    mov $12, %rax
    callq *(0x100000)
    ret

App_Syscall_GetFileList:
    mov $13, %rax
    callq *(0x100000)
    ret

App_Syscall_GetDirEntryByPath:
    mov $14, %rax
    callq *(0x100000)
    ret

App_Syscall_MMapFile:
    mov $15, %rax
    callq *(0x100000)
    ret

App_Syscall_WriteFileFromMem:
    mov $16, %rax
    callq *(0x100000)
    ret

App_Syscall_RemoveFile:
    mov $17, %rax
    callq *(0x100000)
    ret

App_Syscall_MkDir:
    mov $18, %rax
    callq *(0x100000)
    ret

App_Syscall_ShutDown:
    mov $19, %rax
    callq *(0x100000)
    ret

App_Syscall_AllocPage:
    mov $20, %rax
    callq *(0x100000)
    ret

App_Syscall_FreePages:
    mov $21, %rax
    callq *(0x100000)
    ret

App_Syscall_RunApp:
    mov $22, %rax
    callq *(0x100000)
    ret
