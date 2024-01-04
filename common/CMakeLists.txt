add_library(tmixcommon SHARED
    paths.c)

install(TARGETS tmixcommon
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
