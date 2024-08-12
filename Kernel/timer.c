#include <types.h>
#include <efi.h>
#include <kernel.h>
#include "timer.h"
#include "x64.h"
#include "console.h"
#include "mouse.h"
#include "keyboard.h"

#define TIMER_MAX_NUMBER (10)


void Timer_Wrapper(void*, uintn (*callback)(void));

typedef struct {
    EFI_EVENT eventId;
    uintn isEnabled;
} Timer_TimerTable;

extern KernelInputStruct* KernelInput;

EFI_CREATE_EVENT Efi_CreateEvent = NULL;
EFI_SET_TIMER Efi_SetTimer       = NULL;
EFI_RAISE_TPL Efi_RaiseTPL       = NULL;
EFI_RESTORE_TPL Efi_RestoreTPL   = NULL;
EFI_CLOSE_EVENT Efi_CloseEvent   = NULL;
EFI_SIGNAL_EVENT Efi_SignalEvent = NULL;
EFI_EVENT eventId;



static void Timer_CallBack(void) {/*
    Mouse_CheckState();
    Keyboard_CheckState();
*/
    return;
}


void Timer_Init(void) {
    Console_Print("Timer_Init: Initializing Timer...\n");
    EFI_BOOT_SERVICES* bootServices = KernelInput->LoadedImage->SystemTable->BootServices;

    Efi_CreateEvent = bootServices->CreateEvent;
    Efi_SetTimer    = bootServices->SetTimer;
    Efi_RaiseTPL    = bootServices->RaiseTPL;
    Efi_RestoreTPL  = bootServices->RestoreTPL;
    Efi_CloseEvent  = bootServices->CloseEvent;
    Efi_SignalEvent = bootServices->SignalEvent;

    uintn status;
    status = Efi_Wrapper(Efi_CreateEvent, EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_CALLBACK, (uintn)Timer_Wrapper, (uintn)Timer_CallBack, (uintn)&eventId);
    if(status) {
        Console_Print("Timer_Init: Couldn't create event\n");
        while(1) Hlt();
    }

    status = Efi_Wrapper(Efi_SetTimer, (uintn)eventId, (uintn)TimerPeriodic, 10000);
    if(status) {
        Console_Print("Timer_Init: Couldn't set timer\n");
        while(1) Hlt();
    }

    return;
}
