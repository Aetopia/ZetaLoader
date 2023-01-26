cd "%~dp0"
@echo off

:: Format source code files with Nimpretty.
nimpretty src/main.nim src/dll.nim>nul 2>&1

:: ZetaLoader's dynamic link library needs be in the "src" folder in ordered be embedded into the executable.
nim c -d:release -d:strip --app:lib -d:strip -o:src/ZetaLoader.dll src/ZetaLoader.nim
upx --best --ultra-brute src/ZetaLoader.dll>nul 2>&1

:: Compile ZetaLoader's injector with dynamic link library.
nim c -d:release -d:strip --app:gui -o:DumpTool.exe src/DumpTool.nim
upx --best --ultra-brute DumpTool.exe>nul 2>&1

:: Cleanup.
del src\ZetaLoader.dll