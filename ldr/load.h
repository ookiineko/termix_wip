/*
  load.h - Load ELF image to memory

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

#ifndef TERMIX_LOADER_LOAD_H
#define TERMIX_LOADER_LOAD_H

#include "../inc/abi.h"

#include "elf/elf.h"

#ifdef __clangd__
   // for making IDE happy
#  define _tmixldr_api
#else
#  ifdef TMIX_BUILDING_LOADER_SHLIB
#    define _tmixldr_api      __tmixapi_export
#  else
#    define _tmixldr_api      __tmixapi_import
#  endif
#endif

/*
 * information about a loaded ELF in memory
 *
 * initialize this struct with zero
 */
typedef struct {
    void *base;  // the address of the first segment
    __tmixabi void (*entry)(void);  // ELF entrypoint function pointer
} tmixldr_elf;

/*
 * fd - read-only file descriptor referencing and opened ELF file
 * ei - buffer holding information about the previously parsed ELF file
 * e - output buffer
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 *
 * NOTE: if the function failed, no ELF data is mapped to memory
 */
_tmixldr_api int tmixldr_load_elf(int fd, const tmixelf_info *ei, tmixldr_elf *e);

/*
 * e - information about the loaded ELF
 * ei - the ELF header information which used for loading previously
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 */
_tmixldr_api void tmixldr_unload_elf(tmixldr_elf *e, const tmixelf_info *ei);

#endif /* TERMIX_LOADER_LOAD_H */
