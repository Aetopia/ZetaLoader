# ZetaLoader

A utility to fix technical issues with Halo Infinite on PC.

## Fixes

1. Screen Tearing | Input Issues with External/Driver Based Framelimiters 

    > Source: https://forums.guru3d.com/threads/msi-ab-rtss-development-news-thread.412822/page-161#post-5949434   

    This fix simply resolves intense screen tearing and input related issues when an external/driver based framerate limiter is being used.
    Setting the window thread priority to `THREAD_PRIORITY_HIGHEST | THREAD_PRIORITY_TIME_CRITICAL` fixes this issue entirely.
    You can verify if this works by using a lower thread priority is set by `SetThreadPriority` in the source code and using an external framerate limiter like [RTSS](https://www.guru3d.com/files-details/rtss-rivatuner-statistics-server-download.html).       
    This fix was encountered when messing around with the window thread priority.

2. Borderless Fullscreen

    > **Available with Borderless fullscreen only at game startup.**

    ZetaLoader overrides Halo Infinite's borderless fullscreen/window style.            
    Halo Infinite uses the following styles `WS_VISIBLE | WS_OVERLAPPED | WS_CLIPSIBLINGS` when going borderless fullscreen.  
    
    The window style `WS_OVERLAPPED` doesn't fill the screen are correctly if the display resolution is below `1440x810` resulting in the window to extend its client area beyond the screen area itself. 

    This is simply fixed by using the following styles:      
    - Window Styles: `WS_VISIBLE | WS_POPUP` 
    - Extended Window Styles `WS_EX_APPWINDOW`  
    
    > You can use something like [Borderless Gaming](https://github.com/Codeusa/Borderless-Gaming) to fix this issue.
## Features   
1. User Defined Display Resolution/Mode Support

    > **Available with Borderless fullscreen only at game startup.**
    This feature restores the game's ability to run at any user defined resolution. Here is what this feature emulates/restores:
    - Automatically Minimize, when the game is not the foreground window.
    - Dynamically switch between your desired game display resolution & native display resolution like in exclusive fullscreen.            

    > Sometimes the taskbar can appear on top of Halo Infinite's window.        
    To fix this simply Alt + Tab back to the game to resolve the issue.

2. 0.5 Timer Resolution

    Using the hidden `NtSetTimerResolution` function, Halo Infinite can use a maximum timer resolution of 0.5 ms.
    >  Halo Infinite uses 1 ms by default, we can force 0.5 ms using NtSetTimerResolution.        
    Starting with Windows 2004, setting the timer resolution is no longer global but on a per process basis.       
    Reference: https://learn.microsoft.com/en-us/windows/win32/api/timeapi/nf-timeapi-timebeginperiod#remarks    

3.  Multimedia Class Schedule Service Scheduling

    Using the function `DwmEnableMMCSS` makes the calling process opt in for multimedia class schedule service scheduling which boosts performance in non-fullscreen applications.  
    [More information can be found here.](https://github.com/djdallmann/GamingPCSetup/blob/master/CONTENT/RESEARCH/WINSERVICES/README.md#multimedia-class-scheduler-service-mmcss)

## Results
> Specifications:     
    1. i7-10700K @ 4.8 GHz HT Disabled @ 1.15V   
    2. GTX 1650 @ 2055 ~ 2070 MHz (135 MHz+ Core) & 7200 MHz (1200 MHz+ RAM) OCs   
    3. 2933 MHz RAM (**But its mismatched!**)     

Result Video: https://www.youtube.com/watch?v=o9u0oAyv3dc

## Installation:
1. Download ZetaLoader from [GitHub Releases](https://github.com/Aetopia/ZetaLoader).
2. Browse Halo Infinite's local files & unzip `ZetaLoader.zip` in the game's installation directory.
3. Now `[Shift]` + `[Right Click]`, `ZetaLoader.exe` and select `[Copy As Path]`.
4. Go into Halo Infinite's launch options and use the following as the launch command:
    ```
    %ZetaLoader% %COMMAND%
    ```
    where `%ZetaLoader%` is the full path to `ZetaLoader.exe`.
5. Launch the game through or execute `ZetaLoader.exe`.
6. If the following happens, then ZetaLoader has been successfully installed:
    1. The game window automatically maximizes when it starts up in windowed mode.
    2. `ZetaLoader.txt` is generated in the game's installation directory.
    3. Display resolution swaps when the game window is the foreground window.
7.  To configure the resolution Halo Infinite uses, modify `ZetaLoader.txt`.
    To launch Halo Infinite at `1280 x 720`:
    ```
    1280
    720
    ```
    Also ensure **Borderless Fullscreen is enabled** & set this as the contents of `ZetaLoader.txt`.

## Uninstallation
1. Set Halo Infinite's launch options to blank.
2. Delete `ZetaLoader.exe`, `ZetaLoader.dll`, `ZetaLoader.txt` from Halo Infinite's installation directory.

# Building:
1. Install `GCC`. 
    > **Make sure it is in the system path!**
2. Run `build.bat`.

## Limitations

**Here are a few things you should note about ZetaLoader.**

1. Fixes specific techincal issues with the Halo Infinite.
2. Designed for the Steam release of Halo Infinite.
3. Might work with the UWP release or running under a Windows Compatibility Layer on Linux, but the source code might need to be rewritten for these platforms.
