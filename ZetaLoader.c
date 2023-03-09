#include <windows.h>
#include <getopt.h>
#include <dwmapi.h>

struct GAME
{
    DEVMODEW devMode;
    MONITORINFOEXW monitorInfo;
    HMONITOR hMonitor;
    int cx, cy;
    BOOL userSpecifiedDisplayMode;
    WNDPROC wndProc;
} game = {
    .monitorInfo = {.cbSize = sizeof(MONITORINFOEXW)},
    .devMode = {.dmSize = sizeof(DEVMODEW),
                .dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY},
    .userSpecifiedDisplayMode = TRUE};
UINT WM_SHELLHOOKMESSAGE;

LONG NtSetTimerResolution(ULONG DesiredResolution, BOOL SetResolution, PULONG CurrentResolution);
LONG NtQueryTimerResolution(PULONG MinimumResolution, PULONG MaximumResolution, PULONG CurrentResolution);

int atoi_s(const char *str)
{
    if (strspn(str, "0123456789-+") != strlen(str))
        return 0;
    return atoi(str);
}

LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        ShowWindow(hWnd, SW_MINIMIZE);
        break;

    case WM_SIZE:
        if (game.userSpecifiedDisplayMode)
        {
            if (!wParam)
                ChangeDisplaySettingsExW(game.monitorInfo.szDevice, &game.devMode, 0, CDS_FULLSCREEN, NULL);
            else if (wParam == SIZE_MINIMIZED)
                ChangeDisplaySettingsExW(game.monitorInfo.szDevice, NULL, 0, CDS_FULLSCREEN, NULL);
        };
        break;

    case WM_WINDOWPOSCHANGING:
        WINDOWPOS *wndPos = (WINDOWPOS *)lParam;
        wndPos->hwndInsertAfter = HWND_TOPMOST;
        wndPos->x = game.monitorInfo.rcMonitor.left;
        wndPos->y = game.monitorInfo.rcMonitor.top;
        wndPos->cx = game.cx;
        wndPos->cy = game.cy;
        break;

    case WM_STYLECHANGING:
        if (wParam == GWL_STYLE)
            ((STYLESTRUCT *)lParam)->styleNew = WS_VISIBLE | WS_POPUP;
        else if (wParam == GWL_EXSTYLE)
            ((STYLESTRUCT *)lParam)->styleNew = WS_EX_APPWINDOW;
        break;

    default:
        if (uMsg == WM_SHELLHOOKMESSAGE &&
            (wParam == HSHELL_WINDOWACTIVATED ||
             wParam == HSHELL_RUDEAPPACTIVATED))
        {
            HWND hAWnd = (HWND)lParam;
            if (hAWnd == hWnd && IsIconic(hWnd))
                ShowWindow(hWnd, SW_RESTORE);
            else if (hAWnd != hWnd &&
                     !IsIconic(hWnd) &&
                     MonitorFromWindow(hAWnd, MONITOR_DEFAULTTONEAREST) == game.hMonitor)
                ShowWindow(hWnd, SW_MINIMIZE);
        };
    }

    return CallWindowProcW(game.wndProc, hWnd, uMsg, wParam, lParam);
}

void WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hWnd,
    LONG idObject,
    LONG idChild,
    DWORD idEventThread,
    DWORD dwmsEventTime)
{
    if (event != EVENT_OBJECT_SHOW &&
        idObject != OBJID_WINDOW &&
        idChild != CHILDID_SELF)
        return;
    UnhookWinEvent(hWinEventHook);
    game.wndProc = (WNDPROC)GetWindowLongPtr(hWnd, GWLP_WNDPROC);
    game.hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);

    size_t size;
    int opt, argc;
    ULONG min, max, cur;
    DEVMODEW currentDevMode;
    BOOL vAttribute = TRUE,
         forceFullscreen;
    HANDLE hProcess = GetCurrentProcess(),
           hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, idEventThread);
    WCHAR **wArgv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **aArgv = alloca(sizeof(char *) * argc);
    static struct option options[] = {
        {"width", required_argument, 0, 1},
        {"height", required_argument, 0, 2},
        {"refresh", required_argument, 0, 3},
        {"fullscreen", no_argument, 0, 4},
        {"dll", required_argument, 0, 5},
        {0, 0, 0, 0}};

    for (int i = 0; i < argc; i++)
    {
        size = wcslen(wArgv[i]) + 1;
        aArgv[i] = alloca(size);
        wcstombs(aArgv[i], wArgv[i], size);
    };
    while ((opt = getopt_long_only(argc, aArgv, "", options, 0)) != -1)
    {
        switch (opt)
        {
        case 1:
            game.devMode.dmPelsWidth = atoi_s(optarg);
            break;
        case 2:
            game.devMode.dmPelsHeight = atoi_s(optarg);
            break;
        case 3:
            game.devMode.dmDisplayFrequency = atoi_s(optarg);
            break;
        case 4:
            forceFullscreen = TRUE;
            break;
        case 5:
            size = GetFullPathNameW(wArgv[optind - 1], 0, NULL, NULL);
            WCHAR *dll = alloca(sizeof(WCHAR) * size);
            GetFullPathNameW(wArgv[optind - 1], size, dll, NULL);
            _wcslwr_s(dll, size);
            LoadLibraryW(dll);
        };
    };
    LocalFree(wArgv);

    RegisterShellHookWindow(hWnd);
    DwmEnableMMCSS(TRUE);

    NtQueryTimerResolution(&min, &max, &cur);
    NtSetTimerResolution(max, TRUE, &cur);

    SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
    SetProcessPriorityBoost(hProcess, FALSE);
    CloseHandle(hProcess);

    GetMonitorInfoW(game.hMonitor, (MONITORINFO *)(&game.monitorInfo));
    EnumDisplaySettingsW(game.monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &currentDevMode);

    if ((currentDevMode.dmPelsWidth == game.devMode.dmPelsWidth &&
         currentDevMode.dmPelsHeight == game.devMode.dmPelsHeight &&
         currentDevMode.dmDisplayFrequency == game.devMode.dmDisplayFrequency) ||
        !(game.devMode.dmPelsWidth |
          game.devMode.dmPelsHeight |
          game.devMode.dmDisplayFrequency))
        game.userSpecifiedDisplayMode = FALSE;

    if (GetWindowLongPtrW(hWnd, GWL_STYLE) == (WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS) || forceFullscreen)
    {
        DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &vAttribute, 4);
        DwmSetWindowAttribute(hWnd, DWMWA_DISALLOW_PEEK, &vAttribute, 4);
        DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &vAttribute, 4);

        if (game.userSpecifiedDisplayMode)
            game.userSpecifiedDisplayMode = !ChangeDisplaySettingsExW(game.monitorInfo.szDevice, &game.devMode, 0, CDS_FULLSCREEN, NULL);

        GetMonitorInfoW(game.hMonitor, (MONITORINFO *)&game.monitorInfo);
        game.cx = game.monitorInfo.rcMonitor.right - game.monitorInfo.rcMonitor.left;
        game.cy = game.monitorInfo.rcMonitor.bottom - game.monitorInfo.rcMonitor.top;

        SetWindowLongPtrW(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        SetWindowLongPtrW(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
        SetWindowPos(hWnd, HWND_TOPMOST, game.monitorInfo.rcMonitor.left, game.monitorInfo.rcMonitor.top, game.cx, game.cy, SWP_NOSENDCHANGING);
        SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
    };

    PostQuitMessage(0);
}

DWORD MainThread()
{
    MSG msg;
    opterr = 0;
    WM_SHELLHOOKMESSAGE = RegisterWindowMessageW(L"SHELLHOOK");
    SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, 0, WinEventProc, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    while (GetMessageW(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    };

    return TRUE;
}

BOOL DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        CreateThread(0, 0, MainThread, 0, 0, 0);
    return TRUE;
}