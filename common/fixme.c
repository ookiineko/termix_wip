#include <stdarg.h>
#include <stdio.h>

#include "../inc/fixme.h"

void tmix_fixme(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "fixme: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}
