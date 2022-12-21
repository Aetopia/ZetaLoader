#include <windows.h>
#include <shellscalingapi.h>
#include <stdio.h>

// Timer resolution related functions.
NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution);

// Structure relating to information of a process' window.
struct WINDOW
{
    HWND hwnd;
    DEVMODE dm;
    DWORD pid;
    MONITORINFOEX mi;
    BOOL cds;
    int cx, cy;
};
struct WINDOW wnd = {.mi.cbSize = sizeof(wnd.mi),
                     .dm.dmSize = sizeof(wnd.dm),
                     .dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT,
                     .cds = FALSE};

// This function makes the window thread, sleepless and assigns the HWND of the process' window to the WINDOW structure.
BOOL IsPIDWnd(HWND hwnd)
{
    DWORD pid, tid = GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hthread;
    if (wnd.pid != pid)
        return FALSE;
    if (wnd.hwnd == hwnd)
        return TRUE;

    wnd.hwnd = hwnd;
    /*
    Sets the window thread priority to highest and ensures thread priority boost is enabled.
    This oddly fixes Screen Tearing issues, likely meaning the window thread goes to "sleep" when the thread priority is lower.
    */
    hthread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
    SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST);
    SetThreadPriorityBoost(hthread, FALSE);
    CloseHandle(hthread);
    return TRUE;
}

// A wrapper for ChangeDisplaySettingsEx.
void SetDM(DEVMODE *dm) { ChangeDisplaySettingsEx(wnd.mi.szDevice, dm, NULL, CDS_FULLSCREEN, NULL); }

void WndDMThreadProc(
    __attribute__((unused)) HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    __attribute__((unused)) LONG idObject,
    __attribute__((unused)) LONG idChild,
    __attribute__((unused)) DWORD idEventThread,
    __attribute__((unused)) DWORD dwmsEventTime)
{
    if (event != EVENT_SYSTEM_FOREGROUND)
        return;
    if (IsPIDWnd(hwnd) && wnd.cds)
    {
        wnd.cds = FALSE;
        if (IsIconic(wnd.hwnd))
            SwitchToThisWindow(wnd.hwnd, TRUE);
        if (!!wnd.dm.dmFields)
            SetDM(&wnd.dm);
    }
    else if (!wnd.cds)
    {
        wnd.cds = TRUE;
        if (!IsIconic(wnd.hwnd))
            ShowWindow(wnd.hwnd, SW_MINIMIZE);
        if (!!wnd.dm.dmFields)
            SetDM(0);
    };
};

DWORD WndDMThread()
{
    /*
    This thread also controls the window's visibility state and desired display mode/resolution.
    It is given the highest priority to prevent it from going to "sleep".
    This will not affect performance considering window visibility state & display mode/resolution related functions are only called when EVENT_SYSTEM_FOREGROUND occurs.
    */
    MSG msg;
    HANDLE hthread = GetCurrentThread();
    SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST);
    SetThreadPriorityBoost(hthread, FALSE);
    CloseHandle(hthread);
    SetWinEventHook(EVENT_SYSTEM_FOREGROUND,
                    EVENT_SYSTEM_FOREGROUND, 0,
                    WndDMThreadProc, 0, 0,
                    WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };
    return 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd,
                              __attribute__((unused)) LPARAM lParam) { return !IsPIDWnd(hwnd); }

// This thread constantly checks if the process' window is hung or not.
DWORD IsHungAppWindowThread()
{
    while (!IsHungAppWindow(wnd.hwnd))
        Sleep(1);
    ShowWindow(wnd.hwnd, SW_FORCEMINIMIZE);
    if (!!wnd.dm.dmFields)
        SetDM(0);
    MessageBox(wnd.hwnd, "Looks like Halo Infinite has crashed!", "Halo Infinite - Crashed", MB_ICONINFORMATION);
    TerminateProcess(GetCurrentProcess(), 0);
    return TRUE;
}

BOOL WINAPI DllMain(__attribute__((unused)) HINSTANCE hInstDll,
                    DWORD fwdreason,
                    __attribute__((unused)) LPVOID lpReserved)
{
    // The dynamic link library is intitalized via the Zeta() function in a thread to prevent the target application from getting locked up.
    if (fwdreason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, Zeta, NULL, 0, 0);
    return TRUE;
}