import winim/[lean, shell]
import os
import strutils
import parsecfg

type DLL = object
    hwnd: HWND
    dm: DEVMODE
    monitor: string
    x, y, cx, cy: int32
    primary, foreground: bool
    WindowProc: WNDPROC
    cfg: Config
var dll: DLL
dll.dm.dmSize = sizeof(DEVMODE).WORD
dll.dm.dmFields = DM_PELSWIDTH or DM_PELSHEIGHT
dll.primary = true
dll.foreground = false

proc NtSetTimerResolution(DesiredResolution: ULONG, SetResolution: BOOLEAN,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}
proc NtQueryTimerResolution(MinimumResolution, MaximumResolution,
        CurrentResolution: PULONG): LONG {.stdcall, dynlib: "ntdll.dll",
        importc, discardable.}

proc SetDM(dm: ptr DEVMODE) =
    if dll.dm.dmFields != 0:
        ChangeDisplaySettingsEx(dll.monitor, dm, 0, CDS_FULLSCREEN, nil)

proc BorderlessFullscreen() =
    SetWindowLongPtr(dll.hwnd, GWL_STYLE, WS_VISIBLE or WS_POPUP)
    SetWindowLongPtr(dll.hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW)
    SetWindowPos(dll.hwnd, HWND_TOPMOST, dll.x, dll.y, dll.cx, dll.cy,
            SWP_NOACTIVATE or SWP_NOSENDCHANGING)

proc ForegroundWindowLock(lparam: LPVOID): DWORD {.stdcall.} =
    let hthread = GetCurrentThread()
    SetThreadPriority(hthread, THREAD_MODE_BACKGROUND_BEGIN)
    SetThreadPriorityBoost(hthread, false)
    CloseHandle(hthread)
    while not dll.foreground:
        SwitchToThisWindow(dll.hwnd, true)
    return 0

proc WindowProc(hwnd: HWND, msg: UINT, wparam: WPARAM,
        lparam: LPARAM): LRESULT {.stdcall.} =
    case msg:
    of WM_DESTROY, WM_CLOSE, WM_QUIT:
        ShowWindow(hwnd, SW_HIDE)
        SetDM(nil)
    of WM_ACTIVATE, WM_ACTIVATEAPP:
        if dll.primary:
            case wparam:
            of WA_ACTIVE, WA_CLICKACTIVE:
                if (IsIconic(hwnd)):
                    SwitchToThisWindow(hwnd, TRUE)
                SetDM(addr dll.dm)
            of WA_INACTIVE:
                if not IsIconic(hwnd):
                    ShowWindow(hwnd, SW_MINIMIZE)
                SetDM(nil)
            else: discard
    of WM_WINDOWPOSCHANGING:
        BorderlessFullscreen()
        return 0
    else: discard
    return CallWindowProc(dll.WindowProc, hwnd, msg, wparam, lparam)

proc WinEventProc(hWinEventHook: HWINEVENTHOOK, event: DWORD, hwnd: HWND,
        idObject: LONG, idChild: LONG, idEventThread: DWORD,
        dwmsEventTime: DWORD) {.stdcall.} =
    if event != EVENT_OBJECT_SHOW and idobject != OBJID_WINDOW and idchild != CHILDID_SELF:
        return
    UnhookWinEvent(hWinEventHook)

    let
        tid = GetWindowThreadProcessId(hwnd, nil)
        hthread = OpenThread(THREAD_SET_INFORMATION, FALSE, tid)
    dll.hwnd = hwnd
    dll.WindowProc = cast[WNDPROC](GetWindowLongPtr(hwnd, GWLP_WNDPROC))

    DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED,
            addr dll.primary, 4)
    DwmSetWindowAttribute(hwnd, DWMWA_DISALLOW_PEEK, addr dll.primary, 4);
    DwmSetWindowAttribute(hwnd, DWMWA_FORCE_ICONIC_REPRESENTATION,
            addr dll.primary, 4)

    SetThreadPriority(hthread, THREAD_PRIORITY_TIME_CRITICAL)
    SetThreadPriorityBoost(hthread, FALSE)
    CloseHandle(hthread)

    PostQuitMessage(0)

proc MainThread(lparam: LPVOID): DWORD {.stdcall.} =
    if not fileExists("ZetaLoader.ini"):
        writeFile("ZetaLoader.ini", "")

    let
        timeout, pid: DWORD = GetCurrentProcessId()
        min, max, cur: ULONG = 0
        hprocess = GetCurrentProcess()
    var
        cfg = loadConfig("ZetaLoader.ini")
        (width, height) = (cfg.getSectionValue("Display", "Width").strip(),
                cfg.getSectionValue("Display", "Height").strip())
        hmonitor: HMONITOR
        mi: MONITORINFOEX
        dm: DEVMODE
        msg: MSG
    mi.cbSize = sizeof(MONITORINFOEX).DWORD

    SetPriorityClass(hprocess, ABOVE_NORMAL_PRIORITY_CLASS)
    SetProcessPriorityBoost(hprocess, false)
    CloseHandle(hprocess)
    NtQueryTimerResolution(unsafeAddr min, unsafeAddr max, unsafeAddr cur)
    NtSetTimerResolution(max, TRUE, unsafeAddr cur)
    DwmEnableMMCSS(true)
    AllowSetForegroundWindow(pid)
    SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
            unsafeAddr timeout), 0)

    SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, 0, WinEventProc, pid,
            0, WINEVENT_OUTOFCONTEXT)
    while GetMessage(addr msg, 0, 0, 0):
        TranslateMessage(addr msg)
        DispatchMessage(addr msg)

    hmonitor = MonitorFromWindow(dll.hwnd, MONITOR_DEFAULTTONEAREST)
    GetMonitorInfo(hmonitor, cast[ptr MONITORINFO](addr mi))
    for c in mi.szDevice:
        if c != 0: dll.monitor.add(cast[char](c))
    if mi.dwFlags != MONITORINFOF_PRIMARY:
        dll.primary = false
    EnumDisplaySettings(dll.monitor, ENUM_CURRENT_SETTINGS, addr dm)

    if width == "" or height == "":
        cfg.setSectionKey("Display", "Width", $dm.dmPelsWidth)
        cfg.setSectionKey("Display", "Height", $dm.dmPelsHeight)
        writeConfig(cfg, "ZetaLoader.ini")
        dll.dm.dmPelsWidth = dm.dmPelsWidth
        dll.dm.dmPelsHeight = dm.dmPelsHeight
    else:
        try:
            dll.dm.dmPelsWidth = width.parseInt().DWORD
            dll.dm.dmPelsHeight = height.parseInt().DWORD
            if (dll.dm.dmPelsHeight or dll.dm.dmPelsWidth) == 0:
                dll.dm.dmFields = 0
        except ValueError:
            discard

    if ChangeDisplaySettingsEx(dll.monitor, addr dll.dm, 0, CDS_TEST,
            nil) != DISP_CHANGE_SUCCESSFUL or GetWindowLongPtr(dll.hwnd,
                    GWL_STYLE) != (WS_VISIBLE or WS_OVERLAPPED or
                    WS_CLIPSIBLINGS):
        ShowWindow(dll.hwnd, SW_MAXIMIZE)
        return 0

    if dm.dmPelsWidth == dll.dm.dmPelsWidth and dm.dmPelsHeight ==
            dll.dm.dmPelsHeight:
        dll.dm.dmFields = 0

    SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](0), 0)
    CreateThread(nil, 0, ForegroundWindowLock, nil, 0, nil)
    SetDM(addr dll.dm)
    GetMonitorInfo(hmonitor, cast[ptr MONITORINFO](addr mi))
    dll.x = mi.rcMonitor.left
    dll.y = mi.rcMonitor.top
    dll.cx = mi.rcMonitor.right - mi.rcMonitor.left
    dll.cy = mi.rcMonitor.bottom - mi.rcMonitor.top
    BorderlessFullscreen()
    SetWindowLongPtr(dll.hwnd, GWLP_WNDPROC, cast[LONG_PTR](WindowProc))
    if timeout != 0:
        SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, cast[LPVOID](
                unsafeAddr timeout), 0)
    dll.foreground = true
    return 0

when isMainModule:
    if not fileExists("ZetaLoader.ini"):
        writeFile("ZetaLoader.ini", "")
    CreateThread(nil, 0, MainThread, nil, 0, nil)
