@echo off
gcc -mwindows -s -Ofast src\main.c -o ZetaLoader.exe
gcc -shared -s -Ofast "dll.c" -lshcore -lntdll -o ZetaLoader.dll
:: Optional UPX compression.
upx --best --ultra-brute ZetaLoader.dll ZetaLoader.exe>nul 2>&1