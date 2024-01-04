/*
  _elf.h - minimal system elf.h alternative

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TERMIX_LOADER_ELF_INTERNAL_ELF_H
#define TERMIX_LOADER_ELF_INTERNAL_ELF_H

#include <stdint.h>

/*
 * file magic
 */
#define ELFMAG              "\177ELF"
// size
#define SELFMAG             (4)
// size of identification
#define EI_NIDENT           (16)

/*
 * identification fields
 */
// word size
#define EI_CLASS            (4)
#define ELFCLASS32          (1)
#define ELFCLASS64          (2)
// bit order
#define EI_DATA             (5)
#define ELFDATA2LSB         (1)
#define ELFDATA2MSB         (2)
// spec version
#define EI_VERSION          (6)
#define EV_CURRENT          (1)
// ABIs
#define EI_OSABI            (7)
#define EI_ABIVERSION       (8)
#define ELFOSABI_SYSV       (0)
#define ELFOSABI_GNU        (3)

/*
 * ELF header related values
 */
// ELF file type
#define ET_DYN              (3)
// machine architecture
#define EM_386              (3)
#define EM_ARM              (40)
#define EM_X86_64           (62)
#define EM_AARCH64          (183)

/*
 * segment types
 */
// loadable
#define PT_LOAD             (1)
// dynamic linking information
#define PT_DYNAMIC          (2)
// path to dynmaic linker, unused by us
#define PT_INTERP	    (3)
// extra inforamtion, ignored by us
#define PT_NOTE             (4)
// entry used for storing segment header table itself, unused by us
#define PT_PHDR             (6)
// GNU extension for stack information
#define PT_GNU_STACK	    (0x6474e551)
// information for post-relocation read-only segments behavior
#define PT_GNU_RELRO        (0x6474e552)

/*
 * memory protection flags for loadable segments
 */
// segment is executable
#define PF_X                (1 << 0)
// segment is writable
#define PF_W                (1 << 1)
// segment is readable
#define PF_R                (1 << 2)

/*
 * dynamic entry type
 */
// end of dynamic entry table
#define DT_NULL             (0)
// required shared library
#define DT_NEEDED           (1)
// size of each relocation entry
#define DT_PLTRELSZ         (2)
// address of procedure linking table or global offset table, unused by us
#define DT_PLTGOT           (3)
// address of string table
#define DT_STRTAB           (5)
// address of symbol table
#define DT_SYMTAB           (6)
// string table size
#define DT_STRSZ            (10)
// size of each symbol table entry
#define DT_SYMENT           (11)
// relocation type
#define DT_PLTREL           (20)
// placeholder for runtime debug inforatmion, unused by us
#define DT_DEBUG            (21)
// address of relocation entry table
#define DT_JMPREL           (23)
// library search path
#define DT_RUNPATH          (29)
// GNU-style hash table
#define DT_GNU_HASH         (0x6ffffef5)
// flags
#define DT_FLAGS_1          (0x6ffffffb)

/*
 * dynamic flags
 */
#define DF_1_PIE            (0x8000000)

/*
 * relocation types
 */
// relocation entry is Rela
#define DT_RELA             (7)
// relocation entry is Rel
#define DT_REL              (17)

/*
 * data types
 */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

typedef uint32_t Elf32_Word;
typedef uint32_t Elf64_Word;

// signed word
typedef int32_t Elf32_Sword;
typedef int32_t Elf64_Sword;

typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

// extended word
typedef uint64_t Elf32_Xword;
typedef uint64_t Elf64_Xword;

// signed extended word
typedef int64_t Elf32_Sxword;
typedef int64_t Elf64_Sxword;

// section index, unused by us
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

/*
 * ELF header
 */
typedef struct {
    unsigned char e_ident[EI_NIDENT];  // magic and basic info
    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;
    Elf32_Addr e_entry;
    Elf32_Off e_phoff;  // segment header table offset
    Elf32_Off e_shoff;  // section header table offset, unused by us
    Elf32_Word e_flags;
    Elf32_Half e_ehsize;  // ELF header size
    Elf32_Half e_phentsize;  // size of each segment header
    Elf32_Half e_phnum;  // number of segment headers
    Elf32_Half e_shentsize;  // size of each section header
    Elf32_Half e_shnum;  // number of section headers
    Elf32_Half e_shstrndx;  // section string table index, unused by us
} Elf32_Ehdr;

typedef struct {
    unsigned char e_ident[EI_NIDENT];  // magic and basic info
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;  // segment header table offset
    Elf64_Off e_shoff;  // section header table offset, unused by us
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;  // ELF header size
    Elf64_Half e_phentsize;  // size of each segment header
    Elf64_Half e_phnum;  // number of segment headers
    Elf64_Half e_shentsize;  // size of each section header
    Elf64_Half e_shnum;  // number of section headers
    Elf64_Half e_shstrndx;  // section string table index, unused by us
} Elf64_Ehdr;

/*
 * segment header
 */
typedef struct {
    Elf32_Word p_type;
    Elf32_Off p_offset;  // file offset
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;  // physical address, unused by us
    Elf32_Word p_filesz;
    Elf32_Word p_memsz;
    Elf32_Word p_flags;
    Elf32_Word p_align;
} Elf32_Phdr;

typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;  // file offset
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;  // physical address, unused by us
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

/*
 * dynamic entry
 */
typedef struct {
    Elf32_Sword d_tag;  // type
    union {
        Elf32_Word d_val;
        Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;

typedef struct {
    Elf64_Sxword d_tag;  // type
    union {
        Elf64_Xword d_val;
        Elf64_Addr d_ptr;
    } d_un;
} Elf64_Dyn;

/*
 * symbol table entry
 */
typedef struct{
    Elf32_Word st_name;  // string table offset of the name of this symbol
    Elf32_Addr st_value;  // offset of the location of symbol
    Elf32_Word st_size;  // size of symbol
    unsigned char st_info;  // type and binding
    unsigned char st_other;  // visibilities
    Elf32_Section st_shndx;  // section index, unused by us
} Elf32_Sym;

typedef struct {
    Elf64_Word st_name;  // string table offset of the name of this symbol
    unsigned char st_info;  // type and binding
    unsigned char st_other;  // visibilities
    Elf64_Section st_shndx;  // section index, unused by us
    Elf64_Addr st_value;  // offset of the location of symbol
    Elf64_Xword st_size;  // size of symbol
} Elf64_Sym;

#endif /* TERMIX_LOADER_ELF_INTERNAL_ELF_H */
