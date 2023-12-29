add_subdirectory(elf)

add_library(ldr SHARED
    dynld.c
    load.c)
target_link_libraries(ldr
    common
    elf)

add_executable(tmixldr
    main.c)
if (NOT (WIN32 OR CYGWIN))
    target_link_options(tmixldr PRIVATE
        -Wl,-rpath=$ORIGIN/../lib)
endif()
target_link_libraries(tmixldr
    ldr)

install(TARGETS ldr tmixldr
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
