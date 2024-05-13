#include <windows.h>
#include <winrt/Windows.Data.Json.h>
using namespace winrt::Windows::Data::Json;

static BOOL fFullscreen = FALSE;
static DWORD dmPelsWidth = 0, dmPelsHeight = 0, dmDisplayFrequency = 0;
static WNDPROC lpPrevWndFunc = NULL;

static LRESULT WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATEANDEAT;

    case WM_CLOSE:
        return TerminateProcess(GetCurrentProcess(), 0);

    case WM_WINDOWPOSCHANGED:
        return lpPrevWndFunc(NULL, WM_SIZE, SIZE_RESTORED, 0);

    case WM_STYLECHANGING:
        ((LPSTYLESTRUCT)lParam)->styleNew = wParam == GWL_STYLE
                                                ? IsWindowVisible(hWnd) ? WS_VISIBLE | WS_POPUP : WS_POPUP
                                                : WS_EX_APPWINDOW | WS_EX_NOACTIVATE;
        break;

    case WM_NCACTIVATE:
    case WM_WINDOWPOSCHANGING:
        wParam = hWnd == GetForegroundWindow();
        DEVMODEW dm = {.dmSize = sizeof(DEVMODEW),
                       .dmFields = DM_DISPLAYORIENTATION | DM_DISPLAYFIXEDOUTPUT | DM_PELSWIDTH | DM_PELSHEIGHT |
                                   DM_DISPLAYFREQUENCY,
                       .dmDisplayOrientation = DMDO_DEFAULT,
                       .dmDisplayFixedOutput = DMDFO_DEFAULT,
                       .dmPelsWidth = dmPelsWidth,
                       .dmPelsHeight = dmPelsHeight,
                       .dmDisplayFrequency = dmDisplayFrequency};

        if (ChangeDisplaySettingsW(&dm, CDS_TEST))
        {
            EnumDisplaySettingsW(NULL, ENUM_REGISTRY_SETTINGS, &dm);
            if (dm.dmDisplayOrientation == DMDO_90 || dm.dmDisplayOrientation == DMDO_270)
                dm = {.dmSize = sizeof(DEVMODEW), .dmPelsWidth = dm.dmPelsHeight, .dmPelsHeight = dm.dmPelsWidth};
            dm.dmDisplayOrientation = DMDO_DEFAULT;
            dm.dmDisplayFixedOutput = DMDFO_DEFAULT;
            dm.dmFields =
                DM_DISPLAYORIENTATION | DM_DISPLAYFIXEDOUTPUT | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
        }

        if (uMsg == WM_NCACTIVATE)
        {
            SetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0, 0);
            ChangeDisplaySettingsW(wParam ? &dm : NULL, CDS_FULLSCREEN);
        }
        else
            *((PWINDOWPOS)lParam) = {.hwnd = hWnd,
                                     .hwndInsertAfter = wParam ? HWND_TOPMOST : HWND_BOTTOM,
                                     .x = 0,
                                     .y = 0,
                                     .cx = (int)dm.dmPelsWidth,
                                     .cy = (int)dm.dmPelsHeight,
                                     .flags = SWP_ASYNCWINDOWPOS | SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_SHOWWINDOW};
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static VOID WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                         DWORD dwEventThread, DWORD dwmsEventTime)
{
    WCHAR szClassName[256] = {};
    GetClassNameW(hwnd, szClassName, 256);

    if (CompareStringOrdinal(szClassName, -1, L"Halo Infinite", -1, FALSE) == CSTR_EQUAL ||
        CompareStringOrdinal(szClassName, -1, L"CampaignS1", -1, FALSE) == CSTR_EQUAL)
    {
        if (fFullscreen)
        {
            (lpPrevWndFunc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc))(NULL, WM_ACTIVATEAPP,
                                                                                                TRUE, 0);
            SetWindowLongPtrW(hwnd, GWL_STYLE, WS_OVERLAPPED);
            SetWindowLongPtrW(hwnd, GWL_EXSTYLE, WS_EX_LEFT | WS_EX_LTRREADING);
            SetClassLongPtrW(hwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursorW(NULL, IDC_ARROW));
            SetForegroundWindow(hwnd);
        }

        HANDLE hThread = OpenThread(THREAD_SET_LIMITED_INFORMATION, FALSE, dwEventThread);
        SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);
        CloseHandle(hThread);

        PostQuitMessage(0);
    }
}

static DWORD ThreadProc(LPVOID lpParameter)
{
    MSG msg = {};
    SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_CREATE, NULL, WinEventProc, GetCurrentProcessId(), 0,
                    WINEVENT_OUTOFCONTEXT);
    while (GetMessageW(&msg, NULL, 0, 0))
        ;
    return 0;
}

BOOL DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        WCHAR szFileName[MAX_PATH] = {};
        ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\HaloInfinite\\Settings\\ZetaLoader.ini", szFileName, MAX_PATH);

        if ((fFullscreen = GetPrivateProfileIntW(L"Settings", L"Fullscreen", FALSE, szFileName) == TRUE))
        {
            if (!(dmPelsWidth = GetPrivateProfileIntW(L"Settings", L"Width", -1, szFileName)) ||
                !(dmPelsHeight = GetPrivateProfileIntW(L"Settings", L"Height", -1, szFileName)) ||
                !(dmDisplayFrequency = GetPrivateProfileIntW(L"Settings", L"Frequency", -1, szFileName)))
                dmDisplayFrequency = -1;

            ExpandEnvironmentStringsW(L"%LOCALAPPDATA%\\HaloInfinite\\Settings\\SpecControlSettings.json", szFileName,
                                      MAX_PATH);
            HANDLE hFile = CreateFile2(szFileName, FILE_READ_DATA | FILE_WRITE_DATA, 0, OPEN_ALWAYS, NULL);

            if (hFile != INVALID_HANDLE_VALUE)
            {
                JsonObject pSpecControlSettings = NULL, pSpecControlWindowedDisplayResolutionX = JsonObject(),
                           pSpecControlWindowedDisplayResolutionY = JsonObject(), pSpecControlWindowMode = JsonObject();
                JsonValue pJsonValue = JsonValue::CreateNumberValue(0);

                pSpecControlWindowedDisplayResolutionX.SetNamedValue(L"version", pJsonValue);
                pSpecControlWindowedDisplayResolutionX.SetNamedValue(L"value", JsonValue::CreateNumberValue(1024));
                pSpecControlWindowedDisplayResolutionY.SetNamedValue(L"version", pJsonValue);
                pSpecControlWindowedDisplayResolutionY.SetNamedValue(L"value", JsonValue::CreateNumberValue(768));
                pSpecControlWindowMode.SetNamedValue(L"version", pJsonValue);
                pSpecControlWindowMode.SetNamedValue(L"value", pJsonValue);

                INT cbMultiByte = GetFileSize(hFile, NULL);
                LPSTR lpMultiByteStr = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbMultiByte + 1);

                ReadFile(hFile, lpMultiByteStr, cbMultiByte, NULL, NULL);
                if (!JsonObject::TryParse(winrt::to_hstring(lpMultiByteStr), pSpecControlSettings))
                    pSpecControlSettings = JsonObject();

                pSpecControlSettings.TryRemove(L"spec_control_window_size");
                pSpecControlSettings.TryRemove(L"spec_control_resolution_scale");

                pSpecControlSettings.SetNamedValue(L"spec_control_windowed_display_resolution_x",
                                                   pSpecControlWindowedDisplayResolutionX);
                pSpecControlSettings.SetNamedValue(L"spec_control_windowed_display_resolution_y",
                                                   pSpecControlWindowedDisplayResolutionY);
                pSpecControlSettings.SetNamedValue(L"spec_control_window_mode", pSpecControlWindowMode);

                winrt::hstring sWideCharStr = pSpecControlSettings.Stringify();
                cbMultiByte = WideCharToMultiByte(CP_UTF8, 0, sWideCharStr.c_str(), -1, NULL, 0, NULL, NULL);
                WideCharToMultiByte(CP_UTF8, 0, sWideCharStr.c_str(), -1,
                                    lpMultiByteStr = (LPSTR)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
                                                                        lpMultiByteStr, cbMultiByte),
                                    cbMultiByte, NULL, NULL);

                SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
                SetEndOfFile(hFile);
                WriteFile(hFile, lpMultiByteStr, cbMultiByte - 1, NULL, NULL);
                HeapFree(GetProcessHeap(), 0, lpMultiByteStr);
                CloseHandle(hFile);
            }
        }

        DisableThreadLibraryCalls(hinstDLL);
        CloseHandle(CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL));
    }
    return TRUE;
}