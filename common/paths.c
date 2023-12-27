/*
  paths.c - Path helpers

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
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>  // for readlink
#endif

#include "../inc/paths.h"

char *___tmix_progdir = NULL;

static char __progdir_buff[PATH_MAX + 1];

char *_tmix_join_path(const char *a, const char *b) {
    size_t newlen = strlen(a) + 1 + strlen(b);

    if (newlen > PATH_MAX) {
        errno = ENAMETOOLONG;
        return NULL;
    }

    char *buff = malloc(newlen + 1);

    if (!buff)
        return NULL;

    snprintf(buff, newlen + 1, "%s/%s", a, b);

    return buff;
}

__attribute__((constructor)) static void __init_progdir(void) {
#ifdef _WIN32
    if (!GetModuleFileName(NULL, __progdir_buff, sizeof(__progdir_buff))) {
        // TODO: use FormatMessage to print human readable error message
        fprintf(stderr, "error getting self location: WinError %ld\n", GetLastError());

        return;
    }
#else
    ssize_t len;

    len = readlink("/proc/self/exe", __progdir_buff, sizeof(__progdir_buff));

    if (len < 0) {
        perror("error getting self location");

        return;
    }

    __progdir_buff[len] = '\0';
#endif

    ___tmix_progdir = dirname(__progdir_buff);
}
