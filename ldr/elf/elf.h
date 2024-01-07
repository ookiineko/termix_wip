/*
  elf.h - ELF parsing library

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

#ifndef TERMIX_LOADER_ELF_H
#define TERMIX_LOADER_ELF_H

#include <stdbool.h>
#include <sys/types.h>

#include "../../inc/abi.h"
#include "../../inc/types.h"

#ifdef __clangd__
   // for making IDE happy
#  define _tmixlibelf_api
#else
#  ifdef TMIX_BUILDING_LIBELF_SHLIB
#    define _tmixlibelf_api      __tmixapi_export
#  else
#    define _tmixlibelf_api      __tmixapi_import
#  endif
#endif

/*
 * ELF segment protection flags
 */
typedef enum {
    TMIXELF_SEG_READ = 1 << 0,  // segment is readable
    TMIXELF_SEG_WRITE = 1 << 1,  // segment is writable
    TMIXELF_SEG_EXEC = 1 << 2  // segment is executable (requires READ as well)
} tmixelf_seg_flag;

/*
 * ELF segment info
 */
typedef struct {
    size_t off;  // offset relative to the first segment
    tmix_chunk file;  // file offset and size of reference data
    tmix_chunk pad;  /* size and offset relative to the start of this segment for zero paddings
                    　　 empty if no explicit zero padding required */
    tmixelf_seg_flag flags;  // protection flags for this segment
} tmixelf_seg;

/*
 * ELF symbol type
 */
typedef enum {
    TMIXELF_SYM_DATA = 0,  // symbol is data (variables)
    TMIXELF_SYM_FUNC = 1  // symbol is a function
} tmixelf_sym_type;

/*
 * ELF symbol
 *
 * name - dynamically allocated, need to be freed once this struct is not used
 */
typedef struct {
    char *name;
    tmixelf_sym_type type;
    bool imported;
    size_t off;  // location of the symbol, ignored if the symbol is imported
} tmixelf_sym;

/*
 * ELF relocation entry
 */
typedef struct {
    size_t symidx;  // index of the relocated symbol in symbol table
    size_t off;  // location to the where the address to the symbol is stored
} tmixelf_reloc;

/*
 * describes information of an ELF file
 *
 * initialize this struct with zero
 */
typedef struct {
    size_t entry;  // entrypoint address (relative to the first segment)
    tmix_array segs;  // array of segment informations (i.e. tmixelf_seg)
    size_t mem_size;  // sum of sizes of all loadable semgents
    tmix_array syms;  // array of symbols from the ELF symbol table (i.e. tmixelf_sym)
    bool execstack;  // whether if has an executable stack
    tmix_array relros;  /* array of segments that require changing memory protection to
                           read-only after dynamic linking, each element storing tmix_chunk */
    tmix_array needs;  // list of depended shared library names
    tmix_array relocs;  // list of relocation entries (i.e. tmixelf_reloc)
} tmixelf_info;

/*
 * fd - read-only file descriptor referencing and opened ELF file
 * ei - output buffer
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 *
 * on success, the old content in ei is cleared,
 * if the function fails, the old content in ei is unchanged.
 *
 * caller should call free_elfinfo once the returned information is no longer used
 */
_tmixlibelf_api int tmixelf_parse_info(int fd, tmixelf_info *ei);

/*
 * to print out the information in an elfinfo buffer
 *
 * for debugging purposes only
 */
_tmixlibelf_api void tmixelf_print_info(const tmixelf_info *ei);

/*
 * ei - buffer to free
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 */
_tmixlibelf_api void tmixelf_free_info(tmixelf_info *ei);

#endif /* TERMIX_LOADER_ELF_H */
