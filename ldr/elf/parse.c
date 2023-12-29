/*
  parse.c - ELF parser

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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../inc/arch.h"

#include "elf.h"

#include "_segs.h"
#include "_types.h"

#ifdef TMIX32
#define _EXPECTED_EICLASS      (ELFCLASS32)
#elif defined(TMIX64)
#define _EXPECTED_EICLASS      (ELFCLASS64)
#else
#error Dont know ELF class on this platform yet
#endif

#ifdef __i386__
#define _EXPECTED_EMACH        (EM_386)
#elif defined(__arm__)
#define _EXPECTED_EMACH        (EM_ARM)
#elif defined(__x86_64__)
#define _EXPECTED_EMACH        (EM_X86_64)
#elif defined(__aarch64__)
#define _EXPECTED_EMACH        (EM_AARCH64)
#else
#error Dont know ELF machine value on this architecture yet
#endif

#ifdef TMIX_BIG_ENDIAN
#define _EXPECTED_EIDATA       (ELFDATA2MSB)
#elif defined(TMIX_LITTLE_ENDIAN)
#define _EXPECTED_EIDATA       (ELFDATA2LSB)
#else
#error Dont know endian-ness on this platform yet
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
    }

    return 0;
}

void tmixelf_free_info(tmixelf_info *ei) {
    // free arrays

    if (ei->segs.data) {
        free(ei->segs.data);

        ei->segs.data = NULL;
    }

    tmixelf_reloc *ri = ei->relocs.data;  // array

    if (ri) {
        int i;

        // free strings, then free array

        for (i = 0; i < ei->relocs.size; i++) {
            if (ri[i].sym.name) {
                free(ri[i].sym.name);

                ri[i].sym.name = NULL;
            }
        }

        free(ri);

        ei->relocs.data = NULL;
    }

    if (ei->relros.data) {
        free(ei->relros.data);

        ei->relros.data = NULL;
    }

    if (ei->needs.data) {
        free(ei->needs.data);

        ei->needs.data = NULL;
    }
}
