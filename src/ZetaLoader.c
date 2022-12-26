#include <windows.h>
#include <libgen.h>
#include <stdio.h>
#include <shellscalingapi.h>

// Structure relating to information of a process' window.
struct WINDOW
{
    HWND hwnd;
    DEVMODE dm;
    DWORD pid;
    MONITORINFOEX mi;
    BOOL cds;
    int cx, cy;
    HANDLE hproc;
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
    hthread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
    SetThreadPriority(hthread, THREAD_PRIORITY_HIGHEST);
    SetThreadPriorityBoost(hthread, FALSE);
    CloseHandle(hthread);
    SwitchToThisWindow(wnd.hwnd, TRUE);
    return TRUE;
}

// A wrapper for ChangeDisplaySettingsEx.
void SetDM(DEVMODE *dm)
{
    if (!!wnd.dm.dmFields)
    {
        ChangeDisplaySettingsEx(wnd.mi.szDevice, dm, NULL, CDS_FULLSCREEN, NULL);
        if (wnd.cds)
            ChangeDisplaySettingsEx(wnd.mi.szDevice, dm, NULL, 0, NULL);
    };
}

void WinEventProc(
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
        SetDM(&wnd.dm);
        do
            SwitchToThisWindow(wnd.hwnd, TRUE);
        while (IsIconic(wnd.hwnd));
        return;
    }
    wnd.cds = TRUE;
    do
        if (!ShowWindow(wnd.hwnd, SW_MINIMIZE))
            break;
    while (!IsIconic(wnd.hwnd));
    SetDM(0);
}

DWORD WinEvent()
{
    MSG msg;
    SetWinEventHook(EVENT_SYSTEM_FOREGROUND,
                    EVENT_SYSTEM_FOREGROUND,
                    0,
                    WinEventProc, 0, 0,
                    WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };
    return TRUE;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, __attribute__((unused)) LPARAM lParam) { return !IsPIDWnd(hwnd); }

int main(__attribute__((unused)) int argc, char *argv[])
{
    SetCurrentDirectory(dirname(argv[0]));
    STARTUPINFO si = {.cb = sizeof(si)};
    PROCESS_INFORMATION pi;
    FILE *f;
    int sz;
    char *c, pri[CCHDEVICENAME];
    DEVMODE dm;
    HMONITOR hmon;
    UINT dpi;
    float scale;

    if (GetFileAttributes("HaloInfinite.exe") == INVALID_FILE_ATTRIBUTES)
        return 0;
    if (!CreateProcess("HaloInfinite.exe", NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        return 0;

    WaitForInputIdle(pi.hProcess, INFINITE);

    // Makes sure that the SetForegroundWindow() or any similar functions work properly.
    wnd.pid = pi.dwProcessId;
    AllowSetForegroundWindow(wnd.pid);
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_UPDATEINIFILE);

    // Get the HWND of process' window.
    while (EnumWindows(EnumWindowsProc, 0))
        Sleep(1);
    do
        SwitchToThisWindow(wnd.hwnd, TRUE);
    while (wnd.hwnd != GetForegroundWindow());

    // Get the primary monitor.
    GetMonitorInfo(MonitorFromWindow(0, MONITORINFOF_PRIMARY), (MONITORINFO *)&wnd.mi);
    strcpy(pri, wnd.mi.szDevice);

    // Setting up Custom Display Mode Support.
    hmon = MonitorFromWindow(wnd.hwnd, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hmon, (MONITORINFO *)&wnd.mi);
    EnumDisplaySettings(wnd.mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);

    if (GetFileAttributes("ZetaLoader.txt") == INVALID_FILE_ATTRIBUTES)
    {
        f = fopen("ZetaLoader.txt", "w");
        fprintf(f, "%ld\n%ld", dm.dmPelsWidth, dm.dmPelsHeight);
        fclose(f);
        wnd.dm.dmPelsWidth = dm.dmPelsWidth;
        wnd.dm.dmPelsHeight = dm.dmPelsHeight;
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
        wnd.dm.dmPelsWidth = atoi(strtok(c, "\n"));
        wnd.dm.dmPelsHeight = atoi(strtok(NULL, "\n"));
        free(c);
    };

    /*
    1. Verify if the display mode is valid.
    2. Only allow for display mode changing and borderless fullscreen fix, if the window style matches.
    If these requirements aren't fulfilled then simply maximize the window and terminate the further initilization.
    */
    if (ChangeDisplaySettings(&wnd.dm, CDS_TEST) != DISP_CHANGE_SUCCESSFUL ||
        (wnd.dm.dmPelsWidth || wnd.dm.dmPelsHeight) == 0 ||
        GetWindowLongPtr(wnd.hwnd, GWL_STYLE) != (WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS))
    {
        ShowWindow(wnd.hwnd, SW_MAXIMIZE);
        return TRUE;
    };

    /*
    1. Check if the native and specified display mode/resolution are the same, if yes then don't allow for display mode changing.
    2. Scale window size according to DPI of the current resolution.
    3. Override the Borderless Window/Fullscreen style set by the program.
    4. Size the window.
    Reference: https://learn.microsoft.com/en-us/windows/win32/direct2d/how-to--size-a-window-properly-for-high-dpi-displays
    */
    if (dm.dmPelsWidth == wnd.dm.dmPelsWidth && dm.dmPelsHeight == wnd.dm.dmPelsHeight)
        wnd.dm.dmFields = 0;
    SetDM(&wnd.dm);
    GetDpiForMonitor(hmon, 0, &dpi, &dpi);
    scale = (float)dpi / 96.0;
    wnd.cx = wnd.dm.dmPelsWidth * scale;
    wnd.cy = wnd.dm.dmPelsHeight * scale;

    SetWindowLongPtr(wnd.hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
    SetWindowLongPtr(wnd.hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
    SetWindowPos(wnd.hwnd, 0,
                 wnd.mi.rcMonitor.left, wnd.mi.rcMonitor.top,
                 wnd.cx, wnd.cy,
                 SWP_NOACTIVATE | SWP_NOSENDCHANGING);
    do
        SwitchToThisWindow(wnd.hwnd, TRUE);
    while (wnd.hwnd != GetForegroundWindow());

    if (strcmp(pri, wnd.mi.szDevice) == 0)
        CreateThread(0, 0, WinEvent, NULL, 0, 0);

    WaitForSingleObject(pi.hProcess, INFINITE);
    SetDM(0);
    return 0;
}