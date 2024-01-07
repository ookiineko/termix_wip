add_library(tmixcommon SHARED
    paths.c)
target_compile_definitions(tmixcommon PRIVATE
    TMIX_BUILDING_LIBCOMMON_SHLIB)

install(TARGETS tmixcommon
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
