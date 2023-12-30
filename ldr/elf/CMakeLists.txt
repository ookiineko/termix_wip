pkg_search_module(libelf REQUIRED libelf)

add_library(elf SHARED
    dyn.c
    info.c
    segs.c
    reloc.c)

install(TARGETS elf
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
