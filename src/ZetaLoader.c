#include <windows.h>
#include <psapi.h>
#include <libgen.h>

const char *proc = "haloinfinite.exe";
DWORD pid;

void WinEventProc(
    __attribute__((unused)) HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    __attribute__((unused)) LONG idObject,
    __attribute__((unused)) LONG idChild,
    __attribute__((unused)) DWORD idEventThread,
    __attribute__((unused)) DWORD dwmsEventTime)
{
    if (event == EVENT_OBJECT_CREATE)
    {
        char file[MAX_PATH];
        HANDLE hproc;
        GetWindowThreadProcessId(hwnd, &pid);
        hproc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        GetModuleFileNameEx(hproc, NULL, file, MAX_PATH);
        CloseHandle(hproc);
        if (strcmp(strlwr(basename(file)), proc) == 0)
            PostQuitMessage(0);
    };
}

int main(int argc, char *argv[])
{
    SetCurrentDirectory(dirname(argv[0]));
    HANDLE hproc, hthread;
    LPVOID mem;
    char dll[MAX_PATH];
    MSG msg;

    // Verify if Zeta.dll and HaloInfinite.exe exist or not.
    GetFullPathName("Zeta.dll", MAX_PATH, dll, NULL);
    if (GetFileAttributes(dll) == INVALID_FILE_ATTRIBUTES || GetFileAttributes(proc) == INVALID_FILE_ATTRIBUTES)
        return 0;
    ShellExecute(0, "open", proc, NULL, ".", 5);
    
    // Listen for events when a window is in the foreground.
    SetWinEventHook(EVENT_OBJECT_CREATE,
                    EVENT_OBJECT_CREATE, 0,
                    WinEventProc, 0, 0,
                    WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };

    // Inject Zeta.dll into Halo Infinite.
    hproc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    mem = VirtualAllocEx(hproc, NULL, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(hproc, mem, (LPCVOID)dll, MAX_PATH, NULL);
    hthread = CreateRemoteThread(hproc, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, mem, 0, 0);
    WaitForSingleObject(hthread, INFINITE);
    VirtualFreeEx(hproc, mem, MAX_PATH, MEM_RELEASE);
    if (hthread)
        CloseHandle(hthread);
    return 0;
}