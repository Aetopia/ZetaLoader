@echo off
gcc -shared -s -Ofast src\Zeta.c -lshcore -o Zeta.dll
gcc -mwindows -s -Ofast src\ZetaLoader.c -o ZetaLoader.exe
:: Optional UPX compression.
upx --best --ultra-brute Zeta.dll ZetaLoader.exe>nul 2>&1