# SPDX-License-Identifier: MIT
#
# Copyright 2023 alturmann1729
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# The build system structure and ideas are influenced by the guidelines and
# principles laid out in the following resources:
#
# * https://cliutils.gitlab.io/modern-cmake/
# * https://cmake.org/cmake/help/book/mastering-cmake/

# The toolchain will only ever contain v3.25.3 as of this revision.
#
# The FATAL_ERROR option is accepted but ignored by CMake 2.6 and higher. It
# should be specified so CMake versions 2.4 and lower fail with an error instead
# of just a warning.
cmake_minimum_required(VERSION 3.25.3 FATAL_ERROR)

# Allow the `project()` command to specify the project version.
cmake_policy(SET CMP0048 NEW)

project(samething
        VERSION 1.0.0.0
        DESCRIPTION "Specific Area Message Encoding (SAME) header generator"
        HOMEPAGE_URL "https://github.com/alturmann1729/samething"
        LANGUAGES CXX)

# Make sure that an out of source build is enforced.
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR "You cannot build this project in-tree. Create a build "
                      "directory outside of the source code and run "
                      "\"cmake -S ${CMAKE_SOURCE_DIR} -B .\" from there.")
endif()

# Make sure that a toolchain file was specified.
if (NOT CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "You must specify a toolchain file.")
endif()

# XXX: Perhaps we should check if the toolchain is 100% correct to what we
# expect, but that's unnecessary right now.

# Support organizing targets into folders in IDEs. This isn't really too
# necessary as most modern IDEs have native support for CMake, but it isn't
# wrong to support other workflows.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set the default build type if none was specified.
# https://www.kitware.com/cmake-and-the-default-build-type/
set(BUILD_TYPE_DEFAULT "Release")

if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
  set(BUILD_TYPE_DEFAULT "Debug")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${BUILD_TYPE_DEFAULT}' as none was "
                 "specified.")

  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY VALUE “${BUILD_TYPE_DEFAULT}”)

  # Set the possible values of build type for cmake-gui.
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
               "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# All targets will have to conform to our specified build options, so let's
# handle that here.
add_library(samething-build-options INTERFACE)

option(SAMETHING_IPO_ENABLE "Enable inter-procedural optimization" OFF)
option(SAMETHING_TESTS_BUILD "Build unit tests (forcibly enables header validity checks)" OFF)
option(SAMETHING_ENFORCE_VALIDITY "Enforce header validity at core level" OFF)

include(CMakeDependentOption)

# Forcibly enable header validity checking if unit tests are being built.
cmake_dependent_option(SAMETHING_ENFORCE_VALIDITY
                       "Enforce header validity at core level" ON
                       "SAMETHING_TESTS_BUILD" OFF)

# All targets under our control will use C++17 with extensions enabled.
set_target_properties(samething-build-options PROPERTIES
                      CXX_STANDARD 17
                      CXX_STANDARD_REQUIRED YES
                      CXX_EXTENSIONS ON)

# These flags are set regardless of the build type.
target_compile_options(samething-build-options INTERFACE -fno-asynchronous-unwind-tables
                                                         -fno-exceptions
                                                         -fno-rtti
                                                         -fno-unwind-tables
                                                         -pipe)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(samething-build-options INTERFACE -glldb
                                                           -Wall
                                                           -Werror
                                                           -Wextra)
else()
  if (SAMETHING_IPO_ENABLE)
    # We assume IPO is supported, since our toolchains are quite modern.
    set_property(TARGET samething-build-options
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
endif()

if (SAMETHING_TESTS_BUILD)
  enable_testing()
endif()

add_subdirectory(src)
