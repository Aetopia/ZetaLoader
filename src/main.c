#include <windows.h>
#include <libgen.h>

int main(__attribute__((unused)) int argc, char *argv[])
{
    SetCurrentDirectory(dirname(argv[0]));
    LPVOID mem;
    char dll[MAX_PATH];
    STARTUPINFO si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;

    GetFullPathName("ZetaLoader.dll", MAX_PATH, dll, NULL);
    if (GetFileAttributes(dll) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributes("HaloInfinite.exe") == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (!CreateProcess("HaloInfinite.exe", NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        return 0;

    // Inject Zeta.dll into Halo Infinite.
    mem = VirtualAllocEx(pi.hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(pi.hProcess, mem, (LPCVOID)dll, MAX_PATH, NULL);
#pragma GCC diagnostic ignored "-Wcast-function-type"
    WaitForSingleObject(CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, mem, 0, 0), INFINITE);
#pragma GCC diagnostic pop
    VirtualFreeEx(pi.hProcess, mem, MAX_PATH, MEM_RELEASE);
    ResumeThread(pi.hThread);
    return 0;
}