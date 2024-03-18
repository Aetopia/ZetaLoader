@echo off
cd "%~dp0"
g++ -std=c++20 -static -Os -s -shared -municode DllMain.cpp -lOleAut32 -lRuntimeObject -o dpapi.dll
upx --best --ultra-brute dpapi.dll>nul 2>&1