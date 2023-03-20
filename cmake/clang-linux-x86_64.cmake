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

# This toolchain file is configured to use the provided LLVM toolchain for use
# on Linux systems to target the x86_64 architecture and generate ELF
# executables.
#
# If you followed the setup guide, you shouldn't have to change anything here.
# Otherwise, you are responsible for changing the paths appropriately.

set(CMAKE_SYSTEM_NAME Linux)

set(CMAKE_C_COMPILER
    "~/samething-toolchain/llvm/bin/clang" CACHE FILEPATH "LLVM C compiler")

set(CMAKE_CXX_COMPILER
    "~/samething-toolchain/llvm/bin/clang++" CACHE FILEPATH "LLVM C++ compiler")

set(CMAKE_LINKER
    "~/samething-toolchain/llvm/bin/ld.lld" CACHE FILEPATH "LLVM linker")

set(CMAKE_AR
    "~/samething-toolchain/llvm/bin/llvm-ar" CACHE FILEPATH "LLVM archiver")

set(CMAKE_RANLIB
    "~/samething-toolchain/llvm/bin/llvm-ranlib" CACHE FILEPATH "LLVM ranlib")
