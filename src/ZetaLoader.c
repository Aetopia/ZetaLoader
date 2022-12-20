#include <windows.h>
#include <libgen.h>

int main(__attribute__((unused)) int argc, char *argv[])
{
    static char *proc = "haloinfinite.exe";
    LPVOID mem;
    char dll[MAX_PATH];
    STARTUPINFO si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;
    SetCurrentDirectory(dirname(argv[0]));

    // Verify if Zeta.dll and HaloInfinite.exe exist or not.
    GetFullPathName("Zeta.dll", MAX_PATH, dll, NULL);
    if (GetFileAttributes(dll) == INVALID_FILE_ATTRIBUTES || GetFileAttributes(proc) == INVALID_FILE_ATTRIBUTES)
        return 0;
    CreateProcess(proc, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

    // Inject Zeta.dll into Halo Infinite.
    mem = VirtualAllocEx(pi.hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(pi.hProcess, mem, (LPCVOID)dll, MAX_PATH, NULL);
#pragma GCC diagnostic ignored "-Wcast-function-type"
    CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, mem, 0, 0);
#pragma GCC diagnostic pop
    VirtualFreeEx(pi.hProcess, mem, MAX_PATH, MEM_RELEASE);
    return 0;
}