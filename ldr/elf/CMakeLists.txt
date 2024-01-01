add_library(elf SHARED
    dyn.c
    info.c
    segs.c
    symtab.c)

install(TARGETS elf
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
