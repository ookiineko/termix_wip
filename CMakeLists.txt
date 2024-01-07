cmake_minimum_required(VERSION 3.20)
project(termix LANGUAGES C)

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
# use LLD if available
#
option(TERMIX_PREFER_LLD_AS_LINKER "try to use LLVM LLD for linking" ON)
if (TERMIX_PREFER_LLD_AS_LINKER AND CMAKE_C_COMPILER_ID MATCHES "Clang")
  # only try to use LLD with Clang
  # other compilers may not support it correctly

  find_program(LLD_EXECUTABLE lld)
  if (LLD_EXECUTABLE)
    message(STATUS "Found LLD: ${LLD_EXECUTABLE}")
    set(CMAKE_REQUIRED_LINK_OPTIONS -fuse-ld=lld)
    add_link_options(${CMAKE_REQUIRED_LINK_OPTIONS})
  endif()
endif()

#
# LTO
#
option(TERMIX_USE_LTO "enable link time optimization" ON)
if (TERMIX_USE_LTO)
  CHECK_C_COMPILER_FLAG("-flto" C_LTO_WORKS)
  if (C_LTO_WORKS)
    message(STATUS "LTO is available")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else()
    message(STATUS "LTO is NOT available")
  endif()
else()
  message(STATUS "LTO is DISABLED")
endif()

add_compile_options(-Wall -Wextra -Werror)

include(GNUInstallDirs)
set(TERMIX_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/termix")

add_subdirectory(common)
add_subdirectory(tests)
add_subdirectory(ldr)
