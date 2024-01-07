cmake_minimum_required(VERSION 3.20)

#
# prefer clang if available
#
find_program(CLANG_EXECUTABLE clang)
if (CLANG_EXECUTABLE)
  message(STATUS "Found clang: ${CLANG_EXECUTABLE}")
  set(CMAKE_C_COMPILER "clang")
else()
  message(STATUS "CANNOT find clang")
endif()

project(termix LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-fvisibility=hidden" C_VISIBILITY_EQ_HIDDEN_WORKS)
if (C_VISIBILITY_EQ_HIDDEN_WORKS)
  add_compile_options(-fvisibility=hidden)
endif()

#
# prefer lld if available
#
find_program(LLD_EXECUTABLE lld)
if (LLD_EXECUTABLE)
  message(STATUS "Found lld: ${LLD_EXECUTABLE}")
  set(CMAKE_REQUIRED_LINK_OPTIONS -fuse-ld=lld)
  add_link_options(${CMAKE_REQUIRED_LINK_OPTIONS})
else()
  message(STATUS "CANNOT find lld")
endif()

#
# prefer lto is available
#
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-flto" C_LTO_WORKS)
if (C_LTO_WORKS)
  message(STATUS "LTO is enabled")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(STATUS "LTO is DISABLED")
endif()

add_compile_options(-Wall -Wextra -Werror)

include(GNUInstallDirs)
set(TERMIX_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/termix")

add_subdirectory(common)
add_subdirectory(tests)
add_subdirectory(ldr)
