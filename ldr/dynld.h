/*
  dynld.h - Dynamic linker

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

#ifndef TERMIX_LOADER_DYNAMIC_LINKER_H
#define TERMIX_LOADER_DYNAMIC_LINKER_H

#include "../inc/abi.h"

#include "elf/elf.h"

#ifdef __clangd__
   // for making IDE happy
#  define _tmixldr_api
#else
#  ifdef TMIX_BUILDING_LOADER_SHLIB
#    define _tmixldr_api      __tmixapi_export
#  else
#    define _tmixldr_api      __tmixapi_import
#  endif
#endif

/*
 * base - address of the first loaded segment
 * ei - information of the loaded elf
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 *
 * NOTE: the behavior calling this function more than once on the same loaded image is undefined
 */
_tmixldr_api int tmixdynld_handle_elf(void *base, const tmixelf_info *ei);

#endif /* TERMIX_LOADER_DYNAMIC_LINKER_H */
