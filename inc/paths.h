#ifndef TERMIX_COMMON_INCLUDE_PATHS_H
#define TERMIX_COMMON_INCLUDE_PATHS_H

/*
 * directory storing the runtime libraries (relative to the bindir)
 */
#ifdef __CYGWIN__
#define _TMIX_LIBPATH                "."
#else
#define _TMIX_LIBPATH                "../lib"
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
