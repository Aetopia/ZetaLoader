@echo off
cd "%~dp0"

:: Format source code files with Nimpretty.
nimpretty src/ZetaLoader.nim>nul 2>&1

:: ZetaLoader's dynamic link library needs be in the "src" folder in ordered be embedded into the executable.
nim c --mm:arc -d:release -d:strip --app:lib -d:strip -o:ZetaLoader.dll src/ZetaLoader.nim
upx --best --ultra-brute ZetaLoader.dll>nul 2>&1