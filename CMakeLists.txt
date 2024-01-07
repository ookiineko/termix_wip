cmake_minimum_required(VERSION 3.20)

#
# prefer Clang if available
#
option(WITHOUT_CLANG "Do not try to use Clang even if it's present" OFF)
if (WITHOUT_CLANG)
  message(STATUS "Clang is DISABLED")
else()
  find_program(CLANG_EXECUTABLE clang)
  if (CLANG_EXECUTABLE)
    message(STATUS "Found Clang: ${CLANG_EXECUTABLE}")
    set(CMAKE_C_COMPILER "clang")
  else()
    message(STATUS "CANNOT find Clang")
  endif()
endif()

project(termix LANGUAGES C)

#
# detect Clang on Cygwin
#
if (CYGWIN AND CLANG_EXECUTABLE)
  message(FATAL_ERROR "\
    Using Clang on Cygwin is NOT supported and NOT recommended,
    please pass -DWITHOUT_CLANG=ON to CMake and reconfigure.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

#
# try to hide local symbols when possible
#
include(CheckCCompilerFlag)
CHECK_C_COMPILER_FLAG("-fvisibility=hidden" C_VISIBILITY_EQ_HIDDEN_WORKS)
if (C_VISIBILITY_EQ_HIDDEN_WORKS)
  add_compile_options(-fvisibility=hidden)
endif()

#
# prefer LLD if available
#
option(WITHOUT_LLD "Do not try to use LLD even if available" OFF)
if (CLANG_EXECUTABLE AND NOT WITHOUT_LLD)
  find_program(LLD_EXECUTABLE lld)
  if (LLD_EXECUTABLE)
    message(STATUS "Found LLD: ${LLD_EXECUTABLE}")
    set(CMAKE_REQUIRED_LINK_OPTIONS -fuse-ld=lld)
    add_link_options(${CMAKE_REQUIRED_LINK_OPTIONS})
  else()
    message(STATUS "CANNOT find LLD")
  endif()
else()
  # only try to use LLD with Clang
  # other compilers may not support it correctly
  message(STATUS "LLD is DISABLED")
endif()

#
# prefer lto is available
#
option(WITHOUT_LTO "Do not try to use LTO even if available" OFF)
if (WITHOUT_LTO)
  message(STATUS "LTO is DISABLED")
else()
  CHECK_C_COMPILER_FLAG("-flto" C_LTO_WORKS)
  if (C_LTO_WORKS)
    message(STATUS "LTO is available")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "LTO is NOT available")
  endif()
endif()

add_compile_options(-Wall -Wextra -Werror)

include(GNUInstallDirs)
set(TERMIX_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/termix")

add_subdirectory(common)
add_subdirectory(tests)
add_subdirectory(ldr)
