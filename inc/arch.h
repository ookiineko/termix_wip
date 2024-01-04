/*
  arch.h - Architecture-specific definitions

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

#ifndef TERMIX_COMMON_INCLUDE_ARCH_H
#define TERMIX_COMMON_INCLUDE_ARCH_H

#if defined(__BIG_ENDIAN__)|| (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#  define TMIX_BIG_ENDIAN
#elif defined(__LITTLE_ENDIAN__)|| (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#  define TMIX_LITTLE_ENDIAN
#else
#  warning Dont know endian-ness on this platform yet
#endif

#if defined(__i386__) || defined(__arm__)
#  define TMIX32
#elif defined(__x86_64__) || defined(__aarch64__)
#  define TMIX64
#else
#  warning Dont know word size on this platform yet
#endif

#endif /* TERMIX_COMMON_INCLUDE_ARCH_H */
