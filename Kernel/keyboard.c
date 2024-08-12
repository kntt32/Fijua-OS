#include <types.h>
#include <kernel.h>
#include <efi.h>
#include <efi_simple_text_input_protocol.h>
#include "keyboard.h"
#include "functions.h"
#include "x64.h"
#include "queue.h"
#include "task.h"
#include "message.h"
#include "console.h"
#include "graphic.h"
#include "layer.h"

extern KernelInputStruct* KernelInput;

static EFI_SIMPLE_TEXT_INPUT_PROTOCOL* Efi_SimpleTextInputProtocol = NULL;


//キーボードの初期化
void Keyboard_Init(void) {
    Console_Print("keyboard_Init: Initializing...\n");

    uintn status;
    Efi_SimpleTextInputProtocol = KernelInput->LoadedImage->SystemTable->ConIn;

    //Reset
    status = Efi_Wrapper(Efi_SimpleTextInputProtocol->Reset, Efi_SimpleTextInputProtocol, 0);
    if(status) {
        Console_Print("keyboard_Init: Couldn't Reset keyboard\n");
        while(1) Hlt();
    }

    return;
}


//キーボードの状態を取得
void Keyboard_CheckState(void) {
    static EFI_INPUT_KEY Efi_KeyboardState;
    uintn status;

    status = Efi_Wrapper(Efi_SimpleTextInputProtocol->ReadKeyStroke, Efi_SimpleTextInputProtocol, &Efi_KeyboardState);
    if(status) return;

    //タスクにメッセージ送信
    Task_Message message;
    status = Functions_UTF16LE2ASCII(Efi_KeyboardState.UnicodeChar, &(message.data.KeyPushed.asciiCode));
    if(status) message.data.KeyPushed.asciiCode = 0;
    message.data.KeyPushed.scanCode = Efi_KeyboardState.ScanCode;

    message.type = Task_Message_KeyPushed;

    Message_EnQueue(Layer_Window_GetFocusedTaskId(), &message);

    return;
}
