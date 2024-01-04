add_library(tmixelf SHARED
    dyn.c
    info.c
    segs.c
    symtab.c)

install(TARGETS tmixelf
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
