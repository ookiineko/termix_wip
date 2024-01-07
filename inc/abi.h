/*
  abi.h - ABI macros

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

#ifndef TERMIX_COMMON_INCLUDE_ABI_H
#define TERMIX_COMMON_INCLUDE_ABI_H

// use System V calling convertion on x86_64 Windows
#if defined(__x86_64__) && (defined(_WIN32) || defined(__CYGWIN__))
#  define __tmixabi     __attribute__((sysv_abi))
#else
#  define __tmixabi
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#  define __tmixapi_import    __attribute__((dllimport))
#  define __tmixapi_export    __attribute__((dllexport))
#else
#  define __tmixapi_import
#  define __tmixapi_export    __attribute__((visibility ("default")))
#endif

#endif /* TERMIX_COMMON_INCLUDE_ABI_H */
