call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

rem llvm-profdata merge -sparse *.profraw -o default.profdata
rem llvm-cov show fuzzer.exe -instr-profile=default.profdata -format=html -output-dir=coverage_report

cd coverage

llvm-profdata merge -sparse x86_native*.profraw -o x86_native.profdata
llvm-cov show ..\build\x86\fuzzer.exe -instr-profile=x86_native.profdata -format=html -output-dir=x86_native

llvm-profdata merge -sparse x64_native*.profraw -o x64_native.profdata
llvm-cov show ..\build\x64\fuzzer.exe -instr-profile=x64_native.profdata -format=html -output-dir=x64_native
