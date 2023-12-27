## the problem

Termix doesn't trap Linux syscalls, as a result it cannot load statically (or static-PIE) linked or any program tries to make directly system calls (like programs written in Golang) no matter for what purpose.

And unfortunately the runtime dynamic linker (which is in charge of applying the runtime relocations) falls into such category.

For now as a workaround, Termix will try to implement a simple and minimal `ld.so` replacement to make things work.
