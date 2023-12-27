/*
  load.c - Load ELF image to memory

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
#include <stdlib.h>
#include <sys/mman.h>

#include "elf/elf.h"
#include "load.h"

/*
 * convert internal segment flags to OS-specific protections
 */
static inline int __conv_prot(tmixelf_seg_flag flags) {
    int res = 0;

    if (flags & TMIXELF_SEG_READ)
        res |= PROT_READ;
    if (flags & TMIXELF_SEG_WRITE)
        res |= PROT_WRITE;
    if (flags & TMIXELF_SEG_EXEC)
        res |= PROT_EXEC;

    return res;
}

int tmixldr_load_elf(int fd, const tmixelf_info *ei, tmixldr_elf *e) {
    if (e->base) {
        // seems already loaded
        errno = EBUSY;
        return -1;
    }

    if (!ei->seg.size) {
        // no loadable segement??
        return 0;
    }

    // reserve memory

    void *base = mmap(NULL, ei->mem_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);

    if (base == MAP_FAILED)
        return -1;

#if defined(_WIN32) || defined(__CYGWIN__)
    // Unlike Linux, NT kernel doesn't seems to support overlapped memory mappings
    munmap(base, ei->mem_size);
#endif

    // setup segments

    tmixelf_seg *si = ei->seg.data;  // array

    assert(si && si[0].off == 0);

    int i;

    for (i = 0; i < ei->seg.size; i++) {
        int prot = __conv_prot(si[i].flags);

        if (si[i].file.size &&
            mmap(base + si[i].off, si[i].file.size, prot,
                    MAP_FIXED | MAP_PRIVATE, fd, si[i].file.off) == MAP_FAILED) {
quit:
            munmap(base, ei->mem_size);  // release reserved memory
            return -1;
        }

        if (si[i].pad.size &&
            mmap(base+ si[i].off + si[i].pad.off, si[i].pad.size, prot,
                 MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0) == MAP_FAILED)
            goto quit;
    }

    e->base = base;

    if (ei->entry)
        e->entry = e->base + ei->entry;

    return 0;  // success
}

void tmixldr_unload_elf(tmixldr_elf *e, const tmixelf_info *ei) {
    int i;
    tmixelf_seg *si = ei->seg.data;  // array

    if (!e->base)
        return;  // seems already unloaded

    munmap(e->base, ei->mem_size);
    e->base = NULL;
}
