#ifndef INCLUDED_EFI_TYPES_H
#define INCLUDED_EFI_TYPES_H

#include <types.h>

typedef uint8 BOOLEAN;
typedef sintn INTN;
typedef uintn UINTN;
typedef sint8 INT8;
typedef uint8 UINT8;
typedef sint16 INT16;
typedef uint16 UINT16;
typedef sint32 INT32;
typedef uint32 UINT32;
typedef sint64 INT64;
typedef uint64 UINT64;
typedef sint128 INT128;
typedef uint128 UINT128;
typedef uint8 CHAR8;
typedef uint16 CHAR16;
typedef void VOID;
typedef UINTN EFI_STATUS;
typedef VOID* EFI_HANDLE;
typedef VOID* EFI_EVENT;
typedef UINT64 EFI_LBA;
typedef UINTN EFI_TPL;
typedef uint8 EFI_MAC_ADDRESS[32];
typedef uint8 EFI_IPv4_ADDRESS[4];
typedef uint8 EFI_IPv6_ADDRESS[16];
typedef uint8 EFI_IP_ADDRESS[16];

#define IN
#define OUT
#define OPTIONAL
#define CONST
#define EFIAPI

#endif
