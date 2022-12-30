#include <windows.h>
#include <libgen.h>
#include <stdio.h>
#include <shellscalingapi.h>

// Structure relating to information of a process' window.
struct WINDOW
{
    HWND hwnd;
    DEVMODE dm;
    DWORD pid, tm;
    MONITORINFOEX mi;
    BOOL cds;
    int cx, cy;
};
struct WINDOW wnd = {.mi.cbSize = sizeof(wnd.mi),
                     .dm.dmSize = sizeof(wnd.dm),
                     .dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT,
                     .cds = TRUE};
WNDPROC _WindowProc;

// A wrapper for ChangeDisplaySettingsEx.
void SetDM(DEVMODE *dm)
{
    if (!!wnd.dm.dmFields)
    {
        ChangeDisplaySettingsEx(wnd.mi.szDevice, dm, NULL, CDS_FULLSCREEN, NULL);
    };
}
void BorderlessWindow()
{
    SetWindowLongPtr(wnd.hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
    SetWindowLongPtr(wnd.hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST);
    SetWindowPos(wnd.hwnd, 0,
                 wnd.mi.rcMonitor.left, wnd.mi.rcMonitor.top,
                 wnd.cx, wnd.cy, SWP_NOSENDCHANGING);
};
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY || msg == WM_CLOSE || msg == WM_QUIT)
    {
        ShowWindow(wnd.hwnd, SW_HIDE);
        SetDM(0);
        wnd.dm.dmFields = 0;
    }
    else if ((msg == WM_ACTIVATE || msg == WM_ACTIVATEAPP) && wnd.cds)
    {
        if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE || wParam == TRUE)
        {
            SwitchToThisWindow(wnd.hwnd, TRUE);
            SetDM(&wnd.dm);
        }
        else if (wParam == WA_INACTIVE || wParam == FALSE)
        {
            ShowWindow(wnd.hwnd, SW_MINIMIZE);
            SetDM(0);
        };
    }
    else if (msg == WM_WINDOWPOSCHANGING)
        BorderlessWindow();
    return CallWindowProc(_WindowProc, hwnd, msg, wParam, lParam);
}

DWORD IsWindowThreadAlive(LPVOID lParameter)
{
    WaitForSingleObject((HANDLE)lParameter, INFINITE);
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)&wnd.tm, SPIF_UPDATEINIFILE);
    SetDM(0);
    return TRUE;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, __attribute__((unused)) LPARAM lParam)
{
    DWORD pid, tid = GetWindowThreadProcessId(hwnd, &pid);
    HANDLE hthread;
    if (wnd.pid != pid)
        return TRUE;

    wnd.hwnd = hwnd;
    hthread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);
    CreateThread(0, 0, IsWindowThreadAlive, (LPVOID)hthread, 0, 0);
    SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadPriorityBoost(hthread, FALSE);
    while (!IsWindowVisible(wnd.hwnd))
        Sleep(1);
    return FALSE;
}

DWORD ZetaLoader()
{
    FILE *f;
    int sz;
    char *c, pri[CCHDEVICENAME];
    DEVMODE dm;
    HMONITOR hmon;
    UINT dpi;
    float scale;

    // Makes sure that the SetForegroundWindow() or any similar functions work properly.
    wnd.pid = GetCurrentProcessId();
    AllowSetForegroundWindow(wnd.pid);
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, (PVOID)&wnd.tm, 0);
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, SPIF_UPDATEINIFILE);

    // Get the HWND of process' window.
    while (EnumWindows(EnumWindowsProc, 0))
        Sleep(1);

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
    _WindowProc = (WNDPROC)GetWindowLongPtr(wnd.hwnd, GWLP_WNDPROC);
    SetWindowLongPtr(wnd.hwnd, GWLP_WNDPROC, (LONG_PTR)&WindowProc);

    /*
    1. Check if the native and specified display mode/resolution are the same, if yes then don't allow for display mode changing.
    2. Scale window size according to DPI of the current resolution.
    3. Override the Borderless Window/Fullscreen style set by the program.
    4. Size the window.
    Reference: https://learn.microsoft.com/en-us/windows/win32/direct2d/how-to--size-a-window-properly-for-high-dpi-displays
    */
    if (dm.dmPelsWidth == wnd.dm.dmPelsWidth && dm.dmPelsHeight == wnd.dm.dmPelsHeight)
        wnd.dm.dmFields = 0;
    SendMessage(wnd.hwnd, WM_ACTIVATE, WA_ACTIVE, 0);
    GetDpiForMonitor(hmon, 0, &dpi, &dpi);
    scale = (float)dpi / 96.0;
    wnd.cx = wnd.dm.dmPelsWidth * scale;
    wnd.cy = wnd.dm.dmPelsHeight * scale;
    BorderlessWindow();
    SendMessage(wnd.hwnd, WM_ACTIVATE, WA_ACTIVE, 0);

    if (strcmp(pri, wnd.mi.szDevice) != 0)
        wnd.cds = FALSE;
    return TRUE;
}

BOOL WINAPI DllMain(__attribute__((unused)) HINSTANCE hInstDll,
                    DWORD fwdreason,
                    __attribute__((unused)) LPVOID lpReserved)
{
    // The dynamic link library is intitalized via the Zeta() function in a thread to prevent the target application from getting locked up.
    if (fwdreason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, ZetaLoader, NULL, 0, 0);
    return TRUE;
}