#ifndef INCLUDED_EFI_FILE_PROTOCOL_H
#define INCLUDED_EFI_FILE_PROTOCOL_H

#include <efi_file_info.h>


//******************************************************
// Open Modes
//******************************************************
#define EFI_FILE_MODE_READ       0x0000000000000001
#define EFI_FILE_MODE_WRITE      0x0000000000000002
#define EFI_FILE_MODE_CREATE     0x8000000000000000

//******************************************************
// File Attributes
//******************************************************
#define EFI_FILE_READ_ONLY       0x0000000000000001
#define EFI_FILE_HIDDEN          0x0000000000000002
#define EFI_FILE_SYSTEM          0x0000000000000004
#define EFI_FILE_RESERVED        0x0000000000000008
#define EFI_FILE_DIRECTORY       0x0000000000000010
#define EFI_FILE_ARCHIVE         0x0000000000000020
#define EFI_FILE_VALID_ATTR      0x0000000000000037


typedef struct {
  EFI_EVENT                         Event;
  EFI_STATUS                        Status;
  UINTN                             BufferSize;
  VOID                              *Buffer;
  } EFI_FILE_IO_TOKEN;

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_OPEN) (
  IN EFI_FILE_PROTOCOL                  *This,
  OUT EFI_FILE_PROTOCOL                 **NewHandle,
  IN CHAR16                             *FileName,
  IN UINT64                             OpenMode,
  IN UINT64                             Attributes
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_CLOSE) (
  IN EFI_FILE_PROTOCOL                     *This
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_DELETE) (
  IN EFI_FILE_PROTOCOL                     *This
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_READ) (
  IN EFI_FILE_PROTOCOL           *This,
  IN OUT UINTN                   *BufferSize,
  OUT VOID                       *Buffer
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_WRITE) (
  IN EFI_FILE_PROTOCOL              *This,
  IN OUT UINTN                      *BufferSize,
  IN VOID                           *Buffer
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_OPEN_EX) (
IN EFI_FILE_PROTOCOL                  *This,
OUT EFI_FILE_PROTOCOL                 **NewHandle,
IN CHAR16                             *FileName,
IN UINT64                             OpenMode,
IN UINT64                             Attributes,
IN OUT EFI_FILE_IO_TOKEN              *Token
);


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_READ_EX) (
  IN EFI_FILE_PROTOCOL                      *This,
  IN OUT EFI_FILE_IO_TOKEN                  *Token
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_WRITE_EX) (
  IN EFI_FILE_PROTOCOL                *This,
  IN OUT EFI_FILE_IO_TOKEN            *Token
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_FLUSH_EX) (
  IN EFI_FILE_PROTOCOL                       *This,
  IN OUT EFI_FILE_IO_TOKEN                   *Token
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_POSITION) (
   IN EFI_FILE_PROTOCOL      *This,
   IN UINT64                 Position
   );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_POSITION) (
  IN EFI_FILE_PROTOCOL                *This,
  OUT UINT64                          *Position
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_GET_INFO) (
  IN EFI_FILE_PROTOCOL             *This,
  IN EFI_GUID                      *InformationType,
  IN OUT UINTN                     *BufferSize,
  OUT VOID                         *Buffer
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_SET_INFO) (
  IN EFI_FILE_PROTOCOL                *This,
  IN EFI_GUID                         *InformationType,
  IN UINTN                            BufferSize,
  IN VOID                             *Buffer
  );


typedef
EFI_STATUS
(EFIAPI *EFI_FILE_FLUSH) (
  IN EFI_FILE_PROTOCOL             *This
  );




typedef struct _EFI_FILE_PROTOCOL {
  UINT64                          Revision;
  EFI_FILE_OPEN                   Open;
  EFI_FILE_CLOSE                  Close;
  EFI_FILE_DELETE                 Delete;
  EFI_FILE_READ                   Read;
  EFI_FILE_WRITE                  Write;
  EFI_FILE_GET_POSITION           GetPosition;
  EFI_FILE_SET_POSITION           SetPosition;
  EFI_FILE_GET_INFO               GetInfo;
  EFI_FILE_SET_INFO               SetInfo;
  EFI_FILE_FLUSH                  Flush;
  EFI_FILE_OPEN_EX                OpenEx; // Added for revision 2
  EFI_FILE_READ_EX                ReadEx; // Added for revision 2
  EFI_FILE_WRITE_EX               WriteEx; // Added for revision 2
  EFI_FILE_FLUSH_EX               FlushEx; // Added for revision 2
} EFI_FILE_PROTOCOL;

#endif