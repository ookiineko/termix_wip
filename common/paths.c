#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__linux__) || defined(__CYGWIN__)
#include <unistd.h>  // for readlink
#endif

#include "../inc/paths.h"

char *___tmix_progdir = NULL;

static char __progdir_buff[PATH_MAX + 1];

char *_tmix_join_path(const char *a, const char *b) {
    char *buff = malloc(PATH_MAX + 1);

    if (!buff)
        return NULL;

    snprintf(buff, PATH_MAX + 1, "%s/%s", a, b);

    return buff;
}

__attribute__((constructor)) void __init_progdir(void) {
#ifdef _WIN32
    if (!GetModuleFileName(NULL, __progdir_buff, sizeof(__progdir_buff))) {
        // TODO: use FormatMessage to print human readable error message
        fprintf(stderr, "error getting self location: WinError %ld\n", GetLastError());

        return;
    }
#elif defined(__linux__) || defined(__CYGWIN__)
    ssize_t len;

    len = readlink("/proc/self/exe", __progdir_buff, sizeof(__progdir_buff));

    if (len < 0) {
        perror("error getting self location");

        return;
    }

    __progdir_buff[len] = '\0';
#else
#error Dont know how to fetch self location on this platform yet
#endif

    ___tmix_progdir = dirname(__progdir_buff);
}
