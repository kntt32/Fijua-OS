#ifndef INCLUDED_EFI_FILE_INFO_H
#define INCLUDED_EFI_FILE_INFO_H

#define EFI_FILE_INFO_ID \
 {0x09576e92,0x6d3f,0x11d2,\
  {0x8e, 0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}


typedef struct {
  UINT64                         Size;
  UINT64                         FileSize;
  UINT64                         PhysicalSize;
  EFI_TIME                       CreateTime;
  EFI_TIME                       LastAccessTime;
  EFI_TIME                       ModificationTime;
  UINT64                         Attribute;
  CHAR16                         FileName [];
} EFI_FILE_INFO;

//******************************************
// File Attribute Bits
//******************************************

#define EFI_FILE_READ_ONLY      0x0000000000000001
#define EFI_FILE_HIDDEN         0x0000000000000002
#define EFI_FILE_SYSTEM         0x0000000000000004
#define EFI_FILE_RESERVED       0x0000000000000008
#define EFI_FILE_DIRECTORY      0x0000000000000010
#define EFI_FILE_ARCHIVE        0x0000000000000020
#define EFI_FILE_VALID_ATTR     0x0000000000000037


#endif
