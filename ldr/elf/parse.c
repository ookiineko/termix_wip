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
#elif defined(__linux__) || defined(__CYGWIN__)
#include <unistd.h>  // for sysconf
#endif

#include <gelf.h>

#include "../../inc/logging.h"
#include "../../inc/platform.h"

#include "elf.h"

#ifdef TMIX32
#define __EXPECTED_EICLASS      (ELFCLASS32)
#define __EHDR_TYPE             Elf32_Ehdr
#define __PHDR_TYPE             Elf32_Phdr
#define __ELFDYN_TYPE           Elf32_Dyn
#define __ELFSYM_TYPE           Elf32_Sym
#define __ELFWORD_TYPE          Elf32_Word

#define __ELF_ST_BIND           ELF32_ST_BIND
#define __ELF_ST_TYPE           ELF32_ST_TYPE
#define __ELF_ST_VIS            ELF32_ST_VISIBILITY
#elif defined(TMIX64)
#define __EXPECTED_EICLASS      (ELFCLASS64)
#define __EHDR_TYPE             Elf64_Ehdr
#define __PHDR_TYPE             Elf64_Phdr
#define __ELFDYN_TYPE           Elf64_Dyn
#define __ELFSYM_TYPE           Elf64_Sym
#define __ELFWORD_TYPE          Elf64_Word

#define __ELF_ST_BIND           ELF64_ST_BIND
#define __ELF_ST_TYPE           ELF64_ST_TYPE
#define __ELF_ST_VIS            ELF64_ST_VISIBILITY
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
#error Dont know ELF machine value on this architecture yet
#endif

#ifdef TMIX_BIG_ENDIAN
#define __EXPECTED_EIDATA       (ELFDATA2MSB)
#else
#define __EXPECTED_EIDATA       (ELFDATA2LSB)
#endif

#define __ROUND_DOWN(_x, _align)   ((_x / _align) * _align)

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
bad_elf:
        errno = EBADF;
        return -1;
    }

    if (hdr.e_ident[EI_CLASS] != __EXPECTED_EICLASS)
        goto bad_elf;

    if (hdr.e_ident[EI_DATA] != __EXPECTED_EIDATA)
        goto bad_elf;

    if (hdr.e_ident[EI_VERSION] != EV_CURRENT)
        goto bad_elf;

    unsigned char osabi = hdr.e_ident[EI_OSABI];

    if (osabi != ELFOSABI_SYSV &&
        osabi != ELFOSABI_GNU)
        goto bad_elf;

    if (hdr.e_ident[EI_ABIVERSION] != 0)
        goto bad_elf;

    if (hdr.e_type != ET_DYN)
        goto bad_elf;

    if (hdr.e_machine != __EXPECTED_EMACH)
        goto bad_elf;

    if (hdr.e_version != EV_CURRENT)
        goto bad_elf;

    if (hdr.e_ehsize != sizeof(__EHDR_TYPE))
        goto bad_elf;

    if (hdr.e_phentsize != sizeof(__PHDR_TYPE))
        goto bad_elf;

    // parse segments

    __PHDR_TYPE *phdrs = NULL;  // array

    tmixelf_seg *si = NULL;  // array, optional
    tmix_chunk *relros = NULL;  // array, optional

    if (hdr.e_phoff && hdr.e_phnum) {
        // read all segment headers

        if (lseek(fd, hdr.e_phoff, SEEK_SET) < 0)
            return -1;

        if (!(phdrs = calloc(hdr.e_phnum, sizeof(__PHDR_TYPE))))
            return -1;

        if (read(fd, phdrs, sizeof(__PHDR_TYPE) * hdr.e_phnum) != (sizeof(__PHDR_TYPE) * hdr.e_phnum)) {
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

        const __PHDR_TYPE *phdr = NULL;  // current segment header
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
        ssize_t highest_addr = -1;
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
                    size_t vaddr = __ROUND_DOWN(phdr->p_vaddr, phdr->p_align);

                    if (first_seg_vaddr < 0) {
                        first_seg_vaddr = vaddr;
                        seg->off = 0;
                    } else
                        seg->off = vaddr - first_seg_vaddr;

                    size_t filesize = phdr->p_filesz + reminder;
                    size_t memsize = phdr->p_memsz + reminder;

                    if (phdr->p_filesz) {
                        // has file data
                        seg->file.off = __ROUND_DOWN(phdr->p_offset, phdr->p_align);
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

                    if (highest_addr < (ssize_t) end)
                        highest_addr = end;

                    break;
                } /* case PT_LOAD */

                case PT_DYNAMIC: {
                    if (lseek(fd, phdr->p_offset, SEEK_SET) < 0)
                        goto error;

                    // iterate through all entries

                    __ELFDYN_TYPE dyn;

                    ssize_t strtab_off = -1;
                    ssize_t strtab_size = -1;
                    ssize_t symtab_off = -1;
                    ssize_t got_off = -1;

                    for (;;) {
                        if (read(fd, &dyn, sizeof(__ELFDYN_TYPE)) != sizeof(__ELFDYN_TYPE))
                            goto error;

                        switch (dyn.d_tag) {
                            case DT_NULL:
                                // end of array, ignored
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
                                strtab_off = dyn.d_un.d_ptr;
                                break;
                            case DT_STRSZ:
                                strtab_size = dyn.d_un.d_val;
                                break;
                            case DT_SYMENT:
                                assert(dyn.d_un.d_val == sizeof(__ELFSYM_TYPE));
                                break;
                            case DT_SYMTAB:
                                strtab_off = dyn.d_un.d_ptr;
                                break;
                            case DT_PLTGOT:
                                got_off = dyn.d_un.d_ptr;
                                break;
                            case DT_PLTRELSZ:
                                // TODO: is this needed?
                                break;
                            case DT_PLTREL:
                                // TODO: is this needed?
                                break;
                            case DT_JMPREL:
                                // lazy binding is not implemented, ignored
                                break;
                            case DT_FLAGS_1:
                                switch (dyn.d_un.d_val) {
                                    case DF_1_PIE:
                                        // already know, ignored
                                        break;
                                    default:
                                        tmix_fixme("unhandled state flag 0x%lx", dyn.d_un.d_val);
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

                    // TODO: iterate through symbol table

                    break;
                } /* case PT_DYNAMIC */

                case PT_GNU_RELRO: {
                    // populate relro information

                    tmix_chunk *relro = &relros[k++];  // current element

                    size_t reminder = phdr->p_vaddr % __pagesize;

                    relro->off = __ROUND_DOWN(phdr->p_vaddr, __pagesize);
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

            ei->seg.data = si;
            ei->seg.size = j;

            ei->mem_size = highest_addr;

            if (hdr.e_entry)
                ei->entry = hdr.e_entry - si[0].off;  // setup entrypoint

            if (relro_seg_cnt) {
                // at least one relro entry is found

                ei->relro.data = relros;
                ei->relro.size = relro_seg_cnt;
            }
        }

        if (execstack)
            ei->execstack = execstack;
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
