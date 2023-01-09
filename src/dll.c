#include <windows.h>
#include <stdio.h>
#include <dwmapi.h>

// Timer resolution related functions.
NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution,
                                             BOOLEAN SetResolution,
                                             PULONG CurrentResolution);
NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG MinimumResolution,
                                               PULONG MaximumResolution,
                                               PULONG CurrentResolution);

// Check if a string is a number.
static inline BOOL IsNumber(char *str) { return strspn(str, "0123456789") == strlen(str); }

// Global Structure, used to store all the variables.
struct DLL
{
    HWND hwnd;
    DEVMODE dm;
    char mon[CCHDEVICENAME];
    int x, y, cx, cy;
    BOOL pri;
    WNDPROC WindowProc;
};
struct DLL dll = {.dm.dmSize = sizeof(dll.dm),
                  .dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT,
                  .pri = TRUE};

// A wrapper for ChangeDisplaySettingsEx.
static inline void SetDM(DEVMODE *dm)
{
    if (!!dll.dm.dmFields)
        ChangeDisplaySettingsEx(dll.mon, dm, NULL, CDS_FULLSCREEN, NULL);
}

// This function is uses window styles that fix Halo Infinite's borderless fullscreen..
static inline void BorderlessFullscreen()
{
    SetWindowLongPtr(dll.hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
    SetWindowLongPtr(dll.hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowPos(dll.hwnd, HWND_TOPMOST, dll.x, dll.y, dll.cx, dll.cy, SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

// This function is used to bring the game window to the foreground constantly thus locking it.
DWORD ForegroundWindowLock()
{
    HANDLE hthread = GetCurrentThread();
    SetThreadPriority(hthread, THREAD_MODE_BACKGROUND_BEGIN);
    SetThreadPriorityBoost(hthread, FALSE);
    CloseHandle(hthread);
    do
        SwitchToThisWindow(dll.hwnd, TRUE);
    while (TRUE);
    return TRUE;
}

// This function is used to intercept any incoming window messages.
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
    case WM_CLOSE:
    case WM_QUIT:
        ShowWindow(hwnd, SW_HIDE);
        SetDM(0);
        break;
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
        if (dll.pri)
            switch (wparam)
            {
            case WA_ACTIVE:
            case WA_CLICKACTIVE:
                if (IsIconic(hwnd))
                    SwitchToThisWindow(hwnd, TRUE);
                SetDM(&dll.dm);
                break;
            case WA_INACTIVE:
                if (!IsIconic(hwnd))
                    ShowWindow(hwnd, SW_MINIMIZE);
                SetDM(0);
            };
        break;
    case WM_WINDOWPOSCHANGING:
        BorderlessFullscreen();
        msg = WM_NULL;
        break;
    };
    return CallWindowProc(dll.WindowProc, hwnd, msg, wparam, lparam);
}

/*
1. Enumerate all windows and find the one that belongs to the process.
2. Disable transition effects.
3. Prevent window peeking.
4. Disable live representation of the window and force a static bitmap of the window.
5. Set the window thread priority to the time critical.
6. Enable Thread Priority Boost.
7. Wait for the window to be visible.
*/
BOOL EnumWindowsProc(HWND hwnd, LPARAM lparam)
{
    DWORD pid, tid = GetWindowThreadProcessId(hwnd, &pid);
    const BOOL vAttribute = TRUE, *pvAttribute = &vAttribute;
    HANDLE hthread;
    if ((DWORD)lparam != pid)
        return TRUE;
    dll.hwnd = hwnd;
    dll.WindowProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, pvAttribute, 4);
    DwmSetWindowAttribute(hwnd, DWMWA_DISALLOW_PEEK, pvAttribute, 4);
    DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, pvAttribute, 4);
    hthread = OpenThread(THREAD_SET_INFORMATION, FALSE, tid);
    SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadPriorityBoost(hthread, FALSE);
    CloseHandle(hthread);
    return FALSE;
}

DWORD ZetaLoader()
{
    FILE *f;
    int sz;
    char *c, *w, *h;
    MONITORINFOEX mi = {.cbSize = sizeof(mi)};
    HMONITOR hmon;
    DEVMODE dm;
    DWORD tm;
    ULONG min, max, cur;
    DWORD pid = GetCurrentProcessId();
    HANDLE hthread;

    /*
    Halo Infinite uses 1 ms by default, we can force 0.5 ms using NtSetTimerResolution.
    Starting with Windows 2004, setting the timer resolution is no longer global but on a per process basis.
    Reference: https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod#remarks

    Making DWM opt in for MMCSS when calling process is alive improves performance.
    References:
    1. https://github.com/djdallmann/GamingPCSetup/blob/master/CONTENT/RESEARCH/WINSERVICES/README.md#multimedia-class-scheduler-service-mmcss
    2. https://www.overclock.net/threads/if-you-play-non-fullscreen-exclusive-games-you-might-get-a-boost-in-performance-with-dwmenablemmcss.1775433/
    */
    NtQueryTimerResolution(&min, &max, &cur);
    NtSetTimerResolution(max, TRUE, &cur);
    DwmEnableMMCSS(TRUE);
    AllowSetForegroundWindow(pid);

    // Get the HWND of process' window.
    while (EnumWindows(EnumWindowsProc, (LPARAM)pid))
        ;

    // Setting up Custom Display Mode Support.
    hmon = MonitorFromWindow(dll.hwnd, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hmon, (MONITORINFO *)&mi);
    if (mi.dwFlags != MONITORINFOF_PRIMARY)
        dll.pri = FALSE;
    strcpy(dll.mon, mi.szDevice);
    EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);

    if (GetFileAttributes("ZetaLoader.txt") == INVALID_FILE_ATTRIBUTES)
    {
        f = fopen("ZetaLoader.txt", "w");
        fprintf(f, "%ld\n%ld", dm.dmPelsWidth, dm.dmPelsHeight);
        fclose(f);
        dll.dm.dmPelsWidth = dm.dmPelsWidth;
        dll.dm.dmPelsHeight = dm.dmPelsHeight;
    }
    else
    {
        f = fopen("ZetaLoader.txt", "r");
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        c = malloc(sz);
        fread(c, 1, sz, f);
        fclose(f);
        w = strtok(c, "\r\t\n ");
        h = strtok(NULL, "\r\t\n ");
        if (IsNumber(w) && IsNumber(h))
        {
            dll.dm.dmPelsWidth = atoi(w);
            dll.dm.dmPelsHeight = atoi(h);
        };
        free(c);
    };

    /*
    1. Verify if the display mode is valid.
    2. Only allow for display mode changing and borderless fullscreen fix, if the window style matches.
    If these requirements aren't fulfilled then simply maximize the window and terminate the further initilization.
    */
    if (!!ChangeDisplaySettingsEx(dll.mon, &dll.dm, NULL, CDS_TEST, 0) ||
        !(dll.dm.dmPelsWidth || dll.dm.dmPelsHeight) ||
        GetWindowLongPtr(dll.hwnd, GWL_STYLE) != (WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS))
    {
        ShowWindow(dll.hwnd, SW_MAXIMIZE);
        return TRUE;
    };

    /*
    1. Check if the native and specified display mode/resolution are the same, if yes then don't allow for display mode changing.
    2. Override the Borderless Window/Fullscreen style set by the program.
    3. Size and position the window.
    4. Intercept the game's window procedure.
    */
    if (dm.dmPelsWidth == dll.dm.dmPelsWidth &&
        dm.dmPelsHeight == dll.dm.dmPelsHeight)
        dll.dm.dmFields = 0;
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)&tm, 0);
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, 0);
    hthread = CreateThread(0, 0, ForegroundWindowLock, 0, 0, 0);
    SetDM(&dll.dm);
    GetMonitorInfo(hmon, (MONITORINFO *)&mi);
    dll.x = mi.rcMonitor.left;
    dll.y = mi.rcMonitor.top;
    dll.cx = mi.rcMonitor.right - mi.rcMonitor.left;
    dll.cy = mi.rcMonitor.bottom - mi.rcMonitor.top;
    BorderlessFullscreen();
    SetWindowLongPtr(dll.hwnd, GWLP_WNDPROC, (LONG_PTR)&WindowProc);
    TerminateThread(hthread, 0);
    CloseHandle(hthread);
    if (tm)
        SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)&tm, 0);
    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hInstDll,
                    DWORD fwdreason,
                    __attribute__((unused)) LPVOID lpvReserved)
{
    // The dynamic link library is intitalized via the ZetaLoader() function in a thread to prevent the target application from getting locked up.
    if (fwdreason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstDll);
        CreateThread(0, 0, ZetaLoader, NULL, 0, 0);
    };
    return TRUE;
}