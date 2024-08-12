#ifndef INCLUDED_ELF_H
#define INCLUDED_ELF_H

#include <types.h>

#define EI_NIDENT 16

#define Elf_Header_ET_EXEC 2
#define Elf_Header_ET_DYN  3
#define Elf_Header_EM_AMD64 62

#define Elf_ProgramHeader_PT_LOAD 1

#define Elf_SectionHeader_PROGBITS 1
#define Elf_SectionHeader_SYMTAB 2
#define Elf_SectionHeader_STRTAB 3
#define Elf_SectionHeader_RELA 4
#define Elf_SectionHeader_REL 9

#define Elf_RelRela_Sym(info) ((info) >> 32)
#define Elf_RelRela_Type(info) (info & 0xffffffff)
#define Elf_RelRela_Info(sym, type) ((((uint64)sym) << 32) + type)

#define Elf_RelRela_Type_NONE 0
#define Elf_RelRela_Type_64 1
#define Elf_RelRela_Type_PC32 2
#define Elf_RelRela_Type_GLOB_DAT 6
#define Elf_RelRela_Type_JUMP_SLOT 7
#define Elf_RelRela_Type_RELATIVE 8
#define Elf_RelRela_Type_32 10
#define Elf_RelRela_Type_32S 11
#define Elf_RelRela_Type_16 12
#define Elf_RelRela_Type_PC16 13
#define Elf_RelRela_Type_8 14
#define Elf_RelRela_Type_PC8 15
#define Elf_RelRela_Type_PC64 24
#define Elf_RelRela_Type_GOTOFF64 25
#define Elf_RelRela_Type_GOTPC32 26
#define Elf_RelRela_Type_SIZE32 32
#define Elf_RelRela_Type_SIZE64 33



typedef struct {
    unsigned char   e_ident[EI_NIDENT]; 
    uint16      e_type;
    uint16      e_machine;
    uint32      e_version;
    uintn      e_entry;
    uintn       e_phoffset;
    uintn       e_shoffset;
    uint32      e_flags;
    uint16      e_ehsize;
    uint16      e_phentsize;
    uint16      e_phnum;
    uint16      e_shentsize;
    uint16      e_shnum;
    uint16      e_shstrndx;
} Elf_Header;


typedef struct {
        uint32      p_type;
        uint32      p_flags;
        uintn       p_offset;
        uintn      p_vaddr;
        uintn      p_paddr;
        uint64     p_filesz;
        uint64     p_memsz;
        uint64     p_align;
} Elf_ProgramHeader;


typedef struct {
        uint32      sh_name;
        uint32      sh_type;
        uint64     sh_flags;
        uint64      sh_addr;
        uint64       sh_offset;
        uint64     sh_size;
        uint32      sh_link;
        uint32      sh_info;
        uint64     sh_addralign;
        uint64     sh_entsize;
} Elf_SectionHeader;


typedef struct {
        uint32      st_name;
        uint8   st_info;
        uint8   st_other;
        uint16      st_shndx;
        uint64      st_value;
        uint64     st_size;
} Elf_Section_SymbolTable;


typedef struct {
        uint64      r_offset;
        uint64     r_info;
} Elf_Section_Rel;

 
typedef struct {
        uint64      r_offset;
        uint64     r_info;
        sint64    r_addend;
} Elf_Section_Rela;

#endif
