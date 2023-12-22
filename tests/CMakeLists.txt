set(TMIXTEST_INSTALL_DATADIR "${TERMIX_INSTALL_DATADIR}/tests")

# test ELFs compiled on Linux
if (LINUX)
    add_library(hostlib SHARED
        lib/hello.c)

    add_executable(hello_standalone
        hello_main.c)
    target_link_libraries(hello_standalone
        hostlib)
    target_link_options(hello_standalone PRIVATE
        -nostdlib
        -Wl,-rpath=$ORIGIN)

    # for early tests w/o dyld
    add_executable(hello_bare
        lib/hello_bare.c
        hello_main.c)
    target_link_options(hello_bare PRIVATE
        -nostdlib)

    install(TARGETS hostlib hello_bare hello_standalone
            LIBRARY DESTINATION ${TMIXTEST_INSTALL_DATADIR}
            RUNTIME DESTINATION ${TMIXTEST_INSTALL_DATADIR})

endif()
