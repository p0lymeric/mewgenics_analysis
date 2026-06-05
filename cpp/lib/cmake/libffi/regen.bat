@rem Yes, we need cl.exe and GNU Autotools, in the same Bourne shell. It's that straightforward (of a requirement).
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
call C:\msys64\msys2_shell.cmd -use-full-path -ucrt64 -defterm -no-start -here -c "./regen.sh"
pause
