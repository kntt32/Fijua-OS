#ifndef INCLUDED_FAT32_H
#define INCLUDED_FAT32_H

/*
https://wiki.osdev.org/FAT
*/

typedef struct {
//Bios Parametor Block
    uint32 bpb_asmJmpOvrBPB : 3*8;
    uint64 bpb_oemIdentifier : 8*8;
    uint16 bpb_bytesPersector : 2*8;
    uint8 bpb_sectorsPercluster : 1*8;
    uint16 bpb_reservedSectors : 2*8;
    uint8 bpb_fats: 1*8;
    uint16 bpb_rootDirectoryEnts : 2*8;
    uint16 bpb_totalSectors : 2*8;
    uint8 bpb_mediaDescriptorType: 1*8;
    uint16 _bpb_sectorsPerFat : 2*8;
    uint16 bpb_sectorsPerTrack : 2*8;
    uint16 bpb_headsSidsCount : 2*8;
    uint32 bpb_hiddenSectors : 4*8;
    uint32 bpb_largeSectorCount : 4*8;

//Extended Boot record
    uint8 ebpb_driveIdNumber : 1*8;
    uint8 ebpb_winNtFlag : 1*8;
    uint8 ebpb_signature: 1*8;
    //...
} Fat32_Header;


#endif
