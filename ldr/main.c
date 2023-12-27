/*
  main.c - ELF loader

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
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "dynld.h"
#include "elf/elf.h"
#include "load.h"

static int __fd = -1;  // ELF file
static tmixelf_info __ei = {};
static tmixldr_elf __e = {};

/*
 * entrypoint
 */
int main(int argc, char **argv) {
    const char *path = NULL;
    bool debug = false;
    int c;

    while ((c = getopt(argc, argv, "d")) != -1) {
        switch (c) {
            case 'd':
                debug = true;
                break;
            default:
usage_and_exit:
                fprintf(stderr, "Usage: %s [-d] <elf file>\n", argv[0]);
                goto exit;
                break;
        }
    }

    int i;

    for (i = optind; i < argc; i++) {
        if (path) {
            fprintf(stderr, "only one elf file can be specified at a time\n");
            goto usage_and_exit;
        } else
            path = argv[i];
    }

    if (!path) {
        fprintf(stderr, "must specify an elf file to execute\n");
        goto usage_and_exit;
    }

    __fd = open(path, O_RDONLY);
    if (__fd < 0) {
        perror("error opening ELF");

        goto exit;
    }

    if (tmixldr_parse_elf(__fd, &__ei) < 0) {
        perror("error parsing ELF");

        if (errno == EBADF)
            fprintf(stderr, "the file may not be a vaild ELF, "
                            "or incompatible with this machine\n");

        goto exit;
    }

    if (debug)
        tmixldr_print_elfinfo(&__ei);

    if (tmixldr_load_elf(__fd, &__ei, &__e) < 0) {
        perror("error loading ELF");

        if (errno == EINVAL)
            fprintf(stderr, "the file might not be a loadable ELF\n");

        goto exit;
    }

    // fd can be closed once ELF itself is loaded
    close(__fd);
    __fd = -1;

    if (!__e.entry) {
        fprintf(stderr, "ELF entrypoint in unknown\n");

        goto exit;
    }

    if (tmixdynld_handle_elf(__e.base, &__ei) < 0) {
        perror("error linking ELF");

        goto exit;
    }

    __e.entry();

    fprintf(stderr, "[program returned to loader unexpectedly]\n");

exit:
    return EXIT_FAILURE;
}

__attribute__((destructor)) static void __cleanup(void) {
    // internal cleanups
    // automatically called except the target program mess up and crash itself

    if (!(__fd < 0)) {
        close(__fd);
        __fd = -1;
    }

    if (__e.base)
        tmixldr_unload_elf(&__e, &__ei);

    tmixldr_free_elfinfo(&__ei);
}
