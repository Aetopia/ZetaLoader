import winim/[lean, shell], os, strutils, parsecfg

type WINDOW = object
    x, y, cx, cy: int32

type DLL = object
    hWnd: HWND
    devMode: DEVMODE
    pDevMode: ptr DEVMODE
    monitor: string
    wnd: WINDOW
    isPrimaryMonitor, isForegroundWnd: bool
    wndProc: WNDPROC
    cfg: Config

var dll: DLL
dll.pDevMode = addr dll.devMode
dll.devMode.dmSize = sizeof(DEVMODE).WORD
dll.devMode.dmFields = DM_PELSWIDTH or DM_PELSHEIGHT
dll.isPrimaryMonitor = true

proc NtSetTimerResolution(DesiredResolution: ULONG, SetResolution: BOOLEAN,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}
proc NtQueryTimerResolution(MinimumResolution, MaximumResolution,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}

# Wrapper around ChangeDisplaySettingsEx.
proc setDM(dm: ptr DEVMODE) =
    if dll.devMode.dmFields != 0:
        ChangeDisplaySettingsEx(dll.monitor, dm, 0, CDS_FULLSCREEN, nil)

# Constantly bring the game window into the foreground.
proc foregroundWndLock(): DWORD {.stdcall.} =
    let hthread = GetCurrentThread()
    SetThreadPriority(hthread, THREAD_MODE_BACKGROUND_BEGIN)
    SetThreadPriorityBoost(hthread, false)
    CloseHandle(hthread)
    while not dll.isForegroundWnd:
        SwitchToThisWindow(dll.hWnd, true)
    return 0

# ZetaLoader's Window Procedure.
proc wndProc(hWnd: HWND, msg: UINT, wParam: WPARAM,
        lParam: LPARAM): LRESULT {.stdcall.} =
    case msg:
    of WM_ACTIVATE, WM_ACTIVATEAPP:
        # Allow for the window to be minimized automatically if it's on the primary monitor.
        if dll.isPrimaryMonitor:
            case wParam:
            of WA_ACTIVE, WA_CLICKACTIVE:
                if IsIconic(hWnd):
                    SwitchToThisWindow(hWnd, TRUE)
                setDM(dll.pDevMode)
            of WA_INACTIVE:
                if not IsIconic(hWnd):
                    ShowWindow(hWnd, SW_MINIMIZE)
                setDM(nil)
            else: discard
    # Processing WM_WINDOWPOSCHANGING & WM_STYLECHANGING to prevent Halo Infinite's borderless fullscreen from getting disabled.
    of WM_WINDOWPOSCHANGING:
        var wndpos = cast[ptr WINDOWPOS](lParam)
        wndpos.hwnd = hwnd
        wndpos.hwndInsertAfter = HWND_TOPMOST
        wndpos.x = dll.wnd.x
        wndpos.y = dll.wnd.y
        wndpos.cx = dll.wnd.cx
        wndpos.cy = dll.wnd.cy
        wndpos.flags = SWP_NOACTIVATE or SWP_NOSENDCHANGING
    of WM_STYLECHANGING:
        if wParam == GWL_STYLE:
            cast[ptr STYLESTRUCT](lParam).styleNew = WS_VISIBLE or WS_POPUP
        elif wParam == GWL_EXSTYLE:
            cast[ptr STYLESTRUCT](lParam).styleNew = WS_EX_APPWINDOW
    else: discard
    return CallWindowProc(dll.wndProc, hwnd, msg, wParam, lParam)

proc WinEventProc(hWinEventHook: HWINEVENTHOOK, event: DWORD, hWnd: HWND,
        idObject: LONG, idChild: LONG, idEventThread: DWORD,
        dwmsEventTime: DWORD) {.stdcall.} =
    if event != EVENT_OBJECT_SHOW and idobject != OBJID_WINDOW and idchild != CHILDID_SELF:
        return
    UnhookWinEvent(hWinEventHook)

    let
        tid = GetWindowThreadProcessId(hwnd, nil)
        hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, tid)
    dll.hWnd = hWnd
    dll.wndProc = cast[WNDPROC](GetWindowLongPtr(hWnd, GWLP_WNDPROC))

    # Disable the window transitions, disable the peek feature, and force the iconic representation of the window.
    DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED,
            addr dll.isPrimaryMonitor, 4)
    DwmSetWindowAttribute(hwnd, DWMWA_DISALLOW_PEEK, addr dll.isPrimaryMonitor,
            4);
    DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION,
            addr dll.isPrimaryMonitor, 4)

    # Set the window thread priority to time critical which resolves jittery/stuttery input for Mouse & Keyboard.
    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL)
    SetThreadPriorityBoost(hThread, FALSE)
    CloseHandle(hThread)

    PostQuitMessage(0)

# Entry point for ZetaLoader
proc mainThread(): DWORD {.stdcall.} =
    let
        timeout, pid: DWORD = GetCurrentProcessId()
        min, max, cur: ULONG = 0
        hProcess = GetCurrentProcess()
    var
        cfg = loadConfig("ZetaLoader.ini")
        (width, height) = (cfg.getSectionValue("Display", "Width").strip(),
                cfg.getSectionValue("Display", "Height").strip())
        hMonitor: HMONITOR
        mi: MONITORINFOEX
        dm: DEVMODE
        msg: MSG
    mi.cbSize = sizeof(MONITORINFOEX).DWORD

    # 1. Set the process priority to above normal.
    # 2. Set the timer resolution to 0.5 ms.
    # 3. Enable Multimedia Class Scheduler Service (MMCSS) for Halo Infinite.
    # 4. Allow Halo Infinite to set the foreground window.
    # 5. Disable the foreground lock timeout.
    SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS)
    SetProcessPriorityBoost(hProcess, false)
    CloseHandle(hProcess)
    NtQueryTimerResolution(unsafeAddr min, unsafeAddr max, unsafeAddr cur)
    NtSetTimerResolution(max, TRUE, unsafeAddr cur)
    DwmEnableMMCSS(true)
    AllowSetForegroundWindow(pid)
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
            unsafeAddr timeout), 0)

    # Wait until Halo Infinite shows its window.
    SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, 0, WinEventProc, pid,
            0, WINEVENT_OUTOFCONTEXT)
    while GetMessage(addr msg, 0, 0, 0):
        TranslateMessage(addr msg)
        DispatchMessage(addr msg)

    # Get the monitor, the game's window is on.
    hMonitor = MonitorFromWindow(dll.hwnd, MONITOR_DEFAULTTONEAREST)
    GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr mi))
    for i in mi.szDevice:
        if i != 0: dll.monitor.add(cast[char](i))
    # Disable any features of ZetaLoader that are only available if the game is running on the primary monitor.
    if mi.dwFlags != MONITORINFOF_PRIMARY:
        dll.isPrimaryMonitor = false
    EnumDisplaySettings(dll.monitor, ENUM_CURRENT_SETTINGS, addr dm)

    # Fetch user specified resolution or use the current resolution.
    if width == "" or height == "":
        cfg.setSectionKey("Display", "Width", $dm.dmPelsWidth)
        cfg.setSectionKey("Display", "Height", $dm.dmPelsHeight)
        writeConfig(cfg, "ZetaLoader.ini")
        dll.devMode.dmPelsWidth = dm.dmPelsWidth
        dll.devMode.dmPelsHeight = dm.dmPelsHeight
    else:
        try:
            dll.devMode.dmPelsWidth = width.parseInt().DWORD
            dll.devMode.dmPelsHeight = height.parseInt().DWORD
            if (dll.devMode.dmPelsHeight or dll.devMode.dmPelsWidth) == 0:
                dll.devMode.dmFields = 0
        except ValueError:
            discard

    # Test if the user specified resolution is supported by the monitor or if the game is in borderless fullscreen.
    if ChangeDisplaySettingsEx(dll.monitor, dll.pDevMode, 0, CDS_TEST,
            nil) != DISP_CHANGE_SUCCESSFUL or GetWindowLongPtr(dll.hwnd,
                    GWL_STYLE) != (WS_VISIBLE or WS_OVERLAPPED or
                    WS_CLIPSIBLINGS):
        return 0

    # Use WS_VISIBLE | WS_POPUP and WS_EX_APPWINDOW for the game's borderless fullscreen.
    if dm.dmPelsWidth == dll.devMode.dmPelsWidth and dm.dmPelsHeight ==
            dll.devMode.dmPelsHeight:
        dll.devMode.dmFields = 0
    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](0), 0)
    CreateThread(nil, 0, cast[PTHREAD_START_ROUTINE](foregroundWndLock), nil, 0, nil)
    setDM(dll.pDevMode)
    GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr mi))
    SetWindowLongPtr(dll.hwnd, GWL_STYLE, WS_VISIBLE or WS_POPUP)
    SetWindowLongPtr(dll.hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW)
    dll.wnd.x = mi.rcMonitor.left
    dll.wnd.y = mi.rcMonitor.top
    dll.wnd.cx = mi.rcMonitor.right - mi.rcMonitor.left
    dll.wnd.cy = mi.rcMonitor.bottom - mi.rcMonitor.top
    SetWindowPos(dll.hwnd, HWND_TOPMOST, dll.wnd.x, dll.wnd.y, dll.wnd.cx,
            dll.wnd.cy, SWP_NOACTIVATE or SWP_NOSENDCHANGING)
    SetWindowLongPtr(dll.hwnd, GWLP_WNDPROC, cast[LONG_PTR](wndProc))
    if timeout != 0:
        SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
                unsafeAddr timeout), 0)
    dll.isForegroundWnd = true

    return 0

when isMainModule:
    if not fileExists("ZetaLoader.ini"):
        writeFile("ZetaLoader.ini", "")
    CreateThread(nil, 0, cast[PTHREAD_START_ROUTINE](mainThread), nil, 0, nil)
