#include <assert.h>
#include <errno.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "../inc/paths.h"

#include "elf/elf.h"
#include "dyld.h"

// FIXME: Set to libc path
#define _HOSTLIB_PATH               "../share/termix/tests/tmixhostlib.dll"

static void *__hostlib = NULL;

int tmixdyld_reloc_elf(void *base, const tmixelf_info *ei) {
    if (!__hostlib) {
        // dylib handle was failed to open
        errno = EAGAIN;
        return -1;
    }

    if (ei->reloc.size) {
        assert(ei->reloc.data);

        // TODO
    }

    if (ei->reloc.size) {
        int i;
        tmix_chunk *relros = ei->relro.data;  // array

        assert(relros);

        for (i = 0; i < ei->relro.size; i++) {
            if (mprotect(base + relros[i].off, relros[i].size, PROT_READ) < 0)
                return -1;
        }
    }

    return 0;
}

__attribute__((constructor)) void __init_hostlib(void) {
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
            fprintf(stderr, "%s\n", err);
        else
            fprintf(stderr, "unknown error while opening shared library %s\n", hostlib_path);  // how
    }

    free(hostlib_path);
}

__attribute__((destructor)) void __destroy_hostlib(void) {
    if (__hostlib) {
        dlclose(__hostlib);
        __hostlib = NULL;
    }
}
