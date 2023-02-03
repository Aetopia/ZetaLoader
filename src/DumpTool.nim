import winim/lean, os, strutils
const ZetaLoader = staticRead"ZetaLoader.dll"

when isMainModule:
    try:
        let
            cmdline = commandLineParams()
            dir = getAppDir()
            dll = winstrConverterStringToLPWSTR(dir/"ZetaLoader.dll\0")
            hProcess: HANDLE = OpenProcess(PROCESS_ALL_ACCESS, false, cmdline[
                    0].split("-pid:", 1)[1].parseInt().DWORD)
        var mem: LPVOID
        setCurrentDir(dir)

        if not fileExists(dir/"ZetaLoader.dll"):
            writeFile(dir/"ZetaLoader.dll", ZetaLoader)

        mem = VirtualAllocEx(hProcess, nil, MAX_PATH, MEM_COMMIT or MEM_RESERVE, PAGE_EXECUTE_READWRITE)
        WriteProcessMemory(hProcess, mem, cast[LPCVOID](dll), MAX_PATH, nil)
        WaitForSingleObject(CreateRemoteThread(hProcess, nil, 0, cast[
                LPTHREAD_START_ROUTINE](LoadLibrary), mem, 0, nil), INFINITE)
        VirtualFreeEx(hProcess, mem, 0, MEM_RELEASE)
        CloseHandle(hProcess)
    except: discard
