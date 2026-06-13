mkdir coverage
cd coverage

set LLVM_PROFILE_FILE=x64_native-%%p.profraw
..\build\x64\fuzzer.exe -max_total_time=60 -fork=8
set LLVM_PROFILE_FILE=x86_native-%%p.profraw
..\build\x86\fuzzer.exe -max_total_time=60 -fork=8
