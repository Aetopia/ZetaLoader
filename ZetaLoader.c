#include <windows.h>
#include <getopt.h>
#include <dwmapi.h>
#include <shlwapi.h>

struct
{
    DEVMODEW devMode;
    MONITORINFOEXW monitorInfo;
    HMONITOR hMonitor;
    int wndX, wndY, wndCX, wndCY;
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

DWORD ForegroundWindowLock(LPVOID lParam)
{
    HANDLE hThread = GetCurrentThread();
    HWND hWnd = (HWND)(lParam);
    SetThreadPriority(hThread, THREAD_MODE_BACKGROUND_BEGIN);
    SetThreadPriorityBoost(hThread, TRUE);
    CloseHandle(hThread);
    while (TRUE)
        SwitchToThisWindow(hWnd, TRUE);
    return 0;
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
        wndPos->x = game.wndX;
        wndPos->y = game.wndY;
        wndPos->cx = game.wndCX;
        wndPos->cy = game.wndCY;
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

    DWORD timeout;
    BOOL vAttribute = TRUE;
    ULONG min, max, cur;
    int opt, argc;
    size_t size;
    HANDLE hProcess = GetCurrentProcess(),
           hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, idEventThread);
    DEVMODEW currentDevMode = {.dmSize = sizeof(DEVMODEW)};
    WCHAR **wArgv = CommandLineToArgvW(GetCommandLineW(), &argc);
    char **aArgv = alloca(sizeof(char *) * argc);
    static struct option options[] = {
        {"width", required_argument, 0, 1},
        {"height", required_argument, 0, 2},
        {"refresh", required_argument, 0, 3},
        {"dll", required_argument, 0, 4},
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
    SystemParametersInfoW(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)&timeout, 0);

    NtQueryTimerResolution(&min, &max, &cur);
    NtSetTimerResolution(max, TRUE, &cur);

    SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS);
    SetProcessPriorityBoost(hProcess, FALSE);
    CloseHandle(hProcess);

    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
    SetThreadPriorityBoost(hThread, FALSE);
    CloseHandle(hThread);

    GetMonitorInfoW(game.hMonitor, (MONITORINFO *)(&game.monitorInfo));
    EnumDisplaySettingsW(game.monitorInfo.szDevice, ENUM_CURRENT_SETTINGS, &currentDevMode);

    if ((currentDevMode.dmPelsWidth == game.devMode.dmPelsWidth &&
         currentDevMode.dmPelsHeight == game.devMode.dmPelsHeight &&
         currentDevMode.dmDisplayFrequency == game.devMode.dmDisplayFrequency) ||
        !(game.devMode.dmPelsWidth |
          game.devMode.dmPelsHeight |
          game.devMode.dmDisplayFrequency))
        game.userSpecifiedDisplayMode = FALSE;

    if (GetWindowLongPtr(hWnd, GWL_STYLE) == (WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS))
    {
        SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, 0);
        hThread = CreateThread(NULL, 0, ForegroundWindowLock, (LPVOID)&hWnd, 0, NULL);

        DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &vAttribute, 4);
        DwmSetWindowAttribute(hWnd, DWMWA_DISALLOW_PEEK, &vAttribute, 4);
        DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &vAttribute, 4);

        if (game.userSpecifiedDisplayMode)
            game.userSpecifiedDisplayMode = !ChangeDisplaySettingsExW(game.monitorInfo.szDevice, &game.devMode, 0, CDS_FULLSCREEN, NULL);

        GetMonitorInfoW(game.hMonitor, (MONITORINFO *)(&game.monitorInfo));
        game.wndX = game.monitorInfo.rcMonitor.left;
        game.wndY = game.monitorInfo.rcMonitor.top;
        game.wndCX = game.monitorInfo.rcMonitor.right - game.monitorInfo.rcMonitor.left;
        game.wndCY = game.monitorInfo.rcMonitor.bottom - game.monitorInfo.rcMonitor.top;

        SetWindowLongPtrW(hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
        SetWindowLongPtrW(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW);
        SetWindowPos(hWnd, HWND_TOPMOST, game.wndX, game.wndY, game.wndCX, game.wndCY, SWP_NOSENDCHANGING);
        SetWindowLongPtrW(hWnd, GWLP_WNDPROC, (LONG_PTR)(WndProc));

        TerminateThread(hThread, 0);
        CloseHandle(hThread);

        if (timeout)
            SystemParametersInfoW(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, (LPVOID)&timeout, 0);
    };

    PostQuitMessage(0);
}

DWORD MainThread()
{
    MSG msg;
    opterr = 0;
    WM_SHELLHOOKMESSAGE = RegisterWindowMessage("SHELLHOOK");
    SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, 0, WinEventProc, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
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