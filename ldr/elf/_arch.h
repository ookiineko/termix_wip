/*
  _arch.h - ELF architectures

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

#ifndef TERMIX_LOADER_ELF_INTERNAL_ARCH_H
#define TERMIX_LOADER_ELF_INTERNAL_ARCH_H

#include "../../inc/arch.h"

#include "_elf.h"

#ifdef TMIX32
#define _ElfXX_Ehdr             Elf32_Ehdr
#define _ElfXX_Phdr             Elf32_Phdr
#define _ElfXX_Dyn              Elf32_Dyn
#define _ElfXX_Sym              Elf32_Sym
#define _ElfXX_Rel              Elf32_Rel
#define _ElfXX_Rela             Elf32_Rela
#define _ElfXX_Word             Elf32_Word

#define _ELFXX_ST_BIND          ELF32_ST_BIND
#define _ELFXX_ST_TYPE          ELF32_ST_TYPE
#define _ELFXX_ST_VISIBILITY    ELF32_ST_VISIBILITY
#elif defined(TMIX64)
#define _ElfXX_Ehdr             Elf64_Ehdr
#define _ElfXX_Phdr             Elf64_Phdr
#define _ElfXX_Dyn              Elf64_Dyn
#define _ElfXX_Sym              Elf64_Sym
#define _ElfXX_Rel              Elf64_Rel
#define _ElfXX_Rela             Elf64_Rela
#define _ElfXX_Word             Elf64_Word

#define _ELFXX_ST_BIND          ELF64_ST_BIND
#define _ELFXX_ST_TYPE          ELF64_ST_TYPE
#define _ELFXX_ST_VISIBILITY    ELF64_ST_VISIBILITY
#else
#error Dont know ELF types on this platform yet
#endif

#endif /* TERMIX_LOADER_ELF_INTERNAL_ARCH_H */
