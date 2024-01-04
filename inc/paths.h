/*
  paths.h - Path helpers

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

#ifndef TERMIX_COMMON_INCLUDE_PATHS_H
#define TERMIX_COMMON_INCLUDE_PATHS_H

/*
 * directory storing the runtime libraries (relative to the bindir)
 */
#if defined(_WIN32) || defined(__CYGWIN__)
#  define _TMIX_LIBPATH                 "."
#  ifdef __CYGWIN__
#    ifdef __MSYS__
#      define _TMIX_SHLIB_PREFIX        "msys-"
#    else
#      define _TMIX_SHLIB_PREFIX        "cyg"
#    endif
#  elif defined(__MINGW32__)
#    define _TMIX_SHLIB_PREFIX          "lib"
#  else
#    define _TMIX_SHLIB_PREFIX          ""
#  endif
#  define _TMIX_SHLIB_SUFFIX            ".dll"
#else
#  define _TMIX_LIBPATH                 "../lib"
#  define _TMIX_SHLIB_PREFIX            "lib"
#  ifdef __APPLE__
#    define _TMIX_SHLIB_SUFFIX          ".dylib"
#  else
#    define _TMIX_SHLIB_SUFFIX          ".so"
#  endif
#endif

extern char *___tmix_progdir;  // dont use directly

/*
 * absolute path of program directory (bindir)
 * NOTE: this variable might be NULL if it was failed to initialize internally
 */
#define _tmix_progdir              ((const char *)___tmix_progdir)

/*
 * returns the concatenated path (caller should free after use)
 *
 * returns NULL if failed
 */
char *_tmix_join_path(const char *a, const char *b);

#endif /* TERMIX_COMMON_INCLUDE_PATHS_H */
