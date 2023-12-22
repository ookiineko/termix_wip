#ifndef TERMIX_TESTS_HOSTLIB_SYSCALLS
#define TERMIX_TESTS_HOSTLIB_SYSCALLS

#include <sys/types.h>
#include <syscall.h>

// syscall wrappers from musl

#ifdef __i386__
static inline long __syscall1(long n, long a1) {
    unsigned long __ret;
    __asm__ __volatile__ ("xchg %%ebx,%%edx ; int $128 ; xchg %%ebx,%%edx" : "=a"(__ret) : "a"(n), "d"(a1) : "memory");
    return __ret;
}

static inline long __syscall3(long n, long a1, long a2, long a3) {
    unsigned long __ret;
    __asm__ __volatile__ ("xchg %%ebx,%%edi ; int $128 ; xchg %%ebx,%%edi" : "=a"(__ret) : "a"(n), "D"(a1), "c"(a2), "d"(a3) : "memory");
    return __ret;
}
#elif defined(__x86_64__)
static inline long __syscall1(long n, long a1) {
    unsigned long ret;
    __asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

static inline long __syscall3(long n, long a1, long a2, long a3) {
    unsigned long ret;
    __asm__ __volatile__ ("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2),
                          "d"(a3) : "rcx", "r11", "memory");
    return ret;
}
#elif defined(__arm__)
#define __asm_syscall(...) do {             \
    __asm__ __volatile__ ( "svc 0"          \
    : "=r"(r0) : __VA_ARGS__ : "memory");   \
    return r0;                              \
    } while (0)

static inline long __syscall1(long n, long a) {
    register long r7 __asm__("r7") = n;
    register long r0 __asm__("r0") = a;
    __asm_syscall("r"(r7), "0"(r0));
}

static inline long __syscall3(long n, long a, long b, long c) {
    register long r7 __asm__("r7") = n;
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    __asm_syscall("r"(r7), "0"(r0), "r"(r1), "r"(r2));
}
#elif defined(__aarch64__)
#define __asm_syscall(...) do {                 \
    __asm__ __volatile__ ( "svc 0"              \
    : "=r"(x0) : __VA_ARGS__ : "memory", "cc"); \
    return x0;                                  \
    } while (0)

static inline long __syscall1(long n, long a)　{
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    __asm_syscall("r"(x8), "0"(x0));
}

static inline long __syscall3(long n, long a, long b, long c) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    __asm_syscall("r"(x8), "0"(x0), "r"(x1), "r"(x2));
}
#else
#error Dont know how to do raw system call on this architecture yet
#endif

__attribute__((noreturn)) static inline void __exit(int status) {
    for (;;)
        __syscall1(__NR_exit, status);  // noreturn
}

static inline ssize_t __write(int fd, const void* buf, size_t count) {
    return __syscall3(__NR_write, (long) fd, (long) buf, (long) count);
}

#endif /* TERMIX_TESTS_HOSTLIB_SYSCALLS */
