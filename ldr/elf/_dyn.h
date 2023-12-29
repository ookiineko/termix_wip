/*
  _dyn.h - ELF dynamic segment

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

#ifndef TERMIX_LOADER_ELF_INTERNAL_DYN_H
#define TERMIX_LOADER_ELF_INTERNAL_DYN_H

#include "_types.h"

int _tmixelf_internal_parse_dyn(int fd, const _ElfXX_Phdr *phdr);

#endif /* TERMIX_LOADER_ELF_INTERNAL_DYN_H */
