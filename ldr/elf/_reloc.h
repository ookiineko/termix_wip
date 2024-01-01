/*
  _reloc.h - ELF relocation information

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

#ifndef TERMIX_LOADER_ELF_INTERNAL_RELOC_H
#define TERMIX_LOADER_ELF_INTERNAL_RELOC_H

#include <stdbool.h>
#include <sys/types.h>

#include "../../inc/types.h"

/*
 * initialize this struct with zero
 *
 * data stored in the relocs field should be moved to a tmixelf_info
 */
typedef struct {
    const char *strtab;
    size_t symtab_off;
    size_t hashtab_off;
    size_t rel_off;
    size_t rel_size;
    bool rela;
    tmix_array relocs;  // array, optional
} tmixelf_internal_reloc;

/*
 * caller should fill the fields in eir as argument and set the relocs field to zero
 *
 * returns 0 if success, otherwise -1 and sets errno
 *
 * if this function fails, no memory need to be freed, but eir might get modified
 * otherwise the relocs field might be populated, caller should take the ownership of the data inside it
 */
int _tmixelf_internal_parse_reloc(int fd, tmixelf_internal_reloc *eir);

#endif /* TERMIX_LOADER_ELF_INTERNAL_RELOC_H */
