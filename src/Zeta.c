#include <windows.h>
#include <shellscalingapi.h>
#include <stdio.h>

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

// This function makes the window thread, sleepless and assigns the HWND of the process' window to the WINDOW structure..
BOOL IsProcWnd(HWND hwnd)
{
    DWORD pid, tid = GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hthread;
    if (wnd.pid == pid)
    {
        // Values are reassigned when needed.
        if (wnd.hwnd != hwnd)
        {
            wnd.hwnd = hwnd;

            // Sets the window thread priority to highest and ensures thread priority boost is enabled.
            hthread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
            SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST);
            SetThreadPriorityBoost(hthread, FALSE);
            CloseHandle(hthread);
        };
        return TRUE;
    };
    return FALSE;
}

// A wrapper for ChangeDisplaySettingsEx.
void SetDM(DEVMODE *dm)
{
    ChangeDisplaySettingsEx(wnd.mi.szDevice, dm, NULL, CDS_FULLSCREEN, NULL);
}

void WndDMThreadProc(
    __attribute__((unused)) HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    __attribute__((unused)) LONG idObject,
    __attribute__((unused)) LONG idChild,
    __attribute__((unused)) DWORD idEventThread,
    __attribute__((unused)) DWORD dwmsEventTime)
{
    if (event == EVENT_SYSTEM_FOREGROUND)
    {
        if (IsProcWnd(hwnd) && wnd.cds)
        {
            wnd.cds = FALSE;
            if (IsIconic(wnd.hwnd))
                ShowWindow(wnd.hwnd, SW_RESTORE);
            if (!!wnd.dm.dmFields)
            {
                SetDM(&wnd.dm);
            };
        }
        else if (!wnd.cds)
        {
            wnd.cds = TRUE;
            if (!IsIconic(wnd.hwnd))
                ShowWindow(wnd.hwnd, SW_MINIMIZE);
            if (!!wnd.dm.dmFields)
            {
                SetDM(0);
            };
        };
    };
};

DWORD WndDMThread()
{
    MSG msg;
    SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, 0, WndDMThreadProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };
    return 0;
}

void SetWndPosThreadProc(
    __attribute__((unused)) HWINEVENTHOOK hWinEventHook,
    DWORD event,
    __attribute__((unused)) HWND hwnd,
    __attribute__((unused)) LONG idObject,
    __attribute__((unused)) LONG idChild,
    __attribute__((unused)) DWORD idEventThread,
    __attribute__((unused)) DWORD dwmsEventTime)
{
    // Reposition the window, whenever EVENT_OBJECT_LOCATIONCHANGE occurs.
    if (event == EVENT_OBJECT_LOCATIONCHANGE)
        SetWindowPos(wnd.hwnd, 0,
                     wnd.mi.rcMonitor.left, wnd.mi.rcMonitor.top,
                     wnd.cx, wnd.cy,
                     SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOZORDER);
}

DWORD SetWndPosThread()
{
    MSG msg;
    SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE, EVENT_OBJECT_LOCATIONCHANGE, 0, SetWndPosThreadProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };
    return 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, __attribute__((unused)) LPARAM lParam) { return !IsProcWnd(hwnd); }

DWORD Zeta()
{
    FILE *f;
    int sz;
    char *c;
    DEVMODE dm;
    HMONITOR hmon;
    UINT dpi;
    float scale;

    // Get process ID and window HWND using IsProcWnd.
    wnd.pid = GetCurrentProcessId();
    EnumWindows(EnumWindowsProc, 0);
    while (!IsWindowVisible(wnd.hwnd))
        ;
    SetForegroundWindow(wnd.hwnd);

    // Setting up Custom Display Mode Support.
    hmon = MonitorFromWindow(wnd.hwnd, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hmon, (MONITORINFO *)&wnd.mi);
    EnumDisplaySettings(wnd.mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);

    if (GetFileAttributes("Zeta.txt") == INVALID_FILE_ATTRIBUTES)
    {
        f = fopen("Zeta.txt", "w");
        fprintf(f, "%ld\n%ld", dm.dmPelsWidth, dm.dmPelsHeight);
        fclose(f);
        wnd.dm.dmPelsWidth = dm.dmPelsWidth;
        wnd.dm.dmPelsHeight = dm.dmPelsHeight;
    }
    else
    {
        f = fopen("Zeta.txt", "r");
        fseek(f, 0, SEEK_END);
        sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        c = malloc(sz);
        fread(c, 1, sz, f);
        fclose(f);
        wnd.dm.dmPelsWidth = atoi(strtok(c, "\n"));
        wnd.dm.dmPelsHeight = atoi(strtok(NULL, "\n"));
        free(c);
    };

    // Verify if the display mode is valid, if not terminates the thread.
    if (ChangeDisplaySettings(&wnd.dm, CDS_TEST) != DISP_CHANGE_SUCCESSFUL ||
        (wnd.dm.dmPelsWidth || wnd.dm.dmPelsHeight) == 0)
    {
        return 0;
    }

    // If the native and specified display mode/resolution are the same don't allow for display mode changing.
    if (dm.dmPelsWidth == wnd.dm.dmPelsWidth && dm.dmPelsHeight == wnd.dm.dmPelsHeight)
        wnd.dm.dmFields = 0;
    else
        SetDM(&wnd.dm);

    // Scale window size according to DPI of the current resolution.
    GetDpiForMonitor(hmon, 0, &dpi, &dpi);
    scale = dpi / 96;
    wnd.cx = wnd.dm.dmPelsWidth * scale;
    wnd.cy = wnd.dm.dmPelsHeight * scale;

    CreateThread(0, 0, SetWndPosThread, NULL, 0, 0);
    CreateThread(0, 0, WndDMThread, NULL, 0, 0);
    return TRUE;
}

BOOL WINAPI DllMain(__attribute__((unused)) HINSTANCE hInstDll,
                    DWORD fwdreason,
                    __attribute__((unused)) LPVOID lpReserved)
{
    if (fwdreason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, Zeta, NULL, 0, 0);
    return TRUE;
}