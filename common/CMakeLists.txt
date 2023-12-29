add_library(common SHARED
    paths.c)

install(TARGETS common
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
