set(TMIXTEST_INSTALL_DATADIR "${TERMIX_INSTALL_DATADIR}/tests")

add_library(tmixfakelibc SHARED
    lib/hello.c)
target_compile_definitions(tmixfakelibc PRIVATE
    TMIX_BUILDING_LIBC_SHLIB)

install(TARGETS tmixfakelibc
        RUNTIME DESTINATION ${TMIXTEST_INSTALL_DATADIR}
        LIBRARY DESTINATION ${TMIXTEST_INSTALL_DATADIR})

# test ELFs compiled on Linux
if (LINUX)
    add_executable(hello_standalone
        hello_main.c)
    target_link_libraries(hello_standalone
        tmixfakelibc)
    target_link_options(hello_standalone PRIVATE
        -nostartfiles)

    # for early tests w/o dynamic linker
    add_executable(hello_bare
        lib/linux/hello_bare.c
        hello_main.c)
    target_link_options(hello_bare PRIVATE
        -nostartfiles)

    install(TARGETS hello_bare hello_standalone
            RUNTIME DESTINATION ${TMIXTEST_INSTALL_DATADIR})
endif()
