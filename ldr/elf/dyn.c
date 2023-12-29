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
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <gelf.h>

#include "../../inc/logging.h"

#include "_types.h"

#include "_dyn.h"

#define _DYN_TAKE_PTR(_dyn)       ((_dyn).d_un.d_ptr)
#define _DYN_TAKE_VAL(_dyn)       ((_dyn).d_un.d_val)

int _tmixelf_internal_parse_dyn(int fd, const _ElfXX_Phdr *phdr, tmixelf_internal_dyn *eid) {
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

    size_t dyn_ent_count = 0;
    size_t needed_shlib_count = 0;

    for (;;dyn_ent_count++) {
        if (read(fd, &dyn, sizeof(_ElfXX_Dyn)) != sizeof(_ElfXX_Dyn)) {
            errno = EIO;
            return -1;
        }

        switch (dyn.d_tag) {
            case DT_NULL:
                // end of table, handled below
                break;
            case DT_NEEDED:
                needed_shlib_count++;  // for calculating the array size
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
                symtab_off = _DYN_TAKE_PTR(dyn);
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
                        tmix_fixme("unhandled state flag %#lx", _DYN_TAKE_VAL(dyn));
                        break;
                }
                break;
            case DT_DEBUG:
                // placeholder for runtime debug info, ignored
                break;
            default:
                tmix_fixme("unhandled dynamic tag %#lx", dyn.d_tag);
                break;
        } /* switch (dyn.d_tag) */

        if (dyn.d_tag == DT_NULL)
            break;  // end of table, stop
    } /* for (;;) */

    // now let's finish up our todos

    char *strtab = NULL;  // optional
    _ElfXX_Dyn *dyns = NULL;  // array, optional
    char **needs = NULL;  // array, optional

    // prepare string tab is needed

    if (strtab_size) {
        if (!(strtab = malloc(strtab_size)))
            return -1;

        assert(strtab_off);

        if (lseek(fd, strtab_off, SEEK_SET) < 0)
            goto error;

        if (read(fd, strtab, strtab_size) != (ssize_t)strtab_size)
            goto read_failed;
    }

    // prepare dynamic entries

    if (dyn_ent_count) {
        if (lseek(fd, phdr->p_offset, SEEK_SET) < 0)
            goto error;

        if (!(dyns = calloc(dyn_ent_count, sizeof(_ElfXX_Dyn))))
            goto error;

        if (read(fd, dyns, sizeof(_ElfXX_Dyn) * dyn_ent_count) != (ssize_t)(sizeof(_ElfXX_Dyn) * dyn_ent_count)) {
read_failed:
            errno = EIO;
error:
            if (strtab)
                free(strtab);

            if (dyns)
                free(dyns);

            if (needs)
                free(needs);

            return -1;
        }
    }

    // handle needed shlibs if needed

    if (needed_shlib_count) {
        // at least one needed shlib is found

        assert(dyn_ent_count);
        assert(dyns);
        assert(strtab);

        if (!(needs = calloc(needed_shlib_count, sizeof(char *))))
            goto error;

        memset(needs, 0, needed_shlib_count * sizeof(char *));

        size_t i;
        size_t j = 0;  // index of current needed shlib

        // iterate through all entries again

        for (i = 0; i < dyn_ent_count; i++) {
            _ElfXX_Dyn *dyn = &dyns[i];  // current dynamic entry

            if (dyn->d_tag == DT_NEEDED) {
                // find location and size of shlib name in the strtab

                size_t name_off = _DYN_TAKE_VAL(dyns[i]);
                assert(name_off);

                char *name = &strtab[name_off];
                size_t name_len = strlen(name);
                assert(name_len);

                // copy it to array
                char *name_buff = malloc(name_len);

                if (!name_buff) {
                    // free up messes before return
                    for (i = 0; i < j; i++)
                        free(needs[i]);

                    goto error;
                }

                strcpy(name_buff, name);
                needs[j++] = name_buff;
            }
        }

        // populate info
        eid->needs.data = needs;
        eid->needs.size = needed_shlib_count;
    }

    // TODO: iterate through relocation table
    (void)symtab_off;
    (void)rel_off;
    (void)rel_size;
    (void)rela;

    // finally...
    if (strtab)
        free(strtab);

    if (dyns)
        free(dyns);

    return 0;
}
