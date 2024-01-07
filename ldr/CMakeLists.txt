add_subdirectory(elf)

add_library(tmixloader SHARED
    dynld.c
    load.c)
target_link_libraries(tmixloader
    tmixcommon
    tmixelf)
target_compile_definitions(tmixloader PRIVATE
    TMIX_BUILDING_LOADER_SHLIB)

add_executable(tmixldr
    main.c)
target_link_libraries(tmixldr
    tmixloader)

install(TARGETS tmixloader tmixldr
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
