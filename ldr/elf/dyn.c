/*
  dyn.c - ELF dynamic segment

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

#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>

#include <gelf.h>

#include "../../inc/logging.h"

#include "_types.h"

#include "_dyn.h"

#define _DYN_TAKE_PTR(_dyn)       (_dyn.d_un.d_ptr)
#define _DYN_TAKE_VAL(_dyn)       (_dyn.d_un.d_val)

int _tmixelf_internal_parse_dyn(int fd, const _ElfXX_Phdr *phdr) {
    if (lseek(fd, phdr->p_offset, SEEK_SET) < 0)
        return -1;

    // iterate through all entries

    _ElfXX_Dyn dyn;

    size_t strtab_off = 0;
    size_t strtab_size = 0;
    size_t symtab_off = 0;
    size_t rel_off = 0;
    size_t rel_size = 0;
    bool rela = false;

    for (;;) {
        if (read(fd, &dyn, sizeof(_ElfXX_Dyn)) != sizeof(_ElfXX_Dyn))
            return -1;

        switch (dyn.d_tag) {
            case DT_NULL:
                // end of table, handled below
                break;
            case DT_NEEDED:
                // TODO: put this info into actual use when needed
                break;
            case DT_RUNPATH:
                // rpath, currently unused by us
                break;
            case DT_GNU_HASH:
                // TODO: put this info into actual use when needed
                break;
            case DT_STRTAB:
                strtab_off = _DYN_TAKE_PTR(dyn);
                break;
            case DT_STRSZ:
                strtab_size = _DYN_TAKE_VAL(dyn);
                break;
            case DT_SYMENT:
                assert(_DYN_TAKE_VAL(dyn) == sizeof(_ElfXX_Sym));
                break;
            case DT_SYMTAB:
                strtab_off = _DYN_TAKE_PTR(dyn);
                break;
            case DT_PLTGOT:
                // unused by us for now
                break;
            case DT_PLTRELSZ:
                rel_size = _DYN_TAKE_VAL(dyn);
                break;
            case DT_PLTREL:
                rela = _DYN_TAKE_VAL(dyn) == DT_RELA;
                break;
            case DT_JMPREL:
                // location of relocation entries
                rel_off = _DYN_TAKE_PTR(dyn);
                break;
            case DT_FLAGS_1:
                switch (_DYN_TAKE_VAL(dyn)) {
                    case DF_1_PIE:
                        // already assumed, ignore
                        break;
                    default:
                        tmix_fixme("unhandled state flag 0x%lx", _DYN_TAKE_VAL(dyn));
                        break;
                }
                break;
            case DT_DEBUG:
                // placeholder for runtime debug info, ignored
                break;
            default:
                tmix_fixme("unhandled dynamic tag 0x%lx", dyn.d_tag);
                break;
        } /* switch (dyn.d_tag) */

        if (dyn.d_tag == DT_NULL)
            break;  // end of table, stop
    } /* for (;;) */

    // TODO: iterate through relocation table

    return 0;
}
