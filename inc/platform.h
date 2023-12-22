#ifndef TERMIX_COMMON_INCLUDE_PLATFORM_H
#define TERMIX_COMMON_INCLUDE_PLATFORM_H

#if defined(__BIG_ENDIAN__)|| (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#define TMIX_BIG_ENDIAN
#endif

#if defined(__i386__) || defined(__arm__)
#define TMIX32
#elif defined(__x86_64__) || defined(__aarch64__)
#define TMIX64
#endif

#endif /* TERMIX_COMMON_INCLUDE_PLATFORM_H */
