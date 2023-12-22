/*
  types.h - Common type definitions

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
