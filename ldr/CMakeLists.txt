pkg_search_module(libelf REQUIRED libelf)

add_executable(tmixldr
    dynld.c
    elf/debug.c
    elf/parse.c
    load.c
    main.c)
if (LINUX)
    target_link_options(tmixldr PRIVATE
        -Wl,-rpath=$ORIGIN/../lib)
endif()
target_link_libraries(tmixldr
    commonhelpers)

install(TARGETS tmixldr
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
