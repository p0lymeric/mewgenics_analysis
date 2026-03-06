cmake -S . -B ..\out\cmake\win64\

pushd ..\out\cmake\win64\
cmake --build . --config RelWithDebInfo --parallel
cmake --install . --config RelWithDebInfo --prefix=install
popd

pause
