pkg_search_module(REQUIRED libelf)

add_executable(tmixldr
    dynld.c
    elf/debug.c
    elf/parse.c
    load.c
    main.c)
target_link_options(tmixldr PRIVATE
    -Wl,-rpath=$ORIGIN/../lib)
target_link_libraries(tmixldr
    commonhelpers)

install(TARGETS tmixldr
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
