cmake_minimum_required(VERSION 3.20)
project(termix LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

include(GNUInstallDirs)

set(TERMIX_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/termix")

add_compile_options(-Wall -Wextra -Werror)

add_subdirectory(common)
add_subdirectory(tests)
add_subdirectory(ldr)
