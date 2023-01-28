import winim/[lean, inc/dwmapi], os, strutils, tables

type Game = object
    devMode: DEVMODE
    monitor: string
    x, y, cx, cy: int32
    isPrimaryMonitor, isForegroundWnd: bool
    wndProc: WNDPROC
    cfg: OrderedTableRef[string, OrderedTableRef[string, string]]
var game: Game
game.devMode.dmSize = sizeof(DEVMODE).WORD
game.devMode.dmFields = DM_PELSWIDTH or DM_PELSHEIGHT
game.isPrimaryMonitor = true

# Get the value of a key in a section of a configuration file.
proc getSectionValue(cfg: OrderedTableRef[string, OrderedTableRef[string,
        string]], section, key: string): string =
    if cfg.hasKey(section):
        if cfg[section].hasKey(key):
            return cfg[section][key]
    return ""

# Read a configuration file.
proc readCfg(filename: string): OrderedTableRef[string, OrderedTableRef[string, string]] =
    var
        cfg = newOrderedTable[string, OrderedTableRef[string, string]]()
        section: string
    cfg[""] = newOrderedTable[string, string]()
    for i in readFile(filename).strip(chars = {'\n', ' '}).splitLines():
        let line = i.strip()
        if line.len == 0: continue
        elif [line[0], line[^1]] == ['[', ']']:
            section = line.strip(chars = {'[', ']', ' '}).toLower()
            cfg[section] = newOrderedTable[string, string]()
        else:
            let
                keyvalue = line.split("=", 1)
                key = keyvalue[0].strip().toLower()
                value = keyvalue[1].strip()
            if key != "" and value != "":
                cfg[section][key.toLower()] = value
    return cfg

proc NtSetTimerResolution(DesiredResolution: ULONG, SetResolution: BOOLEAN,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}
proc NtQueryTimerResolution(MinimumResolution, MaximumResolution,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}

# Wrapper around ChangeDisplaySettingsEx.
proc setDM(dm: ptr DEVMODE) {.inline.} = ChangeDisplaySettingsEx(game.monitor,
        dm, 0, CDS_FULLSCREEN, nil)

# Constantly bring the game window into the foreground.
proc foregroundWndLock(lParam: LPVOID): DWORD {.stdcall.} =
    let
        hThread = GetCurrentThread()
        hWnd = cast[HWND](lParam)
    SetThreadPriority(hThread, THREAD_MODE_BACKGROUND_BEGIN)
    SetThreadPriorityBoost(hThread, false)
    CloseHandle(hThread)
    while not game.isForegroundWnd:
        SwitchToThisWindow(hWnd, true)
    return 0

# ZetaLoader's Window Procedure.
proc wndProc(hWnd: HWND, msg: UINT, wParam: WPARAM,
        lParam: LPARAM): LRESULT {.stdcall.} =
    case msg:
    # Revert the resolution back to the desktop resolution when the game is being closed.
    of WM_CLOSE, WM_DESTROY, WM_QUIT:
        if not game.isPrimaryMonitor:
            setDM(nil)

    # Processing WM_ACTIVATE & WM_ACTIVATEAPP to allow the game's window to automatically minimize and reset the resolution for multitasking on the primary monitor.
    of WM_ACTIVATE, WM_ACTIVATEAPP:
        if game.isPrimaryMonitor:
            case wParam:
            of WA_ACTIVE, WA_CLICKACTIVE:
                if IsIconic(hWnd): ShowWindow(hWnd, SW_RESTORE)
                setDM(addr game.devMode)
            of WA_INACTIVE:
                if not IsIconic(hWnd): ShowWindow(hWnd, SW_MINIMIZE)
                setDM(nil)
            else: discard

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

    setCurrentDir(getAppDir())
    if not fileExists("ZetaLoader.ini"):
        writeFile("ZetaLoader.ini", "")
    let
        timeout: DWORD = 0
        min, max, cur: ULONG = 0
        hProcess = GetCurrentProcess()
        hThread = OpenThread(THREAD_SET_INFORMATION, FALSE,
                GetWindowThreadProcessId(hWnd, nil))
        vAttribute = TRUE
    var
        cfg = readCfg("ZetaLoader.ini")
        (width, height) = (cfg.getSectionValue("", "width"),
                cfg.getSectionValue("", "height"))
        hMonitor: HMONITOR
        mi: MONITORINFOEX
        dm: DEVMODE
    mi.cbSize = sizeof(MONITORINFOEX).DWORD
    game.wndProc = cast[WNDPROC](GetWindowLongPtr(hWnd, GWLP_WNDPROC))

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
    GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr mi))
    for i in mi.szDevice:
        if i != 0: game.monitor.add(cast[char](i))
    # Disable any features of ZetaLoader that are only available if the game is running on the primary monitor.
    if mi.dwFlags != MONITORINFOF_PRIMARY:
        game.isPrimaryMonitor = false
    EnumDisplaySettings(game.monitor, ENUM_CURRENT_SETTINGS, addr dm)
    game.devMode.dmPelsWidth = dm.dmPelsWidth
    game.devMode.dmPelsHeight = dm.dmPelsHeight

    # Fetch user specified resolution or use the current resolution.
    try:
        game.devMode.dmPelsWidth = width.parseInt().DWORD
        game.devMode.dmPelsHeight = height.parseInt().DWORD
    except ValueError:
        writeFile("ZetaLoader.ini", "Width = " & $dm.dmPelsWidth &
                "\nHeight = " & $dm.dmPelsHeight)

    # Check if the user specified resolution is not supported by the monitor, is the native resolution or if the user specified resolution is 0.
    if ChangeDisplaySettingsEx(game.monitor, addr game.devMode, 0, CDS_TEST,
            nil) != DISP_CHANGE_SUCCESSFUL or
        (dm.dmPelsWidth == game.devMode.dmPelsWidth and dm.dmPelsHeight ==
                game.devMode.dmPelsHeight) or
        (game.devMode.dmPelsHeight or game.devMode.dmPelsWidth) == 0:
        game.devMode.dmFields = 0

    # Set the Foreground Lock Timeout to 0 and lock the foreground window to the game's window.
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](0), 0)
    CreateThread(nil, 0, cast[PTHREAD_START_ROUTINE](foregroundWndLock), cast[
            LPVOID](hWnd), 0, nil)

    # Execute the `if` block if the game is using borderless fullscreen.
    # 1. Disable the window transitions, disable the peek feature, and force the iconic representation of the window.
    # 2. Apply the user specified resolution.
    # 3. Use WS_VISIBLE | WS_POPUP and WS_EX_APPWINDOW for the game's borderless fullscreen.
    # 4. Redirect the game's window procedure to ZetaLoader's window procedure.
    if GetWindowLongPtr(hWnd, GWL_STYLE) == (WS_VISIBLE or WS_OVERLAPPED or
            WS_CLIPSIBLINGS):
        DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED,
                unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_DISALLOW_PEEK, unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION,
                unsafeAddr vAttribute, 4)
        setDM(addr game.devMode)
        GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr mi))
        SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE or WS_POPUP)
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW)
        (game.x, game.y, game.cx, game.cy) = (mi.rcMonitor.left,
                mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                mi.rcMonitor.bottom -
                mi.rcMonitor.top)
        SetWindowPos(hWnd, HWND_TOPMOST, game.x, game.y, game.cx, game.cy,
                SWP_NOACTIVATE or SWP_NOSENDCHANGING)
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, cast[LONG_PTR](wndProc))
        if timeout != 0:
            SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
                    unsafeAddr timeout), 0)
        game.isForegroundWnd = true

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
