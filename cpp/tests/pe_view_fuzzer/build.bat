call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
mkdir build\x64
clang-cl fuzzer.cpp ..\..\amoeba\utilities\pe_view.cpp /I..\..\amoeba /std:c++20 /EHsc /Zi /Od -fsanitize=fuzzer -fprofile-instr-generate -fcoverage-mapping /Fobuild\x64\ /Febuild\x64\fuzzer.exe

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars32.bat"
mkdir build\x86
clang-cl fuzzer.cpp ..\..\amoeba\utilities\pe_view.cpp /I..\..\amoeba /std:c++20 /EHsc /Zi /Od -fsanitize=fuzzer -fprofile-instr-generate -fcoverage-mapping /Fobuild\x86\ /Febuild\x86\fuzzer.exe
