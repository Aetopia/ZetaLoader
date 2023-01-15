@echo off
nim c -d:release -d:strip --app:gui -o:ZetaLoader.exe src/main.nim
nim c -d:release -d:strip --app:lib -d:strip -o:ZetaLoader.dll src/dll.nim
:: Optional UPX compression.
upx --best --ultra-brute ZetaLoader.dll ZetaLoader.exe>nul 2>&1