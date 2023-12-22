add_library(commonhelpers SHARED
    fixme.c
    paths.c)

install(TARGETS commonhelpers
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
