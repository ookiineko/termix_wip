/*
  _segs.h - ELF segments

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

#ifndef TERMIX_LOADER_ELF_INTERNAL_SEGS_H
#define TERMIX_LOADER_ELF_INTERNAL_SEGS_H

#include <stdbool.h>
#include <sys/types.h>

#include "../../inc/macros.h"
#include "../../inc/types.h"

#include "_types.h"

/*
 * initialize this struct with zero
 *
 * values stored in this struct should be moved to a tmixelf_info
 */
_tmix_typedef(struct, elf_internal_segs) {
    tmix_array segs;  // data is optional
    tmix_array relros;  // data is optional
    size_t highest_addr;
    bool execstack;
    tmix_array needs;  // data is optional
};
_tmix_typedef_end(struct, elf_internal_segs);

/*
 * returns 0 if success, otherwise -1 and sets errno
 *
 * if this function fails, no memory need to be freed, but eis might get modified
 * otherwise eis might be populated, caller should take the ownership of the data inside it
 */
int _tmixelf_internal_parse_segs(int fd, _ElfXX_Ehdr *hdr, tmixelf_internal_segs *eis);

#endif /* TERMIX_LOADER_ELF_INTERNAL_SEGS_H */
