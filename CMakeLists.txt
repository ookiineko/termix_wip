cmake_minimum_required(VERSION 3.20)
project(termix LANGUAGES C)

set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "tmix")

set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# mainly for test ELFs to work on Windows
# also allow running Termix on itself on Windows
if (LINUX)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-z,max-page-size=0x10000")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=0x10000")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,max-page-size=0x10000")
endif()

include(GNUInstallDirs)

set(TERMIX_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/termix")

add_compile_options(-Wall -Wextra -Werror)

add_subdirectory(common)
add_subdirectory(tests)
add_subdirectory(ldr)
