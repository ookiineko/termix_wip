#ifndef TERMIX_COMMON_INCLUDE_TYPES_H
#define TERMIX_COMMON_INCLUDE_TYPES_H

#include <sys/types.h>

#include "macros.h"

/*
 * describe the location and size of a chunk of data
 */
_tmix_typedef(struct, _chunk) {
    size_t off;
    size_t size;
};
_tmix_typedef_end(struct, _chunk);

/*
 * array with size of arbitrary kind of data
 */
_tmix_typedef(struct, _array) {
    void *data;
    size_t size;
};
_tmix_typedef_end(struct, _array);

#endif /* TERMIX_COMMON_INCLUDE_TYPES_H */
