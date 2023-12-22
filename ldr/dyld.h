#ifndef TERMIX_LOADER_DYNAMIC_LINKER_H
#define TERMIX_LOADER_DYNAMIC_LINKER_H

#include "elf/elf.h"

/*
 * base - address of the first loaded segment
 * ei - information of the loaded elf
 *
 * returns 0 if succeed, otherwise -1 and sets errno
 *
 * NOTE: the behavior calling this function more than once on the same loaded image is undefined
 */
int tmixdyld_reloc_elf(void *base, const tmixelf_info *ei);

#endif /* TERMIX_LOADER_DYNAMIC_LINKER_H */
