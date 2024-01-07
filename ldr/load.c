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
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef _WIN32
#  include <fcntl.h>
#  include <limits.h>

#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

#  define MAP_FAILED        NULL
#else
#  include <sys/mman.h>
#endif

#include "elf/elf.h"
#include "load.h"

#ifdef _WIN32
#define tmixldr_internal_prot_zero      {}
typedef struct {
    DWORD prot;  // for pages
    DWORD access;  // for file
} tmixldr_internal_prot_t;
#else
#define tmixldr_internal_prot_zero      (0)
typedef int tmixldr_internal_prot_t;
#endif

/*
 * convert internal segment flags to OS-specific protections
 */
static inline tmixldr_internal_prot_t __conv_prot(tmixelf_seg_flag flags, bool pad) {
    tmixldr_internal_prot_t res = tmixldr_internal_prot_zero;

#ifdef _WIN32
    bool read = flags & TMIXELF_SEG_READ;
    bool write = flags & TMIXELF_SEG_WRITE;
    bool exec = flags & TMIXELF_SEG_EXEC;
    bool copy = write && !pad;  // don't use copy-on-write for anonymous mapping

    if (write)
        assert(read);

    if (exec) {
        if (write)
            // write implies read
            res.prot = copy ? PAGE_EXECUTE_WRITECOPY : PAGE_EXECUTE_READWRITE;
        else
            res.prot = read ? PAGE_EXECUTE_READ : PAGE_NOACCESS;
    } else {
        if (write)
            // write implies read
            res.prot = copy ? PAGE_WRITECOPY : PAGE_READWRITE;
        else
            res.prot = read ? PAGE_READONLY : PAGE_NOACCESS;
    }

    if (read && !copy)
        res.access |= FILE_MAP_READ;
    if (write)
        res.access |= copy ? FILE_MAP_COPY : FILE_MAP_WRITE;
    if (exec)
        res.access |= FILE_MAP_EXECUTE;
#else
    (void) pad;

    if (flags & TMIXELF_SEG_READ)
        res |= PROT_READ;
    if (flags & TMIXELF_SEG_WRITE)
        res |= PROT_WRITE;
    if (flags & TMIXELF_SEG_EXEC)
        res |= PROT_EXEC;
#endif

    return res;
}

#ifdef _WIN32
static inline void *__win32_mmap_file(void *addr, size_t len,
                                       tmixldr_internal_prot_t prot,
                                       HANDLE hFile, ssize_t off) {
    _Static_assert(sizeof(DWORD) == sizeof(unsigned long int),
                    "DWORD has an unexpected size on this machine");
#   if __SIZEOF_SIZE_T__ == __SIZEOF_LONG__
    // on 32-bit Windows, size_t is a single DWORD

    DWORD dwSizeLow = len + ((size_t) off);  // max size
    DWORD dwSizeHigh = 0;

    DWORD dwOffsetLow = (size_t) off;
    DWORD dwOffsetHigh = 0;
#   elif __SIZEOF_SIZE_T__ == (__SIZEOF_LONG__ * 2)
    // on 64-bit Windows, size_t can fit two DWORDs

    size_t max_size = len + ((size_t) off);
    DWORD dwSizeLow = (DWORD) (max_size & ULONG_MAX);
    DWORD dwSizeHigh = (DWORD) ((max_size >> (sizeof(DWORD) * CHAR_BIT)) & ULONG_MAX);

    DWORD dwOffsetLow = (DWORD) (((size_t) off) & ULONG_MAX);
    DWORD dwOffsetHigh = (DWORD) ((((size_t) off) >> (sizeof(DWORD) * CHAR_BIT)) & ULONG_MAX);
#  else
#    error Dont know how to deal with DWORDs on this machine yet
#  endif

    HANDLE hMapping = CreateFileMapping(hFile, NULL, prot.prot, dwSizeHigh, dwSizeLow, NULL);

    if (hMapping == MAP_FAILED) {
error:
        // FIXME: set errno according to win32 error
        errno = -1;
        return MAP_FAILED;
    }

    void *ptr = MapViewOfFileEx(hMapping, prot.access, dwOffsetHigh, dwOffsetLow, len, addr);

    CloseHandle(hMapping);

    if (ptr == MAP_FAILED)
        goto error;

    return ptr;
}
#endif

int tmixldr_load_elf(int fd, const tmixelf_info *ei, tmixldr_elf *e) {
    if (e->base) {
        // seems already loaded
        errno = EBUSY;
        return -1;
    }

    if (!ei->segs.size) {
        // no loadable segement??
        return 0;
    }

#ifdef _WIN32
    // reopen file with enough access to perform all possbile mappings

    HANDLE hOldFile = (HANDLE) _get_osfhandle(fd);

    if (hOldFile == INVALID_HANDLE_VALUE) {
        errno = EBADF;
        return -1;
    }

    HANDLE hFile = ReOpenFile(hOldFile, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_VALID_FLAGS, 0);

    if (hFile == INVALID_HANDLE_VALUE) {
        // FIXME: set errno according to win32 error
        errno = -1;
        return -1;
    }
#endif

    // reserve memory

#ifdef _WIN32
    void *base = VirtualAlloc(NULL, ei->mem_size, MEM_RESERVE, PAGE_NOACCESS);
#else
    void *base = mmap(NULL, ei->mem_size, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
#endif

    if (base == MAP_FAILED) {
#ifdef _WIN32
        // FIXME: set errno according to win32 error
        errno = -1;
#endif
quit:
#ifdef _WIN32
        CloseHandle(hFile);
#endif
        return -1;
    }

// Unlike Linux, NT kernel doesn't like overlapped memory mappings
#ifdef _WIN32
    VirtualFree(base, 0, MEM_RELEASE);
#elif defined(__CYGWIN__)
    munmap(base, ei->mem_size);
#endif

    // setup segments

    tmixelf_seg *si = ei->segs.data;  // array

    assert(si && si[0].off == 0);

    size_t i;

    for (i = 0; i < ei->segs.size; i++) {
        tmixldr_internal_prot_t prot_file = __conv_prot(si[i].flags, false);
        tmixldr_internal_prot_t prot_pad = __conv_prot(si[i].flags, true);

        if (si[i].file.size &&
#ifdef _WIN32
            __win32_mmap_file(base + si[i].off, si[i].file.size,
                        prot_file, hFile, si[i].file.off) == MAP_FAILED) {
#else
            mmap(base + si[i].off, si[i].file.size, prot_file,
                    MAP_FIXED | MAP_PRIVATE, fd, si[i].file.off) == MAP_FAILED) {
#endif
error:
#if defined(_WIN32)
            // dummy statement after jump label to make compiler happy
            ;

            // unlike Cygwin, win32 mmap remembers memory mapping by its start,
            // so we have to free each of them separately
            size_t j;

            for (j = 0; j < i; j++) {
                UnmapViewOfFile(base + si[j].off);

                if (si[j].pad.size)
                    VirtualFree(base + si[j].off + si[j].pad.off, si[j].pad.size, MEM_RELEASE);
            }
#else
            munmap(base, ei->mem_size);  // release reserved memory
#endif
            goto quit;
        }

        if (si[i].pad.size &&
#ifdef _WIN32
            VirtualAlloc(base + si[i].off + si[i].pad.off, si[i].pad.size,
                         MEM_RESERVE | MEM_COMMIT, prot_pad.prot) == MAP_FAILED) {

            // if failed, unmap previously mapped file mapping before quit
            UnmapViewOfFile(base + si[i].off);
#else
            mmap(base + si[i].off + si[i].pad.off, si[i].pad.size, prot_pad,
                 MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0) == MAP_FAILED) {
#endif
            goto error;
        }
    }

#ifdef _WIN32
    // close previously reopened handle
    CloseHandle(hFile);
#endif

    e->base = base;

    if (ei->entry)
        e->entry = e->base + ei->entry;

    return 0;  // success
}

void tmixldr_unload_elf(tmixldr_elf *e, const tmixelf_info *ei) {
    if (!e->base)
        return;  // seems already unloaded

#ifdef _WIN32
    // still the same thing...

    tmixelf_seg *si = ei->segs.data;  // array

    if (ei->segs.size) {
        assert(si);

        size_t i;

        for (i = 0; i < ei->segs.size; i++) {
            UnmapViewOfFile(e->base + si[i].off);

            if (si[i].pad.size)
                VirtualFree(e->base + si[i].off + si[i].pad.off, si[i].pad.size, MEM_RELEASE);
        }
    }
#else
    munmap(e->base, ei->mem_size);
#endif
    e->base = NULL;
}
