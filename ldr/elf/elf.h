/*
  elf.h - ELF library

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

#include "../../inc/macros.h"
#include "../../inc/types.h"

/*
 * ELF segment protection flags
 */
_tmix_typedef(enum, elf_seg_flag) {
    TMIXELF_SEG_READ = 1 << 0,  // segment is readable
    TMIXELF_SEG_WRITE = 1 << 1,  // segment is writable
    TMIXELF_SEG_EXEC = 1 << 2  // segment is executable (requires READ as well)
};
_tmix_typedef_end(enum, elf_seg_flag);

/*
 * ELF segment info
 */
_tmix_typedef(struct, elf_seg) {
    size_t off;  // offset relative to the first segment
    tmix_chunk file;  // file offset and size of reference data
    tmix_chunk pad;  // size and offset relative to the start of this segment for zero paddings
                     // empty if no explicit zero padding required
    tmixelf_seg_flag flags;  // protection flags for this segment
};
_tmix_typedef_end(struct, elf_seg);

/*
 * ELF symbol type
 */
_tmix_typedef(enum, elf_sym_type) {
    TMIXELF_SYM_DATA = 0,  // symbol is data (variables)
    TMIXELF_SYM_FUNC = 1  // symbol is a function
};
_tmix_typedef_end(enum, elf_sym_type);

/*
 * ELF symbol
 *
 * name - dynamically allocated, need to be freed once this struct is not used
 */
_tmix_typedef(struct, elf_sym) {
    char *name;
    tmixelf_sym_type type;
};
_tmix_typedef_end(struct, elf_sym);

/*
 * ELF relocation info
 */
_tmix_typedef(struct, elf_reloc) {
    tmixelf_sym sym;  // information about the relocated symbol
    size_t off;  // memory location storing the address of this symbol (relative to the first segment)
};
_tmix_typedef_end(struct, elf_reloc);

/*
 * describes information of an ELF file
 *
 * initialize this struct with zero
 */
_tmix_typedef(struct, elf_info) {
    size_t entry;  // entrypoint address (relative to the first segment)
    tmix_array segs;  // array of segment informations (i.e. tmixelf_seg)
    size_t mem_size;  // sum of sizes of all loadable semgents
    tmix_array relocs;  // array of relocation informations (i.e. tmixelf_reloc)
    bool execstack;  // whether if has an executable stack
    tmix_array relros;  // array of segments that require changing memory protection to
                       // read-only after dynamic linking, each element storing tmix_chunk
    tmix_array needs;  // list of depended shared library names
};
_tmix_typedef_end(struct, elf_info);

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
int tmixldr_parse_elf(int fd, tmixelf_info *ei);

/*
 * to print out the information in an elfinfo buffer
 *
 * for debugging purposes only
 */
void tmixldr_print_elfinfo(const tmixelf_info *ei);

/*
 * ei - buffer to free
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 */
void tmixldr_free_elfinfo(tmixelf_info *ei);

#endif /* TERMIX_LOADER_ELF_H */
