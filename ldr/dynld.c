/*
  dynld.c - Dynamic linker

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
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#else
#  include <dlfcn.h>
#  include <sys/mman.h>
#endif

#include "../inc/paths.h"
#include "../inc/types.h"

#include "elf/elf.h"

#include "dynld.h"

#define _LIBC_PATH               "../share/termix/tests/" _TMIX_SHLIB_PREFIX "tmixfakelibc" _TMIX_SHLIB_SUFFIX

static void *__libc = NULL;

int tmixdynld_handle_elf(void *base, const tmixelf_info *ei) {
    if (!__libc) {
        // dylib handle was failed to open
        errno = EAGAIN;
        return -1;
    }

    size_t i;

    if (ei->relocs.size) {
        tmixelf_reloc *relocs = ei->relocs.data;
        tmixelf_sym *syms = ei->syms.data;

        assert(relocs);
        assert(syms);

        for (i = 0; i < ei->relocs.size; i++) {
            // FIXME: support other shlibs
            tmixelf_sym *sym = &syms[relocs[i].symidx];

            assert(sym->imported);

#ifdef _WIN32
            void *the_sym = GetProcAddress(__libc, sym->name);
#else
            void *the_sym = dlsym(__libc, sym->name);
#endif

            if (!the_sym) {
#ifdef _WIN32
                // TODO: use FormatMessage to print human readable error message
                fprintf(stderr, "error while relocating symbol %s: WinError %ld\n", sym->name, GetLastError());
#else
                const char *err = dlerror();

                if (err)
                    fprintf(stderr, "error while relocating symbol %s: %s\n", sym->name, err);
                else
                    fprintf(stderr, "unknown error while relocating symbol %s\n", sym->name);  // how
#endif

                errno = EAGAIN;
                return -1;
            }

            intptr_t *ptr = (intptr_t *)((char *)base + relocs[i].off);

            *ptr = (intptr_t)the_sym;
        }
    }

    if (ei->relros.size) {
        tmix_chunk *relros = ei->relros.data;  // array

        assert(relros);

        for (i = 0; i < ei->relros.size; i++) {
#ifdef _WIN32
            DWORD old_prot = 0;  // unused

            if (!VirtualProtect(base + relros[i].off, relros[i].size, PAGE_READONLY, &old_prot))
#else
            if (mprotect(base + relros[i].off, relros[i].size, PROT_READ) < 0)
#endif
                return -1;
        }
    }

    return 0;
}

__attribute__((constructor)) static void __init_libc(void) {
    char *libc_path = getenv("TMIXDYNLD_LIBC_PATH");

    if (libc_path) {
        libc_path = strdup(libc_path);

        if (!libc_path) {
            perror("error duplicating libc path");

            return;
        }

        goto try_open;
    }

    if (!_tmix_progdir)
        return;  // sth went wrong during startup

    libc_path = _tmix_join_path(_tmix_progdir, _LIBC_PATH);

    if (!libc_path) {
        perror("error concatenating path for libc");

        return;
    }

try_open:
#ifdef _WIN32
    __libc = LoadLibrary(libc_path);
#else
    __libc = dlopen(libc_path, RTLD_LAZY);
#endif

    if (!__libc) {
#ifdef _WIN32
        // TODO: use FormatMessage to print human readable error message
        fprintf(stderr, "error while opening libc: WinError %ld\n", GetLastError());
#else
        const char *err = dlerror();

        if (err)
            fprintf(stderr, "error while opening libc: %s\n", err);
        else
            fprintf(stderr, "unknown error while opening libc\n");
#endif
    }

    free(libc_path);
}

__attribute__((destructor)) static void __destroy_libc(void) {
    if (__libc) {
#ifdef _WIN32
        FreeLibrary(__libc);
#else
        dlclose(__libc);
#endif
        __libc = NULL;
    }
}
