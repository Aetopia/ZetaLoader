import winim/lean
import os

when isMainModule:
    const ZetaLoader = staticRead"ZetaLoader.dll"
    let
        dir = getAppDir()
        dll = winstrConverterStringToLPWSTR(dir/"ZetaLoader.dll\0")
    var
        pi: PROCESS_INFORMATION
        si: STARTUPINFO
        mem: LPVOID
    si.cb = sizeof(si).DWORD
    setCurrentDir(dir)

    if not fileExists("HaloInfinite.exe") or
            CreateProcess("HaloInfinite.exe", nil, nil, nil, false,
            CREATE_SUSPENDED, nil, nil,
            addr si, addr pi) == FALSE:
        quit(0)

    if not fileExists(dir/"ZetaLoader.dll"):
        writeFile(dir/"ZetaLoader.dll", ZetaLoader)
    elif readFile(dir/"ZetaLoader.dll") != ZetaLoader:
        writeFile(dir/"ZetaLoader.dll", ZetaLoader)

    mem = VirtualAllocEx(pi.hProcess, nil, MAX_PATH, MEM_COMMIT or MEM_RESERVE, PAGE_EXECUTE_READWRITE)
    WriteProcessMemory(pi.hProcess, mem, cast[LPCVOID](dll), MAX_PATH, nil)
    WaitForSingleObject(CreateRemoteThread(pi.hProcess, nil, 0, cast[
            LPTHREAD_START_ROUTINE](LoadLibrary), mem, 0, nil), INFINITE)
    VirtualFreeEx(pi.hProcess, mem, 0, MEM_RELEASE)
    ResumeThread(pi.hThread)
    CloseHandle(pi.hProcess)
    CloseHandle(pi.hThread)
