pkg_search_module(libelf REQUIRED libelf)

add_library(elf SHARED
    debug.c
    parse.c)

install(TARGETS elf
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
