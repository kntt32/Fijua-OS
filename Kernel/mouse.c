#include <kernel.h>
#include <types.h>
#include <efi.h>
#include <efi_simple_pointer_protocol.h>
#include "mouse.h"
#include "graphic.h"
#include "layer.h"
#include "functions.h"
#include "console.h"
#include "x64.h"

#define Mouse_SupportHardwareCount (10)

extern KernelInputStruct* KernelInput;

static uintn Efi_SimplePointerProtocol_Count = 0;
static EFI_SIMPLE_POINTER_PROTOCOL* Efi_SimplePointerProtocol[Mouse_SupportHardwareCount];

static sintn Mouse_y = 0;
static sintn Mouse_x = 0;
static uintn Mouse_leftButton = 0;
static uintn displayWidth;
static uintn displayHeight;
static const sintn Mouse_speed = 30;

void Mouse_Init(void) {
    uintn status;

    Console_Print("Mouse_Init: Initializing Mouse...\n");

    //Get Efi Protocol Interface
    EFI_GUID guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;

    displayWidth = KernelInput->Graphic.width;
    displayHeight = KernelInput->Graphic.height;

    {
        uintn handleBuffSize = 0;
        status = Efi_Wrapper(KernelInput->LoadedImage->SystemTable->BootServices->LocateHandle, ByProtocol, &guid, NULL, &handleBuffSize, NULL);
        EFI_HANDLE handleBuff[handleBuffSize/sizeof(EFI_HANDLE)];
        handleBuffSize = sizeof(handleBuff);
        status = Efi_Wrapper(KernelInput->LoadedImage->SystemTable->BootServices->LocateHandle, ByProtocol, &guid, NULL, &handleBuffSize, handleBuff);
        if(status || handleBuffSize == 0) {
            Console_Print("Mouse_Init: Mouse handle not found\n");
            while(1) Hlt();
        }

        Efi_SimplePointerProtocol_Count = 0;
        for(uintn i=0; i<(handleBuffSize/sizeof(EFI_HANDLE)); i++) {
            if(Mouse_SupportHardwareCount <= Efi_SimplePointerProtocol_Count) break;
            status = Efi_Wrapper(KernelInput->LoadedImage->SystemTable->BootServices->HandleProtocol, handleBuff[i], &guid, &(Efi_SimplePointerProtocol[Efi_SimplePointerProtocol_Count]));
            if(status == 0) Efi_SimplePointerProtocol_Count++;
        }

        for(sintn i=0; i<(sintn)Efi_SimplePointerProtocol_Count; i++) {
            status = Efi_Wrapper(Efi_SimplePointerProtocol[i]->Reset, Efi_SimplePointerProtocol[i], 0);
            if(status) {
                for(uintn k=i+1; k<Efi_SimplePointerProtocol_Count; k++) {
                    Efi_SimplePointerProtocol[k-1] = Efi_SimplePointerProtocol[k];
                }
                Efi_SimplePointerProtocol_Count--;
                i--;
            }
        }
        if(Efi_SimplePointerProtocol_Count == 0) {
            Console_Print("Mouse_Init: Mouse interface not found\n");
            while(1) Hlt();
        }
    }

    return;
}


void Mouse_CheckState(void) {
    static EFI_SIMPLE_POINTER_STATE Efi_Mouse_State;
    uintn status;
    uintn updateFlag = 0;
    sintn divNumX;
    sintn divNumY;

    Mouse_leftButton = 0;

    for(uintn i=0; i<Efi_SimplePointerProtocol_Count; i++) {
        status = Efi_Wrapper(Efi_SimplePointerProtocol[i]->GetState, Efi_SimplePointerProtocol[i], &Efi_Mouse_State);
        if(status == 0) {
            updateFlag = 1;

            divNumX = Efi_SimplePointerProtocol[i]->Mode->ResolutionX;
            divNumY = Efi_SimplePointerProtocol[i]->Mode->ResolutionY;
            if(divNumX != 0) Mouse_x += (Mouse_speed*Efi_Mouse_State.RelativeMovementX)/divNumX;
            if(divNumY != 0) Mouse_y += (Mouse_speed*Efi_Mouse_State.RelativeMovementY)/divNumY;
            if(Efi_Mouse_State.LeftButton) Mouse_leftButton = 1;
        }
    }
    if(updateFlag) {
        if(Mouse_x < 0) Mouse_x = 0;
        if(Mouse_y < 0) Mouse_y = 0;
        if(displayWidth <= (uintn)Mouse_x) Mouse_x = displayWidth-1;
        if(displayHeight <= (uintn)Mouse_y) Mouse_y = displayHeight-1;
        Layer_Mouse_NotifyUpdate(Mouse_x, Mouse_y, Mouse_leftButton);
    }

    return;
}

