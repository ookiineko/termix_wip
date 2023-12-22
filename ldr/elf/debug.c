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
#define __PTRFMT                "0x%08lx"
#elif defined(TMIX64)
#define __PTRFMT                "0x%016lx"
#else
#error Dont know word size on this platform yet
#endif

void tmixldr_print_elfinfo(const tmixelf_info *ei) {
    if (!ei)
        return;

    if (ei->entry)
        printf("entrypoint offset (relative): " __PTRFMT "\n", ei->entry);

    printf("total size in memory when loaded: 0x%lx\n", ei->mem_size);

    printf("stack executable: %s\n", ei->execstack ? "yes" : "no");

    printf("relro segment count: %ld\n", ei->relro.size);

    printf("loadable segment count: %ld\n", ei->seg.size);

    tmix_chunk *relros = ei->relro.data;

    if (ei->relro.size)
        assert(relros);

    int i, j;

    if (ei->seg.size) {
        tmixelf_seg *si = ei->seg.data;

        assert(si);

        for (i = 0; i < ei->seg.size; i++) {
            printf("loadable segment #%d:\n", i);
            printf("  relative offset: " __PTRFMT "\n", si[i].off);
            if (si[i].file.size)
                printf("  file data size: " __PTRFMT " (at file offset " __PTRFMT ")\n", si[i].file.size, si[i].file.off);
            if (si[i].pad.size)
                printf("  zero padding size: " __PTRFMT " (relative offset " __PTRFMT ")\n", si[i].pad.size, si[i].pad.off);

            size_t max_addr = si[i].off;

            if (si[i].file.size) {
                if (si[i].pad.size)
                    max_addr += si[i].pad.off + si[i].pad.size;
                else
                    max_addr += si[i].file.size;
            } else
                max_addr += si[i].pad.size;

            bool relro = false;

            for (j = 0; j < ei->relro.size; j++) {
                size_t relro_end = relros[j].off + relros[j].size;

                if (si[i].off <= relros[j].off &&
                        relros[j].off < max_addr &&
                        relro_end <= max_addr &&
                        si[i].off < relro_end) {
                    relro = true;
                    break;
                }
            }

            printf("  set to read-only after relocation: %s\n", relro ? "yes" : "no");

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

    if (ei->reloc.size) {
        assert(ei->reloc.data);

        // TODO
    }
}
