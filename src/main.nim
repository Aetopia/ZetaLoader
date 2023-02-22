import winim/[lean, inc/dwmapi], strutils, parseopt, os
{.passL: "-lntdll".}

type Game = object
    devMode: DEVMODE
    szDevice: string
    wndX, wndY, wndCX, wndCY: int32
    userSpecifiedDisplayMode: bool
    wndProc: WNDPROC
let WM_SHELLHOOKMESSAGE = RegisterWindowMessage("SHELLHOOK")
var game: Game
game.userSpecifiedDisplayMode = true

proc NtSetTimerResolution(DesiredResolution: ULONG, SetResolution: BOOLEAN,
        CurrentResolution: PULONG): LONG {.stdcall,
        importc, discardable.}
proc NtQueryTimerResolution(MinimumResolution, MaximumResolution,
        CurrentResolution: PULONG): LONG {.stdcall,
        importc, discardable.}

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

proc wndProc(hWnd: HWND, msg: UINT, wParam: WPARAM,
        lParam: LPARAM): LRESULT {.stdcall.} =
    case msg:
    of WM_ACTIVATE, WM_ACTIVATEAPP:
        if wParam == WA_ACTIVE or
           wParam == WA_CLICKACTIVE:
            ShowWindow(hWnd, SW_RESTORE)
    of WM_CLOSE, WM_DESTROY: ShowWindow(hWnd, SW_MINIMIZE)
    of WM_SIZE:
        if game.userSpecifiedDisplayMode:
            if wParam == SIZE_RESTORED:
                ChangeDisplaySettingsEx(game.szDevice, addr game.devMode, 0,
                        CDS_FULLSCREEN, nil)
            elif wParam == SIZE_MINIMIZED:
                ChangeDisplaySettingsEx(game.szDevice, nil, 0,
                        CDS_FULLSCREEN, nil)

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
        if msg == WM_SHELLHOOKMESSAGE and
          (wParam == HSHELL_WINDOWACTIVATED or
           wParam == HSHELL_RUDEAPPACTIVATED):
            let hAWnd = HWND(lParam)
            if hAWnd != hWnd:
                var monitorInfo: MONITORINFOEX
                monitorInfo.cbSize = sizeof(monitorInfo).DWORD
                GetMonitorInfo(MonitorFromWindow(hAWnd,
                        MONITOR_DEFAULTTONEAREST), cast[ptr MONITORINFO](
                        addr monitorInfo))
                if monitorInfo.szDevice.wCharArrayToString() ==
                        game.szDevice:
                    ShowWindow(hWnd, SW_MINIMIZE)

    return CallWindowProc(game.wndProc, hwnd, msg, wParam, lParam)

proc winEventProc(hWinEventHook: HWINEVENTHOOK, event: DWORD, hWnd: HWND,
        idObject, idChild: LONG, idEventThread,
                dwmsEventTime: DWORD) {.stdcall.} =
    if event != EVENT_OBJECT_SHOW and
       idobject != OBJID_WINDOW and
       idchild != CHILDID_SELF: return
    UnhookWinEvent(hWinEventHook)

    let
        timeout: DWORD = 0
        min, max, cur: ULONG = 0
        hProcess = GetCurrentProcess()
        vAttribute = true
    var
        argDisplayMode: bool
        hThread = OpenThread(THREAD_SET_INFORMATION, FALSE, idEventThread)
        hMonitor: HMONITOR = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST)
        monitorInfo: MONITORINFOEX
    monitorInfo.cbSize = sizeof(MONITORINFOEX).DWORD
    game.wndProc = cast[WNDPROC](GetWindowLongPtr(hWnd, GWLP_WNDPROC))

    RegisterShellHookWindow(hWnd)
    GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr monitorInfo))
    game.szDevice = monitorInfo.szDevice.wCharArrayToString()
    EnumDisplaySettings(game.szDevice, ENUM_CURRENT_SETTINGS,
            addr game.devMode)

    NtQueryTimerResolution(unsafeAddr min, unsafeAddr max, unsafeAddr cur)
    NtSetTimerResolution(max, TRUE, unsafeAddr cur)
    DwmEnableMMCSS(true)

    SetPriorityClass(hProcess, ABOVE_NORMAL_PRIORITY_CLASS)
    SetProcessPriorityBoost(hProcess, false)
    CloseHandle(hProcess)

    SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL)
    SetThreadPriorityBoost(hThread, FALSE)
    CloseHandle(hThread)

    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
            unsafeAddr timeout), 0)

    for kind, key, value in getopt():
        if kind != cmdLongOption: continue

        if key == "displaymode" and not argDisplayMode:
            argDisplayMode = true
            try:
                let
                    param = value.split("_", 1)
                    resolution = param[0].split("x", 1)
                    width = resolution[0].parseInt.DWORD
                    height = resolution[1].parseInt.DWORD
                    displayFrequency = param[1].parseInt.DWORD

                if (width == game.devMode.dmPelsWidth and
                    height == game.devMode.dmPelsHeight and
                    displayFrequency == game.devMode.dmDisplayFrequency) or
                    (game.devMode.dmPelsWidth or
                     game.devMode.dmPelsHeight or
                     game.devMode.dmDisplayFrequency) == 0:
                    game.userSpecifiedDisplayMode = false

                else:
                    game.devMode.dmPelsWidth = width
                    game.devMode.dmPelsHeight = height
                    game.devMode.dmDisplayFrequency = displayFrequency

            except ValueError:
                game.userSpecifiedDisplayMode = false

        elif key == "dll":
            LoadLibrary(winstrConverterStringToLPWSTR(absolutePath(
                    value).toLower()))

    if GetWindowLongPtr(hWnd, GWL_STYLE) == (WS_VISIBLE or WS_OVERLAPPED or
            WS_CLIPSIBLINGS):

        SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](0), 0)
        hThread = CreateThread(nil, 0, cast[PTHREAD_START_ROUTINE](
                foregroundWndLock), cast[LPVOID](hWnd), 0, nil)

        DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED,
                unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_DISALLOW_PEEK, unsafeAddr vAttribute, 4)
        DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION,
                unsafeAddr vAttribute, 4)

        if game.userSpecifiedDisplayMode:
            game.userSpecifiedDisplayMode = ChangeDisplaySettingsEx(
                    game.szDevice, addr game.devMode, 0, CDS_FULLSCREEN, nil) == DISP_CHANGE_SUCCESSFUL

        GetMonitorInfo(hMonitor, cast[ptr MONITORINFO](addr monitorInfo))
        (game.wndX, game.wndY, game.wndCX, game.wndCY) = (monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right -
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom -
                monitorInfo.rcMonitor.top)

        SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE or WS_POPUP)
        SetWindowLongPtr(hWnd, GWL_EXSTYLE, WS_EX_APPWINDOW)
        SetWindowPos(hWnd, HWND_TOPMOST, game.wndX, game.wndY, game.wndCX,
                game.wndCY, SWP_NOSENDCHANGING)
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, cast[LONG_PTR](wndProc))

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
