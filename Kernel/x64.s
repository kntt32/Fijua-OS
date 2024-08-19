.data



.text

.global Hlt
.global Efi_Wrapper
.global Timer_Wrapper
.global Mutex_Lock
.global Mutex_UnLock
.global Task_ContextSwitch
.global Task_NewTask_Asm_SetStartContext
.global Syscall_AppEnter


Hlt:
    hlt
    ret


# uintn Efi_Wrapper(uintn (*callback)(), ...);
Efi_Wrapper:
    push %r9
    push %r8
    push %rcx
    push %rdx
    push %rsi

    mov %r8, %r9
    mov %rcx, %r8
    mov %rdx, %rdx
    mov %rsi, %rcx

    call *%rdi

    pop %r9
    pop %r9
    pop %r9
    pop %r9
    pop %r9

    ret


# void Timer_Wrapper(void*, uintn (*callback)(void));
Timer_Wrapper:
    push %rbp
    push %rdi
    push %rsi

    call *%rdx

    pop %rsi
    pop %rdi
    pop %rbp

    ret


# void Mutex_Lock(uintn* lockvar);
Mutex_Lock:
    cmpq $0, (%rdi)
    je Mutex_Lock_GetLock
    pause
    jmp Mutex_Lock
Mutex_Lock_GetLock:
    movq $1, %rax
    xchg (%rdi), %rax
    cmpq $0, %rax
    jne Mutex_Lock
    ret


# void Mutex_UnLock(uintn* lockvar);
Mutex_UnLock:
    movq $0, %rax
    xchg %rax, (%rdi)
    cmpq $1, %rax
    jne Mutex_UnLock_Err
    movq $0, %rax
    ret
Mutex_UnLock_Err:
    movq $1, %rax
    ret


# void Task_ContextSwitch(void);
Task_ContextSwitch:
    push %rax
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
    push %rbx

    mov %rsp, %rdi
    call Task_ContextSwitch_Subroutine
    mov %rax, %rsp

    pop %rbx
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rax

    ret


# void* Task_NewTask_Asm_SetStartContext(void* stackptr);
# input: stack ptr
# return: context saved stack ptr
Task_NewTask_Asm_SetStartContext:
    and $0xfffffffffffffff0, %rdi
    mov %rsp, %rax
    mov %rdi, %rsp
    
    leaq Task_New_StartPoint(%rip), %r10

    pushq $0x0 #dummy

    pushq %r10   #rip

    pushq $0x0                      #rax
    pushq $0x0                  #rbp
    pushq $0x0                  #r12
    pushq $0x0                  #r13
    pushq $0x0                  #r14
    pushq $0x0                  #r15
    pushq $0x0                  #rbx

    mov %rsp, %rdi
    mov %rax, %rsp

    mov %rdi, %rax
    ret


# uintn Syscall_AppEnter(...);
Syscall_AppEnter:#16バイトアライメントの必要なし
    mov %rsp, %r10
    and $0xfffffffffffffff0, %rsp
    push %r10
    push %r10

    #システムコール番号で分岐
    cmp $0, %rax
    je Syscall_AppEnter_Syscall_NewWindow

    cmp $1, %rax
    je Syscall_AppEnter_Syscall_CloseWindow

    cmp $2, %rax
    je Syscall_AppEnter_Syscall_YieldCpu

    cmp $3, %rax
    je Syscall_AppEnter_Syscall_DrawSquare

    cmp $4, %rax
    je Syscall_AppEnter_Syscall_DrawFont

    cmp $5, %rax
    je Syscall_AppEnter_Syscall_ReadMessage

    cmp $6, %rax
    je Syscall_AppEnter_Syscall_CheckMessage

    cmp $7, %rax
    je Syscall_AppEnter_Syscall_SendIPCMessage

    cmp $8, %rax
    je Syscall_AppEnter_Syscall_Exit

    cmp $9, %rax
    je Syscall_AppEnter_Syscall_GetStdIoTaskId

    cmp $10, %rax
    je Syscall_AppEnter_Syscall_StdOut

    cmp $11, %rax
    je Syscall_AppEnter_Syscall_StdIn

    cmp $12, %rax
    je Syscall_AppEnter_Syscall_StdOut_Cls

    cmp $13, %rax
    je Syscall_AppEnter_Syscall_GetFileList

    cmp $14, %rax
    je Syscall_AppEnter_Syscall_GetDirEntryByPath

    cmp $15, %rax
    je Syscall_AppEnter_Syscall_MMapFile

    cmp $16, %rax
    je Syscall_AppEnter_Syscall_WriteFileFromMem

    cmp $17, %rax
    je Syscall_AppEnter_Syscall_RemoveFile

    cmp $18, %rax
    je Syscall_AppEnter_Syscall_MkDir

    cmp $19, %rax
    je Syscall_AppEnter_Syscall_ShutDown

    cmp $20, %rax
    je Syscall_AppEnter_Syscall_AllocPage

    cmp $21, %rax
    je Syscall_AppEnter_Syscall_FreePages

    cmp $22, %rax
    je Syscall_AppEnter_Syscall_RunApp

    #無効なシステムコール番号
    mov $-1, %rax
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_NewWindow:
    call Syscall_NewWindow
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_CloseWindow:
    call Syscall_CloseWindow
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_YieldCpu:
    call Syscall_YieldCpu
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_DrawSquare:
    call Syscall_DrawSquare
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_DrawFont:
    call Syscall_DrawFont
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_ReadMessage:
    call Syscall_ReadMessage
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_CheckMessage:
    call Syscall_CheckMessage
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_SendIPCMessage:
    call Syscall_SendIPCMessage
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_GetStdIoTaskId:
    call Syscall_GetStdIoTaskId
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_Exit:
    call Syscall_Exit
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_StdOut:
    call Syscall_StdOut
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_StdIn:
    call Syscall_StdIn
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_StdOut_Cls:
    call Syscall_StdOut_Cls
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_GetFileList:
    call Syscall_GetFileList
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_GetDirEntryByPath:
    call Syscall_GetDirEntryByPath
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_MMapFile:
    call Syscall_MMapFile
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_WriteFileFromMem:
    call Syscall_WriteFileFromMem
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_RemoveFile:
    call Syscall_RemoveFile
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_MkDir:
    call Syscall_MkDir
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_ShutDown:
    call Syscall_ShutDown
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_AllocPage:
    call Syscall_AllocPage
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_FreePages:
    call Syscall_FreePages
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Syscall_RunApp:
    call Syscall_RunApp
    jmp Syscall_AppEnter_Exit

Syscall_AppEnter_Exit:
    pop %r10
    pop %r10
    mov %r10, %rsp
    ret
