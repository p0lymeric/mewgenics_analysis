#!/bin/sh
if [ -z "$1" ]; then
    exit 1
fi

cmake -S . -B ../out/cmake/$1/ -DCMAKE_BUILD_TYPE=RelWithDebInfo

cd ../out/cmake/$1/
cmake --build . --parallel
cmake --install . --prefix=install
