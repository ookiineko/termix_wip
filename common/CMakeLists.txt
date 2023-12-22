add_library(commonhelpers SHARED
    paths.c)

install(TARGETS commonhelpers
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
