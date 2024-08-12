#ifndef INCLUDED_EFI_SIMPLE_TEXT_INPUT_PROTOCOL_H
#define INCLUDED_EFI_SIMPLE_TEXT_INPUT_PROTOCOL_H

#include <efi.h>

#define EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID \
 {0x387477c1,0x69c7,0x11d2,\
  {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}


#define EFI_SIMPLE_INPUT_SCANCODE_NULL (0x00)
#define EFI_SIMPLE_INPUT_SCANCODE_UPARROW (0x01)
#define EFI_SIMPLE_INPUT_SCANCODE_DOWNARROW (0x02)
#define EFI_SIMPLE_INPUT_SCANCODE_RIGHTARROW (0x03)
#define EFI_SIMPLE_INPUT_SCANCODE_LEFTARROW (0x04)
#define EFI_SIMPLE_INPUT_SCANCODE_HOME (0x05)
#define EFI_SIMPLE_INPUT_SCANCODE_END (0x06)
#define EFI_SIMPLE_INPUT_SCANCODE_INSERT (0x07)
#define EFI_SIMPLE_INPUT_SCANCODE_DELETE (0x08)
#define EFI_SIMPLE_INPUT_SCANCODE_PAGEUP (0x09)
#define EFI_SIMPLE_INPUT_SCANCODE_PAGEDOWN (0x0a)
#define EFI_SIMPLE_INPUT_SCANCODE_ESC (0x17)


//******************************************************
// EFI_INPUT_KEY
//******************************************************
typedef struct {
 UINT16                             ScanCode;
 CHAR16                             UnicodeChar;
} EFI_INPUT_KEY;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;


typedef
EFI_STATUS
(EFIAPI *EFI_INPUT_RESET) (
 IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL                 *This,
 IN BOOLEAN                                        ExtendedVerification
 );

typedef
EFI_STATUS
(EFIAPI *EFI_INPUT_READ_KEY) (
 IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL              *This,
 OUT EFI_INPUT_KEY                              *Key
 );

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
 EFI_INPUT_RESET                       Reset;
 EFI_INPUT_READ_KEY                    ReadKeyStroke;
 EFI_EVENT                             WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

#endif
