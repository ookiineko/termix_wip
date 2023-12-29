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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>  // for sysconf
#endif

#include <gelf.h>

#include "../../inc/arch.h"
#include "../../inc/logging.h"

#include "elf.h"

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

#define _ROUND_DOWN(_x, _align)   ((_x / _align) * _align)

#define _DYN_TAKE_PTR(_dyn)       (_dyn.d_un.d_ptr)
#define _DYN_TAKE_VAL(_dyn)       (_dyn.d_un.d_val)

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

int tmixldr_parse_elf(int fd, tmixelf_info *ei) {
    if (__pagesize < 0) {
        errno = EAGAIN;

        return -1;
    }

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

    _ElfXX_Phdr *phdrs = NULL;  // array

    tmixelf_seg *si = NULL;  // array, optional
    tmix_chunk *relros = NULL;  // array, optional

    if (hdr.e_phoff && hdr.e_phnum) {
        // read all segment headers

        if (lseek(fd, hdr.e_phoff, SEEK_SET) < 0)
            return -1;

        if (!(phdrs = calloc(hdr.e_phnum, sizeof(_ElfXX_Phdr))))
            return -1;

        if (read(fd, phdrs, sizeof(_ElfXX_Phdr) * hdr.e_phnum) != (sizeof(_ElfXX_Phdr) * hdr.e_phnum)) {
            errno = EIO;
error:
            free(phdrs);

            if (si)
                free(si);

            if (relros)
                free(relros);
        }

        // now we will iterate through all segments

        int i;  // current semgent index

        const _ElfXX_Phdr *phdr = NULL;  // current segment header
                                         // set this at the beginning in loops

        // but first calculate the required size for arrays

        size_t load_seg_cnt = 0;
        size_t relro_seg_cnt = 0;

        for (i = 0; i < hdr.e_phnum; i++) {
            phdr = &phdrs[i];

            switch (phdr->p_type) {
                case PT_LOAD:
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
            !(si = calloc(load_seg_cnt, sizeof(tmixelf_seg))))
            goto error;

        if (relro_seg_cnt &&
            !(relros = calloc(relro_seg_cnt, sizeof(tmix_chunk))))
            goto error;

        // ok, now it's ready to go

        int j = 0;  // current load segment index
        int k = 0;  // current relro segment index
        ssize_t first_seg_vaddr = -1;
        size_t highest_addr = 0;
        bool execstack = false;

        for (i = 0; i < hdr.e_phnum; i++) {
            phdr = &phdrs[i];

            switch (phdr->p_type) {
                case PT_PHDR: {
                    // the program header table itself, skipping
                    break;
                }

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
                    memset(seg, 0, sizeof(tmixelf_seg));

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
                            }
                        }
                    } else {
                        // zeros only
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

                    if (highest_addr < end)
                        highest_addr = end;

                    break;
                } /* case PT_LOAD */

                case PT_DYNAMIC: {
                    if (lseek(fd, phdr->p_offset, SEEK_SET) < 0)
                        goto error;

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
                            goto error;

                        switch (dyn.d_tag) {
                            case DT_NULL:
                                // end of array, handled below
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
                            break;
                    } /* for (;;) */

                    // TODO: iterate through relocation table

                    break;
                } /* case PT_DYNAMIC */

                case PT_GNU_RELRO: {
                    // populate relro information

                    tmix_chunk *relro = &relros[k++];  // current element

                    size_t reminder = phdr->p_vaddr % __pagesize;

                    relro->off = _ROUND_DOWN(phdr->p_vaddr, __pagesize);
                    relro->size = phdr->p_memsz + reminder;

                    break;
                }

                case PT_INTERP: {
                    // path to dynamic linker, ignored
                    break;
                }

                case PT_GNU_STACK: {
                    assert(!execstack);
                    execstack = !!(__conv_flags(phdr->p_flags) & TMIXELF_SEG_EXEC);
                    break;
                }

                case PT_NOTE: {
                    // compiler information, ignored
                    break;
                }

                default: {
                    tmix_fixme("unhandled segment type 0x%x", phdr->p_type);
                    break;
                }
            } /* switch (phdr->p_type) */
        } /* for (i = 0; i < hdr.e_phnum; i++) */

        // actual size of segment info array can be smaller, resize if needed

        if (j < load_seg_cnt) {
            void *ptr;

            if (!(ptr = reallocarray(si, j, sizeof(tmixelf_seg))))
                goto error;  // idk know how could shrinking an array fail anyway
            si = ptr;
        }

        // finally...

        free(phdrs);

        // populate elf info

        if (load_seg_cnt) {
            // at least one loadable segment is found

            ei->segs.data = si;
            ei->segs.size = j;

            ei->mem_size = highest_addr;

            if (hdr.e_entry)
                ei->entry = hdr.e_entry - si[0].off;  // setup entrypoint

            if (relro_seg_cnt) {
                // at least one relro entry is found

                ei->relros.data = relros;
                ei->relros.size = relro_seg_cnt;
            }
        }

        if (execstack)
            ei->execstack = execstack;
    }

    return 0;
}

void tmixldr_free_elfinfo(tmixelf_info *ei) {
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
