/*
  segs.c - ELF segments

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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>  // for sysconf
#endif

#include "../../inc/logging.h"
#include "../../inc/types.h"

#include "elf.h"

#include "_arch.h"
#include "_elf.h"
#include "_dyn.h"
#include "_segs.h"

#define _ROUND_DOWN(_x, _align)   ((_x / _align) * _align)

static ssize_t __pagesize = -1;

/*
 * convert ELF segment flags to internal ones
 */
static inline tmixelf_seg_flag __conv_flags(_ElfXX_Word flags) {
    tmixelf_seg_flag res = 0;

    if (flags & PF_R)
        res |= TMIXELF_SEG_READ;
    if (flags & PF_W)
        res |= TMIXELF_SEG_WRITE;
    if (flags & PF_X)
        res |= TMIXELF_SEG_EXEC;

    return res;
}

int _tmixelf_internal_parse_segs(int fd, _ElfXX_Ehdr *hdr, tmixelf_internal_segs *eis) {
    if (__pagesize < 0) {
        errno = EAGAIN;

        return -1;
    }

    _ElfXX_Phdr *phdrs = NULL;  // array

    // read all segment headers

    if (lseek(fd, hdr->e_phoff, SEEK_SET) < 0)
        return -1;

    if (!(phdrs = calloc(hdr->e_phnum, sizeof(_ElfXX_Phdr))))
        return -1;

    if (read(fd, phdrs, sizeof(_ElfXX_Phdr) * hdr->e_phnum) != (ssize_t)(sizeof(_ElfXX_Phdr) * hdr->e_phnum)) {
        errno = EIO;
error:
        // clean up messes before return

        free(phdrs);

        if (eis->segs.data) {
            free(eis->segs.data);
            eis->segs.data = NULL;
        }

        if (eis->relros.data) {
            free(eis->relros.data);
            eis->relros.data = NULL;
        }

        return -1;
    }

    // now we will iterate through all segments

    int i;  // current semgent index

    const _ElfXX_Phdr *phdr = NULL;  // current segment header
                                      // set this at the beginning in loops

    // but first calculate the required size for arrays

    size_t load_seg_cnt = 0;
    size_t relro_seg_cnt = 0;

    for (i = 0; i < hdr->e_phnum; i++) {
        phdr = &phdrs[i];

        switch (phdr->p_type) {
            case PT_LOAD:
                if (!phdr->p_memsz)
                    continue;  // wat

                load_seg_cnt++;
                break;
            case PT_GNU_RELRO:
                relro_seg_cnt++;
                break;
            default:
                break;
        }
    }

    if (load_seg_cnt &&
        !(eis->segs.data = calloc(load_seg_cnt, sizeof(tmixelf_seg))))
        goto error;

    if (relro_seg_cnt &&
        !(eis->relros.data = calloc(relro_seg_cnt, sizeof(tmix_chunk))))
        goto error;

    // ok, now it's ready to go

    tmixelf_seg *si = eis->segs.data;  // array
    tmix_chunk *relros = eis->relros.data;  // array

    int j = 0;  // current load segment index
    int k = 0;  // current relro segment index
    ssize_t first_seg_vaddr = -1;

    for (i = 0; i < hdr->e_phnum; i++) {
        phdr = &phdrs[i];

        switch (phdr->p_type) {
            case PT_LOAD: {
                if (!phdr->p_memsz)
                    continue;  // wat

                // check alignment

                if ((phdr->p_align % __pagesize) != 0 ||
                    ((phdr->p_vaddr - phdr->p_offset) % phdr->p_align) != 0) {
                    errno = EBADF;
                    goto error;
                }

                // populate segment information

                tmixelf_seg *seg = &si[j++];  // current segment

                size_t reminder = phdr->p_vaddr % phdr->p_align;
                size_t vaddr = _ROUND_DOWN(phdr->p_vaddr, phdr->p_align);

                if (first_seg_vaddr < 0) {
                    first_seg_vaddr = vaddr;
                    seg->off = 0;
                } else
                    seg->off = vaddr - first_seg_vaddr;

                size_t filesize = phdr->p_filesz + reminder;
                size_t memsize = phdr->p_memsz + reminder;

                if (phdr->p_filesz) {
                    // has file data
                    seg->file.off = _ROUND_DOWN(phdr->p_offset, phdr->p_align);
                    seg->file.size = filesize;  // add reminder if needed

                    if (phdr->p_memsz > phdr->p_filesz) {
                        // has extra zero paddings after file data
                        size_t file_pages = seg->file.size / phdr->p_align;

                        if ((seg->file.size % phdr->p_align) != 0)
                            file_pages++;

                        size_t real_size = file_pages * phdr->p_align;

                        if (real_size < memsize) {
                            // actually need explicit zero padding
                            seg->pad.off = real_size;
                            seg->pad.size = memsize - real_size;
                        } else
                            seg->pad.size = 0;
                    } else
                            seg->pad.size = 0;
                } else {
                    // zeros only
                    seg->file.size = 0;
                    seg->pad.off = 0;
                    seg->pad.size = memsize;  // add reminder to here since no file data
                }

                seg->flags = __conv_flags(phdr->p_flags);

                if ((seg->flags & TMIXELF_SEG_EXEC) && !(seg->flags & TMIXELF_SEG_READ)) {
                    // executable but not readable ?!
                    errno = EBADF;

                    goto error;
                }

                // update total memory size

                size_t end = seg->off + phdr->p_memsz + reminder;

                if (eis->highest_addr < end)
                    eis->highest_addr = end;

                break;
            } /* case PT_LOAD */
            case PT_DYNAMIC: {
                tmixelf_internal_dyn eid = {};

                if (_tmixelf_internal_parse_dyn(fd, phdr, &eid) < 0)
                    goto error;

                if (eid.needs.size) {
                    eis->needs.data = eid.needs.data;
                    eis->needs.size = eid.needs.size;
                }

                if (eid.syms.size) {
                    eis->syms.data = eid.syms.data;
                    eis->syms.size = eid.syms.size;

                    if (eid.relocs.size) {
                        eis->relocs.data = eid.relocs.data;
                        eis->relocs.size = eid.relocs.size;
                    }
                }

                break;
            }
            case PT_GNU_RELRO: {
                // populate relro information

                tmix_chunk *relro = &relros[k++];  // current element

                size_t reminder = phdr->p_vaddr % __pagesize;

                relro->off = _ROUND_DOWN(phdr->p_vaddr, __pagesize);
                relro->size = phdr->p_memsz + reminder;

                break;
            }
            case PT_GNU_STACK:
                assert(!eis->execstack);
                eis->execstack = !!(__conv_flags(phdr->p_flags) & TMIXELF_SEG_EXEC);
                break;
            case PT_PHDR:
                // the program header table itself, skipping
            case PT_INTERP:
                // path to dynamic linker, ignored
            case PT_NOTE:
                // compiler information, ignored
                break;
            default:
                tmix_fixme("unhandled segment type %#x", phdr->p_type);
                break;
        } /* switch (phdr->p_type) */
    } /* for (i = 0; i < hdr.e_phnum; i++) */

    eis->segs.size = load_seg_cnt;
    eis->relros.size = relro_seg_cnt;

    // finally...

    free(phdrs);

    return 0;
}

__attribute__((constructor)) static void __init_pagesize(void) {
#ifdef _WIN32
    SYSTEM_INFO si = {};
    GetSystemInfo(&si);
    __pagesize = si.dwAllocationGranularity;
#else
    __pagesize = sysconf(_SC_PAGESIZE);

    if (__pagesize < 0)
        perror("error determining machine page size");
#endif
}
