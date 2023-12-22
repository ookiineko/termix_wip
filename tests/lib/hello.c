#include <stdio.h>
#include <stdlib.h>

#include "hello.h"

void _foo() {
    printf("Hello, world!\n");
    exit(EXIT_SUCCESS);  // noreturn
}
