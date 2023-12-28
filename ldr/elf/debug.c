/*
  debug.c - Debug helpers

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
#include <stdio.h>
#include <sys/types.h>

#include "../../inc/platform.h"

#include "elf.h"

#ifdef TMIX32
#define _PTRFMT                "0x%08lx"
#elif defined(TMIX64)
#define _PTRFMT                "0x%016lx"
#else
#error Dont know word size on this platform yet
#endif

void tmixldr_print_elfinfo(const tmixelf_info *ei) {
    if (!ei)
        return;

    if (ei->entry)
        printf("entrypoint offset (relative): " _PTRFMT "\n", ei->entry);

    printf("total size in memory when loaded: 0x%lx\n", ei->mem_size);

    printf("stack executable: %s\n", ei->execstack ? "yes" : "no");

    printf("loadable segment count: %ld\n", ei->segs.size);

    printf("post-reloc RO segment count: %ld\n", ei->relros.size);

    int i, j;

    if (ei->segs.size) {
        if (ei->relros.size) {
            tmix_chunk *relros = ei->relros.data;

            assert(relros);

            for (i = 0; i < ei->relros.size; i++) {
                printf("relro segment #%d:\n", i);
                printf("  range: " _PTRFMT " to " _PTRFMT "\n", relros[i].off, relros[i].off + relros[i].size);
            }
        }

        tmixelf_seg *si = ei->segs.data;

        assert(si);

        for (i = 0; i < ei->segs.size; i++) {
            printf("loadable segment #%d:\n", i);
            printf("  relative offset: " _PTRFMT "\n", si[i].off);
            if (si[i].file.size)
                printf("  file data size: " _PTRFMT " (at file offset " _PTRFMT ")\n", si[i].file.size, si[i].file.off);
            if (si[i].pad.size)
                printf("  zero padding size: " _PTRFMT " (relative offset " _PTRFMT ")\n", si[i].pad.size, si[i].pad.off);

            if (si[i].flags) {
                printf("  flags: ");
                if (si[i].flags & TMIXELF_SEG_READ)
                    printf("R ");
                if (si[i].flags & TMIXELF_SEG_WRITE)
                    printf("W ");
                if (si[i].flags & TMIXELF_SEG_EXEC)
                    printf("X ");
                printf("\n");
            }
        }
    }

    if (ei->relocs.size) {
        assert(ei->relocs.data);

        // TODO
    }
}
