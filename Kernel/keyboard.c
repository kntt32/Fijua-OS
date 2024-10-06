#include <types.h>
#include <kernel.h>
#include <efi.h>
#include <efi_simple_text_input_protocol.h>
//#include <efi_simple_text_input_ex_protocol.h>
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

//static EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL* Efi_SimpleTextInputExProtocol = NULL;

static EFI_SIMPLE_TEXT_INPUT_PROTOCOL* Efi_SimpleTextInputProtocol = NULL;


//キーボードの初期化
void Keyboard_Init(void) {
    Console_Print("keyboard_Init: Initializing...\n");

    uintn status;

    Efi_SimpleTextInputProtocol = KernelInput->LoadedImage->SystemTable->ConIn;
    status = Efi_Wrapper(Efi_SimpleTextInputProtocol->Reset, Efi_SimpleTextInputProtocol, 0);
    if(status) {
        Console_Print("keyboard_Init: Failed");
    }
    return;

/*
    EFI_GUID Efi_SimpleTextInputExProtocol_guid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
    status = Efi_Wrapper(KernelInput->LoadedImage->SystemTable->BootServices->LocateProtocol,
                &Efi_SimpleTextInputExProtocol_guid, NULL, &Efi_SimpleTextInputExProtocol);
    if(status) {
        Console_Print("keyboard_Init: Failed 1");
        while(1) Hlt();
    }
    Console_Print("Log1");

    status = Efi_Wrapper(Efi_SimpleTextInputExProtocol->Reset,
                Efi_SimpleTextInputExProtocol,
                0);
    if(status) {
        Console_Print("keyboard_Init: Failed 2");
        while(1) Hlt();
    }
    Console_Print("Log2");
*/
    return;
}


//キーボードの状態を取得
void Keyboard_CheckState(void) {
    EFI_INPUT_KEY Efi_KeyboardState;
    static uint8 ctrlFlag = 0;

    uintn status;
    status = Efi_Wrapper(Efi_SimpleTextInputProtocol->ReadKeyStroke, Efi_SimpleTextInputProtocol, &Efi_KeyboardState);
    if(status) return;

    //タスクにメッセージ送信
    Task_Message message;
    message.type = Task_Message_KeyPushed;

    status = Functions_UTF16LE2ASCII(Efi_KeyboardState.UnicodeChar, &(message.data.KeyPushed.asciiCode));
    if(status) message.data.KeyPushed.asciiCode = 0;
    message.data.KeyPushed.scanCode = Efi_KeyboardState.ScanCode;

    if(message.data.KeyPushed.asciiCode != 0) {
        ctrlFlag = 0;
    }
    //if(Efi_KeyboardState.Key.ScanCode == )
    message.data.KeyPushed.ctrlFlag = ctrlFlag;

    Message_EnQueue(Layer_Window_GetFocusedTaskId(), &message);
    return;

/*
    static EFI_KEY_DATA Efi_KeyboardState;

    uintn status;
    status = Efi_Wrapper(Efi_SimpleTextInputExProtocol->ReadKeyStrokeEx,
                Efi_SimpleTextInputExProtocol,
                &Efi_KeyboardState);
    if(status) {
        return;
    }

    //タスクにメッセージ送信
    Task_Message message;

    message.type = Task_Message_KeyPushed;

    status = Functions_UTF16LE2ASCII(Efi_KeyboardState.Key.UnicodeChar, &(message.data.KeyPushed.asciiCode));
    if(status) message.data.KeyPushed.asciiCode = 0;
    message.data.KeyPushed.scanCode = Efi_KeyboardState.Key.ScanCode;

    message.data.KeyPushed.shiftState = Efi_KeyboardState.KeyState.KeyShiftState;    

    Message_EnQueue(Layer_Window_GetFocusedTaskId(), &message);
*/
    return;
}
