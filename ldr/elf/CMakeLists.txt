add_library(tmixelf SHARED
    dyn.c
    info.c
    segs.c
    symtab.c)
target_compile_definitions(tmixelf PRIVATE
    TMIX_BUILDING_LIBELF_SHLIB)

install(TARGETS tmixelf
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
