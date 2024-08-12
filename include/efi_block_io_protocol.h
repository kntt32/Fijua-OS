#ifndef INCLUDED_EFI_BLOCK_IO_PROTOCOL_H
#define INCLUDED_EFI_BLOCK_IO_PROTOCOL_H

#define EFI_BLOCK_IO_PROTOCOL_GUID \
 {0x964e5b21,0x6459,0x11d2,\
  {0x8e,0x39,0x00,0xa0,0xc9,0x69,0x72,0x3b}}

typedef struct _EFI_BLOCK_IO_PROTOCOL EFI_BLOCK_IO_PROTOCOL;


//*******************************************************
// EFI_BLOCK_IO_MEDIA
//*******************************************************

typedef struct {
  UINT32                    MediaId;
  BOOLEAN                   RemovableMedia;
  BOOLEAN                   MediaPresent;
  BOOLEAN                   LogicalPartition;
  BOOLEAN                   ReadOnly;
  BOOLEAN                   WriteCaching;
  UINT32                    BlockSize;
  UINT32                    IoAlign;
  EFI_LBA                   LastBlock;

   EFI_LBA                 LowestAlignedLba; //added in Revision 2
   UINT32                  LogicalBlocksPerPhysicalBlock;
//added in Revision 2
UINT32 OptimalTransferLengthGranularity;
// added in Revision 3
} EFI_BLOCK_IO_MEDIA;

//*******************************************************
// EFI_LBA
//*******************************************************
typedef UINT64 EFI_LBA;


typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_RESET) (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN BOOLEAN                  ExtendedVerification
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_READ) (
  IN EFI_BLOCK_IO_PROTOCOL    *This,
  IN UINT32                   MediaId,
  IN EFI_LBA                  LBA,
  IN UINTN                    BufferSize,
  OUT VOID                    *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_WRITE) (
  IN EFI_BLOCK_IO_PROTOCOL       *This,
  IN UINT32                      MediaId,
  IN EFI_LBA                     LBA,
  IN UINTN                       BufferSize,
  IN VOID                        *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_BLOCK_FLUSH) (
  IN EFI_BLOCK_IO_PROTOCOL    *This
  );


typedef struct _EFI_BLOCK_IO_PROTOCOL {
  UINT64                         Revision;
  EFI_BLOCK_IO_MEDIA             *Media;
  EFI_BLOCK_RESET                Reset;
  EFI_BLOCK_READ                 ReadBlocks;
  EFI_BLOCK_WRITE                WriteBlocks;
  EFI_BLOCK_FLUSH                FlushBlocks;
} EFI_BLOCK_IO_PROTOCOL;


#endif
