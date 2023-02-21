import winim/[lean, inc/dwmapi], strutils, parseopt, os

type Game = object
    devMode: DEVMODE
    szDevice: string
    wndX, wndY, wndCX, wndCY: int32
    userSpecifiedDisplayMode: bool
    wndProc: WNDPROC
let WM_SHELLHOOKMESSAGE = RegisterWindowMessage("SHELLHOOK")
var game: Game
game.devMode.dmSize = sizeof(DEVMODE).WORD
game.devMode.dmFields = DM_PELSWIDTH or DM_PELSHEIGHT or DM_DISPLAYFREQUENCY
game.userSpecifiedDisplayMode = true

proc NtSetTimerResolution(DesiredResolution: ULONG, SetResolution: BOOLEAN,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}
proc NtQueryTimerResolution(MinimumResolution, MaximumResolution,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}

# Constantly bring the game window into the foreground.
proc foregroundWndLock(lParam: LPVOID): DWORD {.stdcall.} =
    let
        hThread = GetCurrentThread()
        hWnd = cast[HWND](lParam)
    SetThreadPriority(hThread, THREAD_MODE_BACKGROUND_BEGIN)
    SetThreadPriorityBoost(hThread, false)
    CloseHandle(hThread)
    while true: SwitchToThisWindow(hWnd, true)
    return 0

converter wCharArrayToString(wCharArray: openarray[WCHAR]): string =
    var str: string
    for c in wCharArray:
        if c != 0: str.add(cast[char](c))
    return str

# ZetaLoader's Window Procedure.
proc wndProc(hWnd: HWND, msg: UINT, wParam: WPARAM,
        lParam: LPARAM): LRESULT {.stdcall.} =
    case msg:
    # - Reset the resolution when the game window receives WM_CLOSE or WM_DESTROY.
    of WM_CLOSE, WM_DESTROY: ShowWindow(hWnd, SW_MINIMIZE)
    of WM_SIZE:
        if game.userSpecifiedDisplayMode:
            case wParam:
            of SIZE_RESTORED:
                ChangeDisplaySettingsEx(game.szDevice, addr game.devMode, 0,
                        CDS_FULLSCREEN, nil)
            of SIZE_MINIMIZED:
                ChangeDisplaySettingsEx(game.szDevice, nil, 0,
                        CDS_FULLSCREEN, nil)
            else: discard

    # Processing WM_WINDOWPOSCHANGING & WM_STYLECHANGING to prevent Halo Infinite's borderless fullscreen from getting disabled.
    of WM_WINDOWPOSCHANGING:
        let wndPos = cast[ptr WINDOWPOS](lParam)
        wndPos.hwndInsertAfter = HWND_TOPMOST
        wndPos.x = game.wndX
        wndPos.y = game.wndY
        wndPos.cx = game.wndCX
        wndPos.cy = game.wndCY
    of WM_STYLECHANGING:
        if wParam == GWL_STYLE:
            cast[ptr STYLESTRUCT](lParam).styleNew = WS_VISIBLE or WS_POPUP
        elif wParam == GWL_EXSTYLE:
            cast[ptr STYLESTRUCT](lParam).styleNew = WS_EX_APPWINDOW

    else:
        # Allow the game to tabbed out from the monitor as long as the window becoming the foreground window is on the monitor, the game is running on.
        # Using WM_SHELLHOOKMESSAGE to detect when the foreground window changes, even when the game is not the foreground window.
        if msg == WM_SHELLHOOKMESSAGE and [HSHELL_WINDOWACTIVATED,
                HSHELL_RUDEAPPACTIVATED].contains(int(wParam)):
            let activatedhWnd = HWND(lParam)
            if activatedhWnd == hWnd and IsIconic(hWnd) == TRUE:
                ShowWindow(hWnd, SW_RESTORE)
            elif activatedhWnd != hWnd and IsIconic(hWnd) == FALSE:
                var monitorInfo: MONITORINFOEX
                monitorInfo.cbSize = sizeof(monitorInfo).DWORD
                GetMonitorInfo(MonitorFromWindow(lParam.HWND,
                        MONITOR_DEFAULTTONEAREST), cast[ptr MONITORINFO](
                        addr monitorInfo))
                if monitorInfo.szDevice.wCharArrayToString() ==
                        game.szDevice:
                    ShowWindow(hWnd, SW_MINIMIZE)

    return CallWindowProc(game.wndProc, hwnd, msg, wParam, lParam)

proc winEventProc(hWinEventHook: HWINEVENTHOOK, event: DWORD, hWnd: HWND,
        idObject, idChild: LONG, idEventThread,
                dwmsEventTime: DWORD) {.stdcall.} =
    if event != EVENT_OBJECT_SHOW and idobject != OBJID_WINDOW and idchild != CHILDID_SELF:
        return
    UnhookWinEvent(hWinEventHook)

    let
        timeout: DWORD = 0
        min, max, cur: ULONG = 0
        hProcess = GetCurrentProcess()
        vAttribute = true
    var
        argDisplayMode: bool
        hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, idEventThread)
        devMode: DEVMODE
        hMonitor: HMONITOR
        monitorInfo: MONITORINFOEX
    monitorInfo.cbSize = sizeof(MONITORINFOEX).DWORD
    game.wndProc = cast[WNDPROC](GetWindowLongPtr(hWnd, GWLP_WNDPROC))

    for kind, key, value in getopt():
        if kind != cmdLongOption: continue

        if key == "displaymode" and not argDisplayMode:
            argDisplayMode = true
            try:
                let
                    param = value.split("_", 1)
                    resolution = param[0].split("x", 1)
                game.devMode.dmPelsWidth = resolution[0].parseInt.DWORD
                game.devMode.dmPelsHeight = resolution[1].parseInt.DWORD
                game.devMode.dmDisplayFrequency = param[1].parseInt.DWORD
            except ValueError: discard

        elif key == "dll":
            LoadLibrary(winstrConverterStringToLPWSTR(absolutePath(
                    value).toLower()))

    # 1. Set the process priority to above normal.
    # 2. Set the timer resolution to 0.5 ms.
    # 3. Enable Multimedia Class Schedule Service Scheduling (MMCSS) for Halo Infinite.
    RegisterShellHookWindow(hWnd)
    SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS)
    SetProcessPriorityBoost(hProcess, false)
    CloseHandle(hProcess)

    NtQueryTimerResolution(unsafeAddr min, unsafeAddr max, unsafeAddr cur)
    NtSetTimerResolution(max, TRUE, unsafeAddr cur)
    DwmEnableMMCSS(true)
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
            unsafeAddr timeout), 0)

    # Set the window thread priority to time critical which resolves intense screen tearing and jittery/stutter mouse input when an external framerate limiter is being used.
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL)
    SetThreadPriorityBoost(hThread, FALSE)
    CloseHandle(hThread)

    # Get the monitor, the game's window is on.
    hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST)
    GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr monitorInfo))
    game.szDevice = monitorInfo.szDevice.wCharArrayToString()
    EnumDisplaySettings(game.szDevice, ENUM_CURRENT_SETTINGS,
            addr devMode)

    # Check if the user specified resolution is not supported by the monitor or is the native resolution.
    if ChangeDisplaySettingsEx(game.szDevice, addr game.devMode, 0, CDS_TEST,
            nil) != DISP_CHANGE_SUCCESSFUL or
        (devMode.dmPelsWidth == game.devMode.dmPelsWidth and
                devMode.dmPelsHeight ==
                game.devMode.dmPelsHeight and devMode.dmDisplayFrequency ==
                game.devMode.dmDisplayFrequency) or
        (game.devMode.dmPelsHeight or game.devMode.dmPelsWidth or
                game.devMode.dmDisplayFrequency) == 0:
        game.userSpecifiedDisplayMode = false

    # ZetaLoader Borderless Fullscreen + User Defined Resolution Support
    if GetWindowLongPtr(hWnd, GWL_STYLE) == (WS_VISIBLE or WS_OVERLAPPED or
            WS_CLIPSIBLINGS):

        # 1. Set the Foreground lock timeout to 0 and lock the foreground window to the game's window.
        SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](0), 0)
        hThread = CreateThread(nil, 0, cast[PTHREAD_START_ROUTINE](
                foregroundWndLock), cast[LPVOID](hWnd), 0, nil)

        # 2. Disable the window transitions, disable the peek feature, and force the iconic representation of the window.
        DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED,
                unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_DISALLOW_PEEK, unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION,
                unsafeAddr vAttribute, 4)

        # 3. Apply the user specified resolution.
        if game.userSpecifiedDisplayMode:
            ChangeDisplaySettingsEx(game.szDevice, addr game.devMode, 0,
                    CDS_FULLSCREEN, nil)
        GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr monitorInfo))
        (game.wndX, game.wndY, game.wndCX, game.wndCY) = (monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right -
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom -
                monitorInfo.rcMonitor.top)

        # 4. Setup ZetaLoader's Borderless Fullscreen implementation.
        # - Use WS_VISIBLE | WS_POPUP and WS_EX_APPWINDOW for the game's borderless fullscreen
        # - Resize the game's window to match the user defined resolution.
        # - Redirect the game's window procedure to ZetaLoader's window procedure.
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE or WS_POPUP)
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW)
        SetWindowPos(hWnd, HWND_TOPMOST, game.wndX, game.wndY, game.wndCX,
                game.wndCY, SWP_NOSENDCHANGING)
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, cast[LONG_PTR](wndProc))

        # 5. Revert the Foreground lock timeout to default and unlock the foreground window.
        TerminateThread(hThread, 0)
        CloseHandle(hThread)
        if timeout != 0:
            SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
                    unsafeAddr timeout), 0)

    PostQuitMessage(0)

proc mainThread(): DWORD {.stdcall.} =
    var msg: MSG
    SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, 0, winEventProc,
            GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT)
    while GetMessage(addr msg, 0, 0, 0):
        TranslateMessage(addr msg)
        DispatchMessage(addr msg)
    return 0

when isMainModule:
    CreateThread(nil, 0, cast[PTHREAD_START_ROUTINE](mainThread), nil, 0, nil)
