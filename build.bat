@echo off
gcc -shared -Ofast src\Zeta.c -lshcore -o Zeta.dll
gcc -Ofast src\ZetaLoader.c -mwindows -o ZetaLoader.exe
:: Optional UPX compression.
upx --best Zeta.dll ZetaLoader.exe