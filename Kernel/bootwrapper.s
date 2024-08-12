.global entrypoint

entrypoint:
    # rcx: kernelInput
    push %rbx
    push %rsi
    push %rdi

    mov %rcx, %rdi
    call Main

    pop %rdi
    pop %rsi
    pop %rbx

    ret