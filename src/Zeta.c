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

// A wrapper for ChangeDisplaySettingsEx.
void SetDM(DEVMODE *dm)
{
    ChangeDisplaySettingsEx(wnd.mi.szDevice, dm, NULL, CDS_FULLSCREEN, NULL);
}

// This function makes the window thread, sleepless and returns the HWND of the process' window.
BOOL IsProcWndForeground(HWND hwnd)
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

// Prevents a process' window from being resized.
DWORD WndSizeThread()
{
    do
    {
        SetWindowPos(wnd.hwnd, 0,
                     wnd.mi.rcMonitor.left, wnd.mi.rcMonitor.top,
                     wnd.cx, wnd.cy,
                     SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOOWNERZORDER | SWP_NOZORDER);
    } while (TRUE);
    return TRUE;
}

// Dynamically switches the display mode/resolution based of if the process' window is in the foreground or not.
void WinEventProc(
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
        switch (IsProcWndForeground(hwnd))
        {
        case TRUE:
            if (wnd.cds)
            {
                if (IsIconic(wnd.hwnd))
                    ShowWindow(wnd.hwnd, SW_RESTORE);
                if (!!wnd.dm.dmFields)
                    SetDM(&wnd.dm);
                wnd.cds = FALSE;
            };
            return;
        case FALSE:
            if (!wnd.cds)
            {
                if (!IsIconic(wnd.hwnd))
                    ShowWindow(wnd.hwnd, SW_MINIMIZE);
                if (!!wnd.dm.dmFields)
                    SetDM(0);
                wnd.cds = TRUE;
            }
        }
    };
}

// Listens to the EVENT_SYSTEM_FOREGROUND for switching display modes dynamically.
DWORD WndDMThread()
{
    MSG msg;
    SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, 0, WinEventProc, 0, 0, WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };
    return 0;
}

DWORD Zeta()
{
    FILE *f;
    int sz;
    char *c;
    DEVMODE dm;
    HMONITOR hmon;
    UINT dpi;
    float scale;

    // Get process ID and HWND + Window TID using IsProcWndForeground.
    wnd.pid = GetCurrentProcessId();
    while (!IsProcWndForeground(GetForegroundWindow()))
        ;

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

    // Run respective threads.
    CreateThread(0, 0, WndSizeThread, NULL, 0, 0);
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