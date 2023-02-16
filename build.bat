@echo off
cd "%~dp0"
nimpretty src/main.nim>nul 2>&1
nim c --mm:arc -d:release -d:strip --app:lib -d:strip -o:wininet.dll src/main.nim
upx --best --ultra-brute wininet.dll>nul 2>&1