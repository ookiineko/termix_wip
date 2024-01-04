// only borrow headers, libc won't be linked

#include <stdlib.h>
#include <unistd.h>

#include "syscalls.h"

#include "../hello.h"

void _foo() {
    const char hello[] = "Hello, world!\n";
    __write(STDOUT_FILENO, hello, sizeof(hello));
    __exit(EXIT_SUCCESS);  // noreturn
}
