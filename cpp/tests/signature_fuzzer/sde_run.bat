mkdir coverage
cd coverage

set LLVM_PROFILE_FILE=x86_pentium4.profraw
"C:\ETOOLS\sde-external-10.8.0-2026-03-15-win\sde.exe" -p4 -- ..\build\x86\signature_fuzzer.exe -max_total_time=120
set LLVM_PROFILE_FILE=x86_native.profraw
..\build\x86\signature_fuzzer.exe -max_total_time=120

set LLVM_PROFILE_FILE=x64_sandybridge.profraw
"C:\ETOOLS\sde-external-10.8.0-2026-03-15-win\sde.exe" -snb -- ..\build\x64\signature_fuzzer.exe -max_total_time=120
set LLVM_PROFILE_FILE=x64_haswell.profraw
"C:\ETOOLS\sde-external-10.8.0-2026-03-15-win\sde.exe" -hsw -- ..\build\x64\signature_fuzzer.exe -max_total_time=120
set LLVM_PROFILE_FILE=x64_native.profraw
..\build\x64\signature_fuzzer.exe -max_total_time=120
