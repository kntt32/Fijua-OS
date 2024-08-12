#ifndef INCLUDED_EFI_SIMPLE_POINTER_PROTOCOL_H
#define INCLUDED_EFI_SIMPLE_POINTER_PROTOCOL_H

#include <efi.h>

#define EFI_SIMPLE_POINTER_PROTOCOL_GUID \
 {0x31878c87,0xb75,0x11d5,\
  {0x9a,0x4f,0x00,0x90,0x27,0x3f,0xc1,0x4d}}

typedef struct _EFI_SIMPLE_POINTER_PROTOCOL EFI_SIMPLE_POINTER_PROTOCOL;

//******************************************************
// EFI_SIMPLE_POINTER_MODE
//******************************************************
typedef struct {
 UINT64                    ResolutionX;
 UINT64                    ResolutionY;
 UINT64                    ResolutionZ;
 BOOLEAN                   LeftButton;
 BOOLEAN                   RightButton;
} EFI_SIMPLE_POINTER_MODE;

//******************************************************
// EFI_SIMPLE_POINTER_STATE
//******************************************************
typedef struct {
 INT32                  RelativeMovementX;
 INT32                  RelativeMovementY;
 INT32                  RelativeMovementZ;
 BOOLEAN                LeftButton;
 BOOLEAN                RightButton;
} EFI_SIMPLE_POINTER_STATE;


typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_POINTER_RESET) (
 IN EFI_SIMPLE_POINTER_PROTOCOL              *This,
 IN BOOLEAN                                  ExtendedVerification
 );


typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_POINTER_GET_STATE) (
 IN EFI_SIMPLE_POINTER_PROTOCOL              *This,
 OUT EFI_SIMPLE_POINTER_STATE                *State
 );


typedef struct _EFI_SIMPLE_POINTER_PROTOCOL {
 EFI_SIMPLE_POINTER_RESET                    Reset;
 EFI_SIMPLE_POINTER_GET_STATE                GetState;
 EFI_EVENT                                   WaitForInput;
 EFI_SIMPLE_POINTER_MODE                       *Mode;
} EFI_SIMPLE_POINTER_PROTOCOL;

#endif
