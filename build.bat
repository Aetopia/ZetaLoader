@echo off
gcc -shared -s src\Zeta.c -lshcore -ntdll -o Zeta.dll
gcc -mwindows -s src\ZetaLoader.c -o ZetaLoader.exe
:: Optional UPX compression.
upx --best --ultra-brute Zeta.dll ZetaLoader.exe>nul 2>&1