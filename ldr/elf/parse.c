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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__) || defined(__CYGWIN__)
#include <unistd.h>
#endif

#include <gelf.h>

#include "../../inc/platform.h"
#include "../../inc/fixme.h"

#include "elf.h"

#ifdef TMIX32
#define __EXPECTED_EICLASS      (ELFCLASS32)
#define __EHDR_TYPE             Elf32_Ehdr
#define __PHDR_TYPE             Elf32_Phdr
#define __ELFWORD_TYPE          Elf32_Word
#elif defined(TMIX64)
#define __EXPECTED_EICLASS      (ELFCLASS64)
#define __EHDR_TYPE             Elf64_Ehdr
#define __PHDR_TYPE             Elf64_Phdr
#define __ELFWORD_TYPE          Elf64_Word
#else
#error Dont know ELF types on this platform yet
#endif

#ifdef __i386__
#define __EXPECTED_EMACH        (EM_386)
#elif defined(__arm__)
#define __EXPECTED_EMACH        (EM_ARM)
#elif defined(__x86_64__)
#define __EXPECTED_EMACH        (EM_X86_64)
#elif defined(__aarch64__)
#define __EXPECTED_EMACH        (EM_AARCH64)
#else
#error Dont know some values on this architecture yet
#endif

#ifdef TMIX_BIG_ENDIAN
#define __EXPECTED_EIDATA       (ELFDATA2MSB)
#else
#define __EXPECTED_EIDATA       (ELFDATA2LSB)
#endif

#define ROUND_DOWN(_x, _align)   ((_x / _align) * _align)

static ssize_t __pagesize = -1;

/*
 * convert ELF segment flags to internal ones
 */
static inline tmixelf_seg_flag __conv_flags(__ELFWORD_TYPE flags) {
    tmixelf_seg_flag res = 0;

    if (flags & PF_R)
        res |= TMIXELF_SEG_READ;
    if (flags & PF_W)
        res |= TMIXELF_SEG_WRITE;
    if (flags & PF_X)
        res |= TMIXELF_SEG_EXEC;

    return res;
}

int tmixldr_parse_elf(int fd, tmixelf_info *ei) {
    if (__pagesize < 0) {
        errno = EAGAIN;

        return -1;
    }

    if (lseek(fd, 0, SEEK_SET) < 0)
        return -1;

    __EHDR_TYPE hdr;

    if (read(fd, &hdr, sizeof(__EHDR_TYPE)) != sizeof(__EHDR_TYPE)) {
read_failed:
        // failed to short read
        errno = EIO;
        return -1;
    }

    // basic checking

    if (memcmp(hdr.e_ident, ELFMAG, SELFMAG)) {
invalid_elf:
        errno = EBADF;
        return -1;
    }

    if (hdr.e_ident[EI_CLASS] != __EXPECTED_EICLASS)
        goto invalid_elf;

    if (hdr.e_ident[EI_DATA] != __EXPECTED_EIDATA)
        goto invalid_elf;

    if (hdr.e_ident[EI_VERSION] != EV_CURRENT)
        goto invalid_elf;

    if (hdr.e_ident[EI_OSABI] != ELFOSABI_SYSV)
        goto invalid_elf;

    if (hdr.e_ident[EI_ABIVERSION] != 0)
        goto invalid_elf;

    if (hdr.e_type != ET_DYN)
        goto invalid_elf;

    if (hdr.e_machine != __EXPECTED_EMACH)
        goto invalid_elf;

    if (hdr.e_version != EV_CURRENT)
        goto invalid_elf;

    if (hdr.e_ehsize != sizeof(__EHDR_TYPE))
        goto invalid_elf;

    if (hdr.e_phentsize != sizeof(__PHDR_TYPE))
        goto invalid_elf;

    // parse segments

    if (hdr.e_phoff && hdr.e_phnum) {
        if (lseek(fd, hdr.e_phoff, SEEK_SET) < 0)
            return -1;

        int i, j = 0;
        __PHDR_TYPE phdr;
        ssize_t first_seg_vaddr = -1;
        ssize_t highest_addr = -1;
        tmixelf_seg *si = NULL;  // array
        char *interp = NULL;
        bool execstack = false;
        char *rpath = NULL;
        int relro_count = 0;
        tmix_chunk *relros = NULL;  // array

        // iterate through all segments

        for (i = 0; i < hdr.e_phnum; i++) {
            if (read(fd, &phdr, sizeof(__PHDR_TYPE)) != sizeof(__PHDR_TYPE)) {
                if (si)
                    free(si);  // clean up mess before quit

                goto read_failed;
            }

            switch (phdr.p_type) {
                case PT_PHDR: {
                    // the program header table itself, skipping
                    break;
                }
                case PT_LOAD: {
                    if (!phdr.p_memsz)
                        continue;  // wat

                    // check alignment

                    if ((phdr.p_align % __pagesize) != 0 ||
                        ((phdr.p_vaddr - phdr.p_offset) % phdr.p_align) != 0) {
                        errno = EBADF;
error:
                        if (si)
                            free(si);

                        if (relros)
                            free(relros);

                        return -1;
                    }

                    // resize buffer
                    void *ptr;

                    j++;
                    if (!(ptr = reallocarray(si, j, sizeof(tmixelf_seg))))
                        goto error;
                    si = ptr;

                    // populate segment information

                    tmixelf_seg *seg = &si[j - 1];  // current segment
                    memset(seg, 0, sizeof(tmixelf_seg));

                    size_t reminder = phdr.p_vaddr % phdr.p_align;
                    size_t vaddr = ROUND_DOWN(phdr.p_vaddr, phdr.p_align);

                    if (first_seg_vaddr < 0) {
                        first_seg_vaddr = vaddr;
                        seg->off = 0;
                    } else
                        seg->off = vaddr - first_seg_vaddr;

                    if (phdr.p_filesz) {
                        // has file data
                        seg->file.off = ROUND_DOWN(phdr.p_offset, phdr.p_align);
                        seg->file.size = phdr.p_filesz + reminder;  // add reminder if needed

                        if (phdr.p_memsz > phdr.p_filesz) {
                            // has extra zero paddings after file data
                            size_t file_pages = seg->file.size / phdr.p_align;

                            if ((seg->file.size % phdr.p_align) != 0)
                                file_pages++;

                            size_t real_size = file_pages * phdr.p_align;

                            seg->pad.off = real_size;
                            seg->pad.size = phdr.p_memsz - real_size;
                        }
                    } else {
                        // zeros only
                        seg->pad.off = 0;
                        seg->pad.size = phdr.p_memsz + reminder;  // add reminder to here since no file data
                    }

                    seg->flags = __conv_flags(phdr.p_flags);

                    if ((seg->flags & TMIXELF_SEG_EXEC) && !(seg->flags & TMIXELF_SEG_READ)) {
                        // executable but not readable ?!
                        errno = EBADF;

                        goto error;
                    }

                    // update total memory size

                    size_t end = seg->off + phdr.p_memsz + reminder;

                    if (highest_addr < (ssize_t) end)
                        highest_addr = end;

                    break;
                }
                case PT_DYNAMIC: {
                    // TODO
                    break;
                }
                case PT_GNU_RELRO: {
                    // resize buffer

                    void *ptr;

                    relro_count++;

                    if (!(ptr = reallocarray(relros, relro_count, sizeof(tmix_chunk))))
                        goto error;
                    relros = ptr;

                    // populate relro information

                    tmix_chunk *relro = &relros[relro_count - 1];  // current element

                    size_t reminder = phdr.p_vaddr % __pagesize;

                    relro->off = ROUND_DOWN(phdr.p_vaddr, __pagesize);
                    relro->size = phdr.p_memsz + reminder;

                    break;
                }
                case PT_INTERP: {
                    off_t old_fptr = lseek(fd, 0, SEEK_CUR);

                    if (old_fptr < 0)
                        goto error;

                    if (lseek(fd, phdr.p_offset, SEEK_SET) < 0)
                        goto error;

                    interp = malloc(phdr.p_filesz);

                    if (!interp)
                        goto error;

                    if (read(fd, interp, phdr.p_filesz) != phdr.p_filesz) {
                        errno = EIO;
                        goto error;
                    }

                    if (lseek(fd, old_fptr, SEEK_SET) < 0)
                        goto error;

                    break;
                }
                case PT_GNU_STACK: {
                    execstack = __conv_flags(phdr.p_flags) & TMIXELF_SEG_EXEC;
                    break;
                }
                case PT_NOTE: {
                    // compiler information, ignored
                    break;
                }
                default: {
                    tmix_fixme("unhandled segment type 0x%x", phdr.p_type);
                    break;
                }
            }
        }

        if (j) {
            // at least one loadable segment is found

            ei->seg.data = si;
            ei->seg.size = j;

            ei->mem_size = highest_addr;

            if (hdr.e_entry)
                ei->entry = hdr.e_entry - si[0].off;  // setup entrypoint

            if (relro_count) {
                // at least one relro entry is found

                ei->relro.data = relros;
                ei->relro.size = relro_count;
            }
        }

        if (interp)
            ei->interp = interp;  // interpreter path is found

        if (execstack)
            ei->execstack = execstack;

        if (rpath)
            ei->rpath = rpath;  // runtime path is found
    }

    return 0;
}

void tmixldr_free_elfinfo(tmixelf_info *ei) {
    // free arrays

    if (ei->seg.data) {
        free(ei->seg.data);

        ei->seg.data = NULL;
    }

    tmixelf_reloc *ri = ei->reloc.data;  // array

    if (ri) {
        int i;

        // free strings, then free array

        for (i = 0; i < ei->reloc.size; i++) {
            if (ri[i].sym) {
                free(ri[i].sym);

                ri[i].sym = NULL;
            }
        }

        free(ri);

        ei->reloc.data = NULL;
    }

    if (ei->relro.data) {
        free(ei->relro.data);

        ei->relro.data = NULL;
    }

    if (ei->interp) {
        free(ei->interp);
        ei->interp = NULL;
    }

    if (ei->rpath) {
        free(ei->rpath);
        ei->rpath = NULL;
    }
}

__attribute__((constructor)) static void __init_pagesize(void) {
#ifdef _WIN32
    SYSTEM_INFO si = {};
    GetSystemInfo(&si);
    __pagesize = si.dwAllocationGranularity;
#elif defined(__linux__) || defined(__CYGWIN__)
    __pagesize = sysconf(_SC_PAGESIZE);

    if (__pagesize < 0)
        perror("error determining machine page size");
#else
#error Dont know how to get page size on this platform yet
#endif
}
