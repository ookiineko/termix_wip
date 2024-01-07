#ifndef TERMIX_TESTS_HOSTLIB_HELLO
#define TERMIX_TESTS_HOSTLIB_HELLO

#include "../../inc/abi.h"

#ifdef TMIX_BUILDING_LIBC_SHLIB
#  define __tmixlibc_api        __tmixapi_export
#else
#  define __tmixlibc_api        __tmixapi_import
#endif

__attribute__((noreturn)) __tmixlibc_api __tmixabi void _foo();

#endif /* TERMIX_TEST_HOSTLIB_HELLO */
