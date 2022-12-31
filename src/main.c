#include <windows.h>
#include <libgen.h>

PROCESS_INFORMATION pi;

void WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD idEventThread,
    DWORD dwmsEventTime)
{
    if (event != EVENT_OBJECT_CREATE)
        return;
    if (idObject != OBJID_WINDOW ||
        idChild != CHILDID_SELF)
        return;
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != pi.dwProcessId)
        return;
    UnhookWinEvent(hWinEventHook);
    PostQuitMessage(0);
}

int main(__attribute__((unused)) int argc, char *argv[])
{
    SetCurrentDirectory(dirname(argv[0]));
    LPVOID mem;
    char dll[MAX_PATH];
    STARTUPINFO si = {.cb = sizeof(si)};
    MSG msg;

    GetFullPathName("ZetaLoader.dll", MAX_PATH, dll, NULL);
    if (GetFileAttributes(dll) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributes("HaloInfinite.exe") == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (!CreateProcess("HaloInfinite.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return 0;
    SetWinEventHook(EVENT_OBJECT_CREATE,
                    EVENT_OBJECT_CREATE, 0,
                    WinEventProc,
                    pi.dwProcessId,
                    0,
                    WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };

    // Inject Zeta.dll into Halo Infinite.
    mem = VirtualAllocEx(pi.hProcess,
                         NULL, MAX_PATH,
                         MEM_COMMIT | MEM_RESERVE,
                         PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(pi.hProcess, mem,
                       (LPCVOID)dll,
                       MAX_PATH,
                       NULL);
#pragma GCC diagnostic ignored "-Wcast-function-type"
    WaitForSingleObject(CreateRemoteThread(pi.hProcess, 0, 0,
                                           (LPTHREAD_START_ROUTINE)LoadLibrary, mem, 0, 0),
                        INFINITE);
#pragma GCC diagnostic pop
    VirtualFreeEx(pi.hProcess, mem, MAX_PATH, MEM_RELEASE);
    return 0;
}