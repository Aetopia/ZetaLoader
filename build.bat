@echo off
cd "%~dp0"
gcc -shared -s -Os ZetaLoader.c -lntdll -ldwmapi -o dinput8.dll
upx --best --ultra-brute dinput8.dll>nul 2>&1