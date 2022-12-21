#include <windows.h>
#include <libgen.h>

int main(__attribute__((unused)) int argc, char *argv[])
{
    SetCurrentDirectory(dirname(argv[0]));
    LPVOID mem;
    char dll[MAX_PATH];
    SHELLEXECUTEINFO ei = {.cbSize = sizeof(ei),
                           .lpFile = "HaloInfinite.exe",
                           .lpVerb = "open",
                           .nShow = 5,
                           .fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC};

    GetFullPathName("Zeta.dll", MAX_PATH, dll, NULL);
    if (GetFileAttributes(dll) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributes("HaloInfinite.exe") == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (!ShellExecuteEx(&ei))
        return 0;

    // Inject Zeta.dll into Halo Infinite.
    mem = VirtualAllocEx(ei.hProcess, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(ei.hProcess, mem, (LPCVOID)dll, MAX_PATH, NULL);
#pragma GCC diagnostic ignored "-Wcast-function-type"
    WaitForSingleObject(CreateRemoteThread(ei.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, mem, 0, 0), INFINITE);
#pragma GCC diagnostic pop
    VirtualFreeEx(ei.hProcess, mem, MAX_PATH, MEM_RELEASE);
    return 0;
}