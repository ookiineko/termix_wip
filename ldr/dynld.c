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
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "../inc/paths.h"
#include "../inc/types.h"

#include "elf/elf.h"

#include "dynld.h"

#define _LIBC_PATH               "../share/termix/tests/tmixfakelibc.dll"

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

        assert(relocs);

        for (i = 0; i < ei->relocs.size; i++) {
            // FIXME: support other shlibs
            void *the_sym = dlsym(__libc, relocs[i].sym.name);

            if (!the_sym) {
                fprintf(stderr, "missing symbol to relocate: %s\n", relocs[i].sym.name);
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
            if (mprotect(base + relros[i].off, relros[i].size, PROT_READ) < 0)
                return -1;
        }
    }

    return 0;
}

__attribute__((constructor)) static void __init_libc(void) {
    if (!_tmix_progdir)
        return;  // sth went wrong at startup

    char *libc_path = _tmix_join_path(_tmix_progdir, _LIBC_PATH);

    if (!libc_path) {
        // maybe memory error
        perror("error concatenating path for libc");

        return;
    }

    __libc = dlopen(libc_path, RTLD_LAZY);

    if (!__libc) {
        const char *err = dlerror();

        if (err)
            fprintf(stderr, "error while opening shared library: %s\n", err);
        else
            fprintf(stderr, "unknown error while opening shared library %s\n", libc_path);  // how
    }

    free(libc_path);
}

__attribute__((destructor)) static void __destroy_libc(void) {
    if (__libc) {
        dlclose(__libc);
        __libc = NULL;
    }
}
