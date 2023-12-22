/*
  macros.h - Helper macros

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

#ifndef TERMIX_COMMON_INCLUDE_MACROS_H
#define TERMIX_COMMON_INCLUDE_MACROS_H

/*
 * macro for defining structs and etc. with Termix prefix and typedefs
 *
 * must be paired when using
 */
#define _tmix_typedef(_type, _name)         _type _tmix##_name
#define _tmix_typedef_end(_type, _name)     typedef _type _tmix##_name tmix##_name

#endif /* TERMIX_COMMON_INCLUDE_MACROS_H */
