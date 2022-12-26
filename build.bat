@echo off
gcc -mwindows -s -Ofast src\ZetaLoader.c -lshcore -o ZetaLoader.exe
:: Optional UPX compression.
upx --best --ultra-brute ZetaLoader.exe>nul 2>&1