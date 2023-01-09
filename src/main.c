#include <windows.h>
#include <libgen.h>

// In order to ensure the dynamic link library injects with a 100% success rate, listen for Window creation events and then inject.
void WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    __attribute__((unused)) HWND hwnd,
    LONG idObject,
    LONG idChild,
    __attribute__((unused)) DWORD idEventThread,
    __attribute__((unused)) DWORD dwmsEventTime)
{
    if (event != EVENT_OBJECT_SHOW)
        return;
    if (idObject != OBJID_WINDOW ||
        idChild != CHILDID_SELF)
        return;
    UnhookWinEvent(hWinEventHook);
    PostQuitMessage(0);
}

// This thread will ensure incase Halo Infinite crashes before the dynamic link library is injected, ZetaLoader will terminate itself.
DWORD IsProcessAlive(LPVOID lparam)
{
    PROCESS_INFORMATION *pi = (PROCESS_INFORMATION *)lparam;
    WaitForSingleObject(pi->hProcess, INFINITE);
    CloseHandle(pi->hProcess);
    CloseHandle(pi->hThread);
    ExitProcess(0);
    return TRUE;
}

int main(__attribute__((unused)) int argc, char *argv[])
{
    SetCurrentDirectory(dirname(argv[0]));
    LPVOID mem;
    char dll[MAX_PATH];
    STARTUPINFO si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;
    MSG msg;

    GetFullPathName("ZetaLoader.dll", MAX_PATH, dll, NULL);
    if (GetFileAttributes(dll) == INVALID_FILE_ATTRIBUTES ||
        GetFileAttributes("HaloInfinite.exe") == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (!CreateProcess("HaloInfinite.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return 0;
    CreateThread(0, 0, IsProcessAlive, (LPVOID)&pi, 0, 0);

    SetWinEventHook(EVENT_OBJECT_SHOW,
                    EVENT_OBJECT_SHOW, 0,
                    WinEventProc,
                    pi.dwProcessId,
                    0,
                    WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };

    // Inject the dynamic link library into Halo Infinite.
    mem = VirtualAllocEx(pi.hProcess,
                         NULL, MAX_PATH,
                         MEM_COMMIT | MEM_RESERVE,
                         PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(pi.hProcess, mem,
                       (LPCVOID)dll,
                       MAX_PATH,
                       NULL);
#pragma GCC diagnostic ignored "-Wcast-function-type"
    WaitForSingleObject(CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, mem, 0, 0), INFINITE);
#pragma GCC diagnostic pop

    // Cleanup.
    VirtualFreeEx(pi.hProcess, mem, MAX_PATH, MEM_RELEASE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}