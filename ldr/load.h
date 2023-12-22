#ifndef TERMIX_LOADER_LOAD_H
#define TERMIX_LOADER_LOAD_H

#include "../inc/macros.h"

#include "elf/elf.h"

/*
 * information about a loaded ELF in memory
 *
 * initialize this struct with zero
 */
_tmix_typedef(struct, ldr_elf) {
    void *base;  // the address of the first segment
    void (*entry)(void);  // ELF entrypoint function pointer
};
_tmix_typedef_end(struct, ldr_elf);

/*
 * fd - read-only file descriptor referencing and opened ELF file
 * ei - buffer holding information about the previously parsed ELF file
 * e - output buffer
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 *
 * NOTE: if the function failed, no ELF data is mapped to memory
 */
int tmixldr_load_elf(int fd, const tmixelf_info *ei, tmixldr_elf *e);

/*
 * e - information about the loaded ELF
 * ei - the ELF header information which used for loading previously
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 */
void tmixldr_unload_elf(tmixldr_elf *e, const tmixelf_info *ei);

#endif /* TERMIX_LOADER_LOAD_H */
