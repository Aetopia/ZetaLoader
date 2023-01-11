#include <windows.h>
#include <stdio.h>
#include <dwmapi.h>

NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG DesiredResolution,
                                             BOOLEAN SetResolution,
                                             PULONG CurrentResolution);
NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG MinimumResolution,
                                               PULONG MaximumResolution,
                                               PULONG CurrentResolution);

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

static inline BOOL IsNumber(char *str) { return strspn(str, "0123456789") == strlen(str); }

static inline void SetDM(DEVMODE *dm)
{
    if (!!dll.dm.dmFields)
        ChangeDisplaySettingsEx(dll.mon, dm, NULL, CDS_FULLSCREEN, NULL);
}

static inline void BorderlessFullscreen()
{
    SetWindowLongPtr(dll.hwnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
    SetWindowLongPtr(dll.hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
    SetWindowPos(dll.hwnd, HWND_TOPMOST, dll.x, dll.y, dll.cx, dll.cy, SWP_NOACTIVATE | SWP_NOSENDCHANGING);
}

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

void WinEventProc(
    HWINEVENTHOOK hwineventhook,
    DWORD event,
    HWND hwnd,
    LONG idobject,
    LONG idchild,
    __attribute__((unused)) DWORD ideventthread,
    __attribute__((unused)) DWORD dwmseventtime)
{
    if (event != EVENT_OBJECT_SHOW ||
        idobject != OBJID_WINDOW ||
        idchild != CHILDID_SELF)
        return;
    UnhookWinEvent(hwineventhook);

    DWORD tid = GetWindowThreadProcessId(hwnd, NULL);
    HANDLE hthread;
    dll.hwnd = hwnd;
    dll.WindowProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);

    DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &dll.pri, 4);
    DwmSetWindowAttribute(hwnd, DWMWA_DISALLOW_PEEK, &dll.pri, 4);
    DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &dll.pri, 4);

    hthread = OpenThread(THREAD_SET_INFORMATION, FALSE, tid);
    SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadPriorityBoost(hthread, FALSE);
    CloseHandle(hthread);

    PostQuitMessage(0);
}

DWORD ZetaLoader()
{
    FILE *f;
    int sz;
    char *c, *w, *h;
    MONITORINFOEX mi = {.cbSize = sizeof(mi)};
    HMONITOR hmon;
    DEVMODE dm;
    ULONG min, max, cur;
    DWORD tm, pid = GetCurrentProcessId();
    HANDLE hthread;
    MSG msg;

    SetWinEventHook(EVENT_OBJECT_SHOW,
                    EVENT_OBJECT_SHOW, 0,
                    WinEventProc,
                    pid,
                    0,
                    WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    };

    NtQueryTimerResolution(&min, &max, &cur);
    NtSetTimerResolution(max, TRUE, &cur);
    DwmEnableMMCSS(TRUE);
    AllowSetForegroundWindow(pid);

    hmon = MonitorFromWindow(dll.hwnd, MONITOR_DEFAULTTONEAREST);
    GetMonitorInfo(hmon, (MONITORINFO *)&mi);
    if (mi.dwFlags != MONITORINFOF_PRIMARY)
        dll.pri = FALSE;
    strcpy(dll.mon, mi.szDevice);
    EnumDisplaySettings(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm);
    dll.dm.dmPelsWidth = dm.dmPelsWidth;
    dll.dm.dmPelsHeight = dm.dmPelsHeight;
    if (GetFileAttributes("ZetaLoader.txt") == INVALID_FILE_ATTRIBUTES)
    {
        f = fopen("ZetaLoader.txt", "w");
        fprintf(f, "%ld\n%ld", dll.dm.dmPelsWidth, dll.dm.dmPelsHeight);
        fclose(f);
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
        if (w && h && IsNumber(w) && IsNumber(h))
        {
            dll.dm.dmPelsWidth = atoi(w);
            dll.dm.dmPelsHeight = atoi(h);
        }
        free(c);
    };

    if (!!ChangeDisplaySettingsEx(dll.mon, &dll.dm, NULL, CDS_TEST, 0) ||
        !(dll.dm.dmPelsWidth || dll.dm.dmPelsHeight) ||
        GetWindowLongPtr(dll.hwnd, GWL_STYLE) != (WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS))
    {
        ShowWindow(dll.hwnd, SW_MAXIMIZE);
        return TRUE;
    };

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
    SetWindowLongPtr(dll.hwnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
    TerminateThread(hthread, 0);
    CloseHandle(hthread);
    if (tm)
        SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)&tm, 0);
    return TRUE;
}

BOOL WINAPI DllMain(__attribute__((unused)) HINSTANCE hinstdll,
                    DWORD fdwreason,
                    __attribute__((unused)) LPVOID lpvreserved)
{
    if (fdwreason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, ZetaLoader, NULL, 0, 0);
    return TRUE;
}