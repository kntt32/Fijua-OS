.global App_Syscall_NewWindow
.global App_Syscall_CloseWindow
.global App_Syscall_YieldCpu
.global App_Syscall_DrawSquare
.global App_Syscall_DrawFont
.global App_Syscall_ReadMessage
.global App_Syscall_CheckMessage
.global App_Syscall_SendIPCMessage
.global App_Syscall_Exit
.global App_Syscall_GetStdIoTaskId
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
.global App_Syscall_ExitStdIo
.global App_Syscall_Confirm
.global App_Syscall_GetThisTaskId
.global App_Syscall_GetDisplaySize
.global App_Syscall_Alert
.global App_Syscall_EditBox
.global App_Syscall_Prompt
.global App_Syscall_TextBox
.global App_Syscall_SetClipBoard
.global App_Syscall_GetClipBoard
.global App_Syscall_DrawButton
.global App_Syscall_DrawButton_Pushed
.global App_Syscall_DrawButton_NotActive
.global App_Syscall_DrawScrollBar
.global App_Syscall_DrawScrollBar_Response
.global App_Syscall_DrawShade
.global App_Syscall_EditBox_Draw
.global App_Syscall_EditBox_Response

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

App_Syscall_GetStdIoTaskId:
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

App_Syscall_ExitStdIo:
    mov $23, %rax
    callq *(0x100000)
    ret

App_Syscall_Confirm:
    mov $24, %rax
    callq *(0x100000)
    ret

App_Syscall_GetThisTaskId:
    mov $25, %rax
    callq *(0x100000)
    ret

App_Syscall_GetDisplaySize:
    mov $26, %rax
    callq *(0x100000)
    ret

App_Syscall_Alert:
    mov $27, %rax
    callq *(0x100000)
    ret

App_Syscall_EditBox:
    mov $28, %rax
    callq *(0x100000)
    ret

App_Syscall_Prompt:
    mov $29, %rax
    callq *(0x100000)
    ret


App_Syscall_TextBox:
    mov $30, %rax
    callq *(0x100000)
    ret

App_Syscall_SetClipBoard:
    mov $31, %rax
    callq *(0x100000)
    ret

App_Syscall_GetClipBoard:
    mov $32, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawButton:
    mov $33, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawButton_Pushed:
    mov $34, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawButton_NotActive:
    mov $35, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawScrollBar:
    mov $36, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawScrollBar_Response:
    mov $37, %rax
    callq *(0x100000)
    ret

App_Syscall_DrawShade:
    mov $38, %rax
    callq *(0x100000)
    ret

App_Syscall_EditBox_Draw:
    mov $39, %rax
    callq *(0x100000)
    ret

App_Syscall_EditBox_Response:
    mov $40, %rax
    callq *(0x100000)
    ret
