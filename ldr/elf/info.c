/*
  info.c - ELF information

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
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../inc/arch.h"
#include "../../inc/types.h"

#include "elf.h"

#include "_arch.h"
#include "_elf.h"
#include "_segs.h"

#ifdef TMIX32
#  define _EXPECTED_EICLASS      (ELFCLASS32)
#elif defined(TMIX64)
#  define _EXPECTED_EICLASS      (ELFCLASS64)
#else
#  error Dont know ELF class on this platform yet
#endif

#ifdef TMIX32
#  define _PTRFMT                "%#08" PRIxPTR
#elif defined(TMIX64)
#  define _PTRFMT                "%#016" PRIxPTR
#else
#  error Dont know word size on this platform yet
#endif

#ifdef __i386__
#  define _EXPECTED_EMACH        (EM_386)
#elif defined(__arm__)
#  define _EXPECTED_EMACH        (EM_ARM)
#elif defined(__x86_64__)
#  define _EXPECTED_EMACH        (EM_X86_64)
#elif defined(__aarch64__)
#  define _EXPECTED_EMACH        (EM_AARCH64)
#else
#  error Dont know ELF machine value on this architecture yet
#endif

#ifdef TMIX_BIG_ENDIAN
#  define _EXPECTED_EIDATA       (ELFDATA2MSB)
#elif defined(TMIX_LITTLE_ENDIAN)
#  define _EXPECTED_EIDATA       (ELFDATA2LSB)
#else
#  error Dont know endian-ness on this platform yet
#endif

int tmixelf_parse_info(int fd, tmixelf_info *ei) {
    if (lseek(fd, 0, SEEK_SET) < 0)
        return -1;

    _ElfXX_Ehdr hdr;

    if (read(fd, &hdr, sizeof(_ElfXX_Ehdr)) != sizeof(_ElfXX_Ehdr)) {
        // failed to short read
        errno = EIO;
        return -1;
    }

    // basic checking

    if (memcmp(hdr.e_ident, ELFMAG, SELFMAG)) {
bad_elf:
        errno = EBADF;
        return -1;
    }

    if (hdr.e_ident[EI_CLASS] != _EXPECTED_EICLASS
        || hdr.e_ident[EI_DATA] != _EXPECTED_EIDATA
        || hdr.e_ident[EI_VERSION] != EV_CURRENT
        || (hdr.e_ident[EI_OSABI] != ELFOSABI_SYSV
            && hdr.e_ident[EI_OSABI] != ELFOSABI_GNU)
        || hdr.e_ident[EI_ABIVERSION] != 0)
        goto bad_elf;

    if (hdr.e_type != ET_DYN
        || hdr.e_machine != _EXPECTED_EMACH
        || hdr.e_version != EV_CURRENT
        || hdr.e_ehsize != sizeof(_ElfXX_Ehdr)
        || hdr.e_phentsize != sizeof(_ElfXX_Phdr))
        goto bad_elf;

    // parse segments

    if (hdr.e_phoff && hdr.e_phnum) {
        // at least one segment is present

        tmixelf_internal_segs eis = {};

        if (_tmixelf_internal_parse_segs(fd, &hdr, &eis) < 0)
            return -1;

        // populate elf info

        if (eis.segs.size) {
            // at least one loadable segment is found

            ei->segs.data = eis.segs.data;
            ei->segs.size = eis.segs.size;

            ei->mem_size = eis.highest_addr;

            tmixelf_seg *si = ei->segs.data;  // array

            if (hdr.e_entry)
                ei->entry = hdr.e_entry - si[0].off;  // setup entrypoint

            if (eis.relros.size) {
                // at least one relro entry is found

                ei->relros.data = eis.relros.data;
                ei->relros.size = eis.relros.size;
            }
        }

        if (eis.execstack)
            ei->execstack = eis.execstack;

        if (eis.needs.size) {
            ei->needs.data = eis.needs.data;
            ei->needs.size = eis.needs.size;
        }

        if (eis.syms.size) {
            ei->syms.data = eis.syms.data;
            ei->syms.size = eis.syms.size;

            if (eis.relocs.size) {
                ei->relocs.data = eis.relocs.data;
                ei->relocs.size = eis.relocs.size;
            }
        }
    }

    return 0;
}


void tmixelf_print_info(const tmixelf_info *ei) {
    if (!ei)
        return;

    if (ei->entry)
        printf("entrypoint offset (relative): %#" PRIxPTR "\n", ei->entry);

    printf("total size in memory when loaded: %#" PRIxPTR "\n", ei->mem_size);

    printf("stack executable: %s\n", ei->execstack ? "yes" : "no");

    printf("loadable segment count: %" PRIuPTR "\n", ei->segs.size);

    printf("post-reloc RO segment count: %" PRIuPTR "\n", ei->relros.size);

    printf("symbol count: %" PRIuPTR "\n", ei->syms.size);

    printf("relocation count: %" PRIuPTR "\n", ei->relocs.size);

    size_t i;

    if (ei->segs.size) {
        tmixelf_seg *si = ei->segs.data;

        assert(si);

        for (i = 0; i < ei->segs.size; i++) {
            printf("loadable segment #%" PRIuPTR ":\n", i);
            printf("  relative offset: " _PTRFMT "\n", si[i].off);
            if (si[i].file.size)
                printf("  file data size: %#" PRIxPTR " (at file offset " _PTRFMT ")\n", si[i].file.size, si[i].file.off);
            if (si[i].pad.size)
                printf("  zero padding size: %#" PRIxPTR " (relative offset " _PTRFMT ")\n", si[i].pad.size, si[i].pad.off);

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

        if (ei->relros.size) {
            tmix_chunk *relros = ei->relros.data;

            assert(relros);

            printf("relro segment list:\n");

            for (i = 0; i < ei->relros.size; i++) {
                printf("  " _PTRFMT " to " _PTRFMT "\n", relros[i].off, relros[i].off + relros[i].size);
            }
        }
    }

    if (ei->syms.size) {
        tmixelf_sym *syms = ei->syms.data;
        assert(syms);

        printf("symbol table:\n");

        for (i = 0; i < ei->syms.size; i++) {
            printf("  " _PTRFMT " %s (", syms[i].off, syms[i].name);

            switch(syms[i].type) {
                case TMIXELF_SYM_DATA:
                    printf("data");
                    break;
                case TMIXELF_SYM_FUNC:
                    printf("function");
                    break;
                default:
                    printf("unknown");
                    break;
            }

            printf(") ");

            if (syms[i].imported)
                printf("(external) ");

            printf("\n");
        }

        if (ei->relocs.size) {
            tmixelf_reloc *relocs = ei->relocs.data;
            assert(relocs);

            printf("relocation table:\n");

            for (i = 0; i < ei->relocs.size; i++) {
                tmixelf_sym *sym = &syms[relocs[i].symidx];

                printf("  " _PTRFMT " %s\n", relocs[i].off, sym->name);
            }
        }
    }

    if (ei->needs.size) {
        printf("required library list:\n");

        char **needs = ei->needs.data;

        assert(needs);

        for (i = 0; i < ei->needs.size; i++) {
            assert(needs[i]);
            printf("  %s\n", needs[i]);
        }
    }
}

void tmixelf_free_info(tmixelf_info *ei) {
    // free arrays

    if (ei->segs.data) {
        free(ei->segs.data);

        ei->segs.data = NULL;
    }

    tmixelf_sym *syms = ei->syms.data;  // array

    if (syms) {
        size_t i;

        // free strings, then free array

        for (i = 0; i < ei->syms.size; i++) {
            if (syms[i].name) {
                free(syms[i].name);

                syms[i].name = NULL;
            }
        }

        free(syms);

        ei->syms.data = NULL;
    }

    if (ei->relros.data) {
        free(ei->relros.data);

        ei->relros.data = NULL;
    }

    if (ei->needs.data) {
        free(ei->needs.data);

        ei->needs.data = NULL;
    }

    if (ei->relocs.data) {
        free(ei->relocs.data);

        ei->relocs.data = NULL;
    }
}
