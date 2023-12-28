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
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "../inc/paths.h"

#include "elf/elf.h"

#include "dynld.h"

// FIXME: Set to libc path
#define _HOSTLIB_PATH               "../share/termix/tests/tmixhostlib.dll"

static void *__hostlib = NULL;

int tmixdynld_handle_elf(void *base, const tmixelf_info *ei) {
    if (!__hostlib) {
        // dylib handle was failed to open
        errno = EAGAIN;
        return -1;
    }

    if (ei->relocs.size) {
        assert(ei->relocs.data);

        // TODO
    }

    if (ei->relros.size) {
        int i;
        tmix_chunk *relros = ei->relros.data;  // array

        assert(relros);

        for (i = 0; i < ei->relros.size; i++) {
            if (mprotect(base + relros[i].off, relros[i].size, PROT_READ) < 0)
                return -1;
        }
    }

    return 0;
}

__attribute__((constructor)) static void __init_hostlib(void) {
    if (!_tmix_progdir)
        return;  // sth went wrong at startup

    char *hostlib_path = _tmix_join_path(_tmix_progdir, _HOSTLIB_PATH);

    if (!hostlib_path) {
        // maybe memory error
        perror("error concatenating path for hostlib");

        return;
    }

    __hostlib = dlopen(hostlib_path, RTLD_LAZY);

    if (!__hostlib) {
        const char *err = dlerror();

        if (err)
            fprintf(stderr, "error while opening shared library: %s\n", err);
        else
            fprintf(stderr, "unknown error while opening shared library %s\n", hostlib_path);  // how
    }

    free(hostlib_path);
}

__attribute__((destructor)) static void __destroy_hostlib(void) {
    if (__hostlib) {
        dlclose(__hostlib);
        __hostlib = NULL;
    }
}
