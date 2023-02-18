@echo off
cd "%~dp0"
nimpretty src/main.nim>nul 2>&1
nim c -d:danger -d:useMalloc --mm:arc --opt:size -d:noRes -d:strip --app:lib -o:dinput8.dll src/main.nim
upx --best --ultra-brute dinput8.dll>nul 2>&1