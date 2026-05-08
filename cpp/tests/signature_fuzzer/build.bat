call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
mkdir build\x64
clang-cl signature_fuzzer.cpp /I..\..\amoeba /std:c++20 /EHsc /Zi /Od -fsanitize=fuzzer -fprofile-instr-generate -fcoverage-mapping /Fobuild\x64\ /Febuild\x64\signature_fuzzer.exe

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat"
mkdir build\x86
clang-cl signature_fuzzer.cpp /I..\..\amoeba /std:c++20 /EHsc /Zi /Od -fsanitize=fuzzer -fprofile-instr-generate -fcoverage-mapping /Fobuild\x86\ /Febuild\x86\signature_fuzzer.exe
