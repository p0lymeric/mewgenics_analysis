#!/bin/sh

# libffi relies on Autotools to generate two header files.
# We generate the two files and check them into our tree.
# They should be good to reuse unless libffi source is updated.

SCRIPT_DIR=`pwd`
TRIPLE=x86_64-w64-mingw32

cd ../../libffi
git apply "${SCRIPT_DIR}/msvcc.sh.patch" # Handle spaces in build path
./autogen.sh
./configure CC="../msvcc.sh -m64" CXX="../msvcc.sh -m64" LD=link CPP="cl -nologo -EP" CXXCPP="cl -nologo -EP" CPPFLAGS="-DFFI_STATIC_BUILD"
# We could possibly use GCC instead of MSVC for autoconf here. The generated files do differ slightly.
# ./configure

cp "${TRIPLE}/fficonfig.h" "${SCRIPT_DIR}/autoconf/"
cp "${TRIPLE}/include/ffi.h" "${SCRIPT_DIR}/autoconf/"

# make distclean
# git checkout msvcc.sh

# git clean -fdx
