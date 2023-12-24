set(TMIXTEST_INSTALL_DATADIR "${TERMIX_INSTALL_DATADIR}/tests")

add_library(hostlib SHARED
    lib/hello.c)

install(TARGETS hostlib
        RUNTIME DESTINATION ${TMIXTEST_INSTALL_DATADIR})

# test ELFs compiled on Linux
if (LINUX)
    add_executable(hello_standalone
        hello_main.c)
    target_link_libraries(hello_standalone
        hostlib)
    target_link_options(hello_standalone PRIVATE
        -nostdlib
        -Wl,-rpath=$ORIGIN)

    # for early tests w/o dynamic linker
    add_executable(hello_bare
        lib/hello_bare.c
        hello_main.c)
    target_link_options(hello_bare PRIVATE
        -nostdlib)

    install(TARGETS hello_bare hello_standalone
            LIBRARY DESTINATION ${TMIXTEST_INSTALL_DATADIR})
endif()
