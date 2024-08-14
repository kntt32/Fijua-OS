#ifndef INCLUDED_FILE_H
#define INCLUDED_FILE_H

typedef struct {
    enum {
        File_Directory,
        File_File
    } type;
    ascii name[32];
    uintn size;
} File_DirectoryEntry;

void File_Init(void);

uintn File_GetDirectory(const ascii path[], uintn* buffCount, File_DirectoryEntry buff[]);

uintn File_GetDirectoryEntryByPath(const ascii path[], File_DirectoryEntry* buff);

uintn File_OpenAndMMapFile(const ascii path[], uintn buffSize, void* buff);

uintn File_WriteFromMem(const ascii path[], uintn buffSize, void* buff);

uintn File_Remove(const ascii path[]);

#endif
