import winim/[lean, inc/dwmapi], os, strutils

type Game = object
    devMode: DEVMODE
    monitor: string
    x, y, cx, cy: int32
    userDefinedDisplayMode: bool
    wndProc: WNDPROC
var game: Game
game.devMode.dmSize = sizeof(DEVMODE).WORD
game.devMode.dmFields = DM_PELSWIDTH or DM_PELSHEIGHT or DM_DISPLAYFREQUENCY
game.userDefinedDisplayMode = true

proc NtSetTimerResolution(DesiredResolution: ULONG, SetResolution: BOOLEAN,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}
proc NtQueryTimerResolution(MinimumResolution, MaximumResolution,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}

# Wrapper around ChangeDisplaySettingsEx.
proc setDM(dm: ptr DEVMODE) {.inline.} =
    if game.userDefinedDisplayMode: ChangeDisplaySettingsEx(game.monitor, dm, 0,
            CDS_FULLSCREEN, nil)

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

converter wCharArrayToString(wCharArray: array[CCHDEVICENAME, WCHAR]): string =
    var str: string
    for c in wCharArray:
        if c != 0: str.add(cast[char](c))
    return str


# ZetaLoader's Window Procedure.
proc wndProc(hWnd: HWND, msg: UINT, wParam: WPARAM,
        lParam: LPARAM): LRESULT {.stdcall.} =
    case msg:
    # Processing WM_ACTIVATE & WM_ACTIVATEAPP
    # - Allow the game to be tabbed in from any monitor.
    # - Allow the game to tabbed out from the monitor, the game is running on.
    of WM_ACTIVATE:
        case wParam:
        of WA_ACTIVE, WA_CLICKACTIVE:
            if IsIconic(hWnd): SwitchToThisWindow(hWnd, TRUE)
            setDM(addr game.devMode)
        of WA_INACTIVE:
            var
                pt: POINT
                monitorInfo: MONITORINFOEX
            monitorInfo.cbSize = sizeof(MONITORINFOEX).DWORD
            GetCursorPos(addr pt)
            GetMonitorInfo(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), cast[
                    ptr MONITORINFO](addr monitorInfo))
            if monitorInfo.szDevice.wCharArrayToString == game.monitor:
                if not IsIconic(hWnd): ShowWindow(hWnd, SW_MINIMIZE)
                setDM(nil)
        else: discard
    of WM_ACTIVATEAPP:
        if wParam == FALSE:
            if not IsIconic(hWnd): ShowWindow(hWnd, SW_MINIMIZE)
            setDM(nil)

    # Processing WM_WINDOWPOSCHANGING & WM_STYLECHANGING to prevent Halo Infinite's borderless fullscreen from getting disabled.
    of WM_WINDOWPOSCHANGING:
        let wndPos = cast[ptr WINDOWPOS](lParam)
        wndPos.hwndInsertAfter = HWND_TOPMOST
        wndPos.x = game.x
        wndPos.y = game.y
        wndPos.cx = game.cx
        wndPos.cy = game.cy
    of WM_STYLECHANGING:
        if wParam == GWL_STYLE:
            cast[ptr STYLESTRUCT](lParam).styleNew = WS_VISIBLE or WS_POPUP
        elif wParam == GWL_EXSTYLE:
            cast[ptr STYLESTRUCT](lParam).styleNew = WS_EX_APPWINDOW
    else: discard

    return CallWindowProc(game.wndProc, hwnd, msg, wParam, lParam)

proc winEventProc(hWinEventHook: HWINEVENTHOOK, event: DWORD, hWnd: HWND,
        idObject: LONG, idChild: LONG, idEventThread: DWORD,
        dwmsEventTime: DWORD) {.stdcall.} =
    if event != EVENT_OBJECT_SHOW and idobject != OBJID_WINDOW and idchild != CHILDID_SELF:
        return
    UnhookWinEvent(hWinEventHook)

    let
        timeout: DWORD = 0
        min, max, cur: ULONG = 0
        hProcess = GetCurrentProcess()
        vAttribute = TRUE
        cmdline = commandLineParams()
    var
        argdisplayMode: bool
        hThread = OpenThread(THREAD_SET_INFORMATION, FALSE,
                GetWindowThreadProcessId(hWnd, nil))
        devMode: DEVMODE
        hMonitor: HMONITOR
        monitorInfo: MONITORINFOEX
    monitorInfo.cbSize = sizeof(MONITORINFOEX).DWORD
    game.wndProc = cast[WNDPROC](GetWindowLongPtr(hWnd, GWLP_WNDPROC))

    for i in 0..cmdline.len-1:
        let arg = cmdline[i].toLower().strip()
        case arg:
        of "/dm", "/displaymode":
            if argdisplayMode: continue
            argdisplayMode = true
            try:
                let
                    param = cmdline[i+1].toLower().split("_", 1)
                    resolution = param[0].strip().split("x", 1)
                game.devMode.dmPelsWidth = resolution[0].strip().parseInt.DWORD
                game.devMode.dmPelsHeight = resolution[1].strip().parseInt.DWORD
                game.devMode.dmDisplayFrequency = param[1].strip().parseInt.DWORD
            except ValueError, IndexDefect: discard

    # 1. Set the process priority to above normal.
    # 2. Set the timer resolution to 0.5 ms.
    # 3. Enable Multimedia Class Scheduler Service (MMCSS) for Halo Infinite.
    SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS)
    SetProcessPriorityBoost(hProcess, false)
    CloseHandle(hProcess)
    NtQueryTimerResolution(unsafeAddr min, unsafeAddr max, unsafeAddr cur)
    NtSetTimerResolution(max, TRUE, unsafeAddr cur)
    DwmEnableMMCSS(true)
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
            unsafeAddr timeout), 0)

    # Set the window thread priority to time critical which resolves jittery/stuttery input for Mouse & Keyboard.
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL)
    SetThreadPriorityBoost(hThread, FALSE)
    CloseHandle(hThread)

    # Get the monitor, the game's window is on.
    hMonitor = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST)
    GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr monitorInfo))
    game.monitor = monitorInfo.szDevice.wCharArrayToString
    EnumDisplaySettings(game.monitor, ENUM_CURRENT_SETTINGS,
            addr devMode)

    # Check if the user specified resolution is not supported by the monitor or is the native resolution.
    if ChangeDisplaySettingsEx(game.monitor, addr game.devMode, 0, CDS_TEST,
            nil) != DISP_CHANGE_SUCCESSFUL or
        (devMode.dmPelsWidth == game.devMode.dmPelsWidth and
                devMode.dmPelsHeight ==
                game.devMode.dmPelsHeight and devMode.dmDisplayFrequency ==
                game.devMode.dmDisplayFrequency) or
        (game.devMode.dmPelsHeight or game.devMode.dmPelsWidth or
                game.devMode.dmDisplayFrequency) == 0:
        game.userDefinedDisplayMode = false

    # ZetaLoader Borderless Fullscreen + User Defined Resolution Support
    if GetWindowLongPtr(hWnd, GWL_STYLE) == (WS_VISIBLE or WS_OVERLAPPED or
            WS_CLIPSIBLINGS):

        # Set the Foreground lock timeout to 0 and lock the foreground window to the game's window.
        SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](0), 0)
        hThread = CreateThread(nil, 0, cast[PTHREAD_START_ROUTINE](
                foregroundWndLock), cast[LPVOID](hWnd), 0, nil)

        # 1. Disable the window transitions, disable the peek feature, and force the iconic representation of the window.
        DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED,
                unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_DISALLOW_PEEK, unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION,
                unsafeAddr vAttribute, 4)


        # 2. Apply the user specified resolution.
        setDM(addr game.devMode)
        GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr monitorInfo))
        (game.x, game.y, game.cx, game.cy) = (monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right -
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom -
                monitorInfo.rcMonitor.top)

        # 3. Setup ZetaLoader's Borderless Fullscreen implementation.
        # - Use WS_VISIBLE | WS_POPUP and WS_EX_APPWINDOW for the game's borderless fullscreen
        # - Resize the game's window to match the user defined resolution.
        # - Redirect the game's window procedure to ZetaLoader's window procedure.
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE or WS_POPUP)
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW)
        SetWindowPos(hWnd, HWND_TOPMOST, game.x, game.y, game.cx, game.cy,
                SWP_NOACTIVATE or SWP_NOSENDCHANGING)
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, cast[LONG_PTR](wndProc))

        # 4. Revert the Foreground lock timeout to default and unlock the foreground window.
        if timeout != 0:
            SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
                    unsafeAddr timeout), 0)
        TerminateThread(hThread, 0)
        CloseHandle(hThread)

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
